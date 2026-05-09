/* async-scheduler.c — XEmacs cooperative coroutine scheduler */
#include <config.h>
#include "lisp.h"
#include "async-scheduler.h"
#include "events.h"
#include "systime.h"
#include "gc.h"
#include "opaque.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* Forward declaration to avoid circular include with async-http.h */
#ifdef HAVE_LIBCURL
extern void async_http_tick (void);
#endif

/* Debug logging macro — compiled out in release builds */
#ifdef DEBUG_XEMACS
#define ASYNC_DEBUG(...) stderr_out (__VA_ARGS__)
#else
#define ASYNC_DEBUG(...) ((void)0)
#endif

/* XEmacs interpreter state globals we must save/restore across coro_swap */
extern struct specbinding *specpdl;
extern struct specbinding *specpdl_ptr;
extern int specpdl_depth_counter;
extern int specpdl_size;
/* catchlist chain lives on the C stack, so must be saved/restored per coro.
   (handlerlist in backtrace.h is FSF-legacy dead code.) */
extern struct catchtag *catchlist;
/* Vcondition_handlers is a Lisp-object list mutated by every condition_case_1
   invocation.  Even though it's GC-managed, it is mutated as a global — each
   coroutine's condition_case_1 push/pop leaves it at a different value while
   the coroutine is running.  Must be saved/restored around coro_swap like
   catchlist, otherwise a suspended coroutine leaks its handler cons to the
   top level and later signals route to a catchtag no longer in catchlist. */
extern Lisp_Object Vcondition_handlers;

/* gcprolist: global head of the linked list of GCPRO'd C-stack slots.  Each
   Ffuncall frame pushes/pops.  When a coroutine runs Lisp, pushes land on
   the coroutine's malloc stack.  Must be saved/restored around coro_swap
   so the scheduler doesn't walk into the coroutine's GCPRO frames during a
   top-level GC. */
extern struct gcpro *gcprolist;

/* Default coroutine stack size: 256KB */
#define CORO_DEFAULT_STACK_SIZE (256 * 1024)

/* Initial per-coroutine specpdl size (entries).  XEmacs uses 50 by default;
   we use 200 to handle deeper binding stacks. */
#define CORO_SPECPDL_INITIAL_SIZE 200

/* The currently running coroutine (NULL = main Lisp thread) */
static xemacs_coro *current_coro = NULL;

/* Doubly-linked list of all live coroutines */
static xemacs_coro *all_coros = NULL;

/* Singly-linked list of dead coroutines awaiting join or GC */
static xemacs_coro *dead_coros = NULL;

/* Scheduler's own context — the "main" context we return to after each tick */
static coro_context_t scheduler_ctx;

/* Scheduler-level interpreter state saved before each coro_swap.
   These hold the GLOBAL specpdl/ptr while a coroutine is running on its
   own per-coro specpdl array. */
static struct specbinding *sched_specpdl;
static struct specbinding *sched_specpdl_ptr;
static int                 sched_specpdl_depth;
static int                 sched_specpdl_size;
static struct backtrace   *sched_backtrace_list;
static struct catchtag    *sched_catchlist;
static Lisp_Object         sched_condition_handlers;
static struct gcpro       *sched_gcprolist;

/* Number of live (non-DEAD) coroutines */
static int coro_count = 0;

/* Tick-within-tick re-entrancy guard (same thread, same C stack). */
static int in_tick = 0;

Lisp_Object Qasync_timeout;
Lisp_Object Qasync_error;

/* Lisp variable: default stack size */
static Fixnum Vasync_coroutine_stack_size;

/* ---- Intrusive list helpers ---- */

static void
coro_list_add (xemacs_coro *c)
{
  c->next = all_coros;
  c->prev = NULL;
  if (all_coros) all_coros->prev = c;
  all_coros = c;
  coro_count++;
}

static void
coro_list_remove (xemacs_coro *c)
{
  if (c->prev) c->prev->next = c->next;
  else all_coros = c->next;
  if (c->next) c->next->prev = c->prev;
  coro_count--;
}

static void
coro_graveyard_remove (xemacs_coro *c)
{
  xemacs_coro **p = &dead_coros;
  while (*p)
    {
      if (*p == c)
        {
          *p = c->next;
          return;
        }
      p = &(*p)->next;
    }
}

/* ---- specpdl swap helpers ----
   When context-switching between coroutines (or between coro and scheduler),
   any specpdl entries specific to the outgoing coroutine must have their
   symbol bindings "undone" (symbol values reset to pre-binding state), and
   the incoming coroutine's bindings must be re-applied.

   Each specbind entry stores (symbol, old_value).  "Swap" means:
     tmp = XSYMBOL_VALUE (entry->symbol);
     SET_SYMBOL_VALUE (entry->symbol, entry->old_value);
     entry->old_value = tmp;
   After the swap, old_value holds the symbol's value as it was AT the
   swap point, so a future swap correctly restores it.  Entries with
   non-NULL func (unwind_protect) are skipped. */

/* Swap symbol->value with entry->old_value for a specpdl entry.
   Uses direct access on simple (non-magic) bindings, falls back to Fset
   otherwise so buffer-local / variable-alias magic stays consistent. */
static inline void
specpdl_swap_one (struct specbinding *p)
{
  Lisp_Symbol *sym = XSYMBOL (p->symbol);
  Lisp_Object curr = sym->value;
  if (!SYMBOL_VALUE_MAGIC_P (curr) && !SYMBOL_VALUE_MAGIC_P (p->old_value))
    {
      sym->value = p->old_value;
      p->old_value = curr;
    }
  else
    {
      Lisp_Object tmp = p->old_value;
      p->old_value = Fsymbol_value (p->symbol);
      Fset (p->symbol, tmp);
    }
}

static void
specpdl_unwind_range (struct specbinding *from, struct specbinding *to)
{
  /* Walk from TOP down to BOTTOM (reverse order), undoing bindings */
  struct specbinding *p;
  for (p = to - 1; p >= from; p--)
    if (p->func == NULL)
      specpdl_swap_one (p);
}

static void
specpdl_rebind_range (struct specbinding *from, struct specbinding *to)
{
  /* Walk from BOTTOM up to TOP (forward order), re-applying bindings */
  struct specbinding *p;
  for (p = from; p < to; p++)
    if (p->func == NULL)
      specpdl_swap_one (p);
}

/* ---- Vcondition_handlers detach / re-attach across coro_swap ----

   Detach: called at yield/swap-out.  Splits the coroutine-owned prefix off
   the global Vcondition_handlers chain at the boundary cons whose CDR ==
   `sched_condition_handlers' (the snapshot taken at tick swap-in).  Stores
   (head, boundary) in the coro and nulls boundary's CDR so the prefix is
   self-contained while suspended.  Restores Vcondition_handlers to the
   outer snapshot.  No-op if the coro has added no handler frames.

   Re-attach: called at swap-in.  If the coro has a saved prefix, splices
   it back onto the top of the current Vcondition_handlers by setting the
   boundary cons's CDR to the current Vcondition_handlers, and makes the
   saved head the new Vcondition_handlers.  */

static void
coro_detach_condition_handlers (xemacs_coro *c)
{
  Lisp_Object head = Vcondition_handlers;
  Lisp_Object outer = sched_condition_handlers;

  if (EQ (head, outer))
    {
      /* Coro added nothing -- chain is exactly the outer world's chain. */
      c->saved_condition_handlers_head     = Qnil;
      c->saved_condition_handlers_boundary = Qnil;
    }
  else
    {
      /* Walk down from head until we find the boundary cons: the last
         coro-owned cell, whose CDR == outer.  We assume head != outer
         (checked above) and that the chain does reach outer; if it
         doesn't, something has already corrupted the chain and we'd
         rather not walk off the end. */
      Lisp_Object curr = head;
      while (CONSP (curr) && !EQ (XCDR (curr), outer))
        curr = XCDR (curr);

      if (CONSP (curr) && EQ (XCDR (curr), outer))
        {
          c->saved_condition_handlers_head     = head;
          c->saved_condition_handlers_boundary = curr;
          XSETCDR (curr, Qnil);
        }
      else
        {
          /* Chain didn't reach outer -- e.g. outer was `free_cons'd and a
             nested call already spliced over it.  Bail safely: keep the
             full chain as-is (will be marked by GC via the head). */
          c->saved_condition_handlers_head     = head;
          c->saved_condition_handlers_boundary = Qnil;
        }
    }

  Vcondition_handlers = outer;
}

static void
coro_reattach_condition_handlers (xemacs_coro *c)
{
  if (NILP (c->saved_condition_handlers_head))
    return;  /* Nothing to re-thread. */

  if (!NILP (c->saved_condition_handlers_boundary))
    XSETCDR (c->saved_condition_handlers_boundary, Vcondition_handlers);

  Vcondition_handlers = c->saved_condition_handlers_head;

  /* Clear the saved slots so GC won't hold references to the chain we just
     restored to the global.  The chain is now reachable from
     Vcondition_handlers (staticpro'd). */
  c->saved_condition_handlers_head     = Qnil;
  c->saved_condition_handlers_boundary = Qnil;
}

/* ---- Trampoline helpers for error trapping ---- */

/* coro_trampoline_call_arg: packed (fn . arg) cons for passing to condition_case_1 */
static Lisp_Object
coro_trampoline_body (Lisp_Object fn_and_arg)
{
  return call1 (XCAR (fn_and_arg), XCDR (fn_and_arg));
}

static Lisp_Object
coro_trampoline_handler (Lisp_Object errordata, Lisp_Object ignored)
{
  /* An error escaped the coroutine body.  Store it; don't re-signal here
     because we're still on the coroutine's stack. */
  current_coro->error = errordata;
  return Qnil;
}

/* ---- Trampoline: entry point for a new coroutine ---- */
static void
coro_trampoline (void)
{
  xemacs_coro *c = current_coro;
  /* Keep fn and arg in c->lisp_fn / c->result until after condition_case_1
     returns.  async_scheduler_mark_gcpros marks both fields, so the lambda
     (and its closure) stay alive across any GC while this coro is suspended.
     We pass them packed in a cons; the cons itself is also GC-visible as
     c->result until we overwrite it. */
  Lisp_Object fn_and_arg = Fcons (c->lisp_fn, c->result);
  Lisp_Object ret;

  /* Store fn_and_arg so GC can see it too while coroutine is suspended.
     We reuse c->result for this; c->lisp_fn keeps the lambda alive. */
  c->result = fn_and_arg;

  /* Run the Lisp function, catching any errors so they don't longjmp
     out of the coroutine context (which would leave specpdl in wrong state). */
  ret = condition_case_1 (Qt,
                          coro_trampoline_body, fn_and_arg,
                          coro_trampoline_handler, Qnil);

  /* Now clear fn references — we're done with the function */
  c->lisp_fn = Qnil;

  if (!NILP (c->error))
    {
      /* Error path: wake join_waiter with the error */
      if (c->join_waiter)
        {
          coro_resume_with_error (c->join_waiter, c->error);
          c->join_waiter = NULL;
          /* Arm a wakeup: the join_waiter is now RUNNABLE but Phase 2 of
             the current tick may have already passed it in the list. */
          async_schedule_wakeup_tick (1);
        }
    }
  else
    {
      /* Normal completion: wake any join_waiter */
      c->result = ret;
      if (c->join_waiter)
        {
          coro_resume (c->join_waiter, ret);
          c->join_waiter = NULL;
          async_schedule_wakeup_tick (1);
        }
    }

  /* Notify all monitors with (:exit reason), where reason is nil on normal
     exit or the error data on error exit. */
  {
    Lisp_Object reason = NILP (c->error) ? Qnil : c->error;
    Lisp_Object exit_msg = list2 (intern (":exit"), reason);
    int i;
    for (i = 0; i < c->monitor_count; i++)
      {
        xemacs_coro *watcher = c->monitor_list[i];
        if (watcher && watcher->state != CORO_DEAD)
          actor_send_internal (watcher, exit_msg);
      }
    if (c->monitor_list)
      {
        xfree (c->monitor_list);
        c->monitor_list = NULL;
        c->monitor_count = 0;
        c->monitor_cap = 0;
      }
  }

  c->state = CORO_DEAD;
  coro_list_remove (c);
  /* Move to graveyard — stack is still live here (we're running on it).
     The tick loop frees the stack after coro_swap returns. */
  c->next = dead_coros;
  dead_coros = c;

  /* Return to scheduler */
  coro_swap (&c->ctx, &scheduler_ctx);
  /* Never reached */
}

/* ---- spawn ---- */

xemacs_coro *
coro_spawn (Lisp_Object fn, Lisp_Object arg)
{
  xemacs_coro *c = xnew_and_zero (xemacs_coro);
  Bytecount stack_size = Vasync_coroutine_stack_size > 0
    ? Vasync_coroutine_stack_size
    : CORO_DEFAULT_STACK_SIZE;

  c->stack = (unsigned char *) xmalloc (stack_size);
  c->stack_size = stack_size;
  c->state = CORO_RUNNABLE;
  c->wait_reason = WAIT_NONE;
  c->wait_fd = -1;
  c->mailbox = Qnil;
  c->result = arg;
  c->error = Qnil;
  c->lisp_fn = fn;
  c->saved_gcprolist = NULL;
  c->join_waiter = NULL;
  c->monitor_list = NULL;
  c->monitor_count = 0;
  c->monitor_cap = 0;
  c->is_actor = 0;
  c->actor_name = Qnil;

  ASYNC_DEBUG ("[coro-spawn] new coro=%p fn=%p\n", (void *)c, (void *)fn);

  /* Allocate per-coroutine specpdl array, sized to hold the current spawning
     context's bindings plus headroom for the coroutine's own bindings. */
  {
    int inherit_depth = (current_coro != NULL)
                        ? sched_specpdl_depth
                        : specpdl_depth_counter;
    int initial_size = inherit_depth + CORO_SPECPDL_INITIAL_SIZE;
    c->coro_specpdl = (struct specbinding *)
      xmalloc (initial_size * sizeof (struct specbinding));
    c->coro_specpdl_size = initial_size;

    /* Copy the current interpreter's specpdl entries so the coroutine inherits
       the caller's dynamic bindings.  When spawned from inside a coroutine, the
       scheduler's specpdl is the interpreter's "base" — copy that. */
    if (current_coro != NULL)
      {
        memcpy (c->coro_specpdl, sched_specpdl,
                inherit_depth * sizeof (struct specbinding));
      }
    else
      {
        memcpy (c->coro_specpdl, specpdl,
                inherit_depth * sizeof (struct specbinding));
      }
    c->coro_specpdl_ptr = c->coro_specpdl + inherit_depth;
    c->coro_inherit_depth = inherit_depth;
  }

  /* Save the interpreter state the coroutine should start with.
     When spawned from inside a running coroutine, use the scheduler's saved
     state so the new coro gets a clean frame (not the spawner's). */
  if (current_coro != NULL)
    {
      c->saved_backtrace_list = sched_backtrace_list;
      c->saved_catchlist      = sched_catchlist;
    }
  else
    {
      c->saved_backtrace_list = backtrace_list;
      c->saved_catchlist      = catchlist;
    }

  /* The coroutine has not yet pushed any condition-case frames of its own,
     so it owns no prefix of Vcondition_handlers.  The detach/re-attach
     machinery treats a nil head as "nothing to re-thread" on first swap-in.  */
  c->saved_condition_handlers_head     = Qnil;
  c->saved_condition_handlers_boundary = Qnil;

  /* Set up initial stack frame.
     coro_swap on x86-64 pops: r15, r14, r13, r12, rbx, rbp then does ret.
     So the stack (growing downward) must look like, from low addr to high:
       [r15=0][r14=0][r13=0][r12=0][rbx=0][rbp=0][trampoline_addr][pad]
     with sp pointing at r15 (the lowest, first-popped slot).

     Alignment: x86-64 SysV ABI requires %rsp be 16-byte aligned at the
     point of a `call` instruction (i.e. %rsp % 16 == 0 just BEFORE call
     pushes the return address, so %rsp % 16 == 8 at the callee's first
     instruction).  Our 'ret' in coro_swap takes the place of a 'call'
     from the callee's perspective.  So we need %rsp % 16 == 0 right
     before ret, which means the trampoline-address slot must sit at a
     16-aligned address.  We add an 8-byte pad above the trampoline addr
     to achieve this: the topmost aligned address is the pad, then
     trampoline_addr is at (aligned-16) so popping it leaves %rsp aligned. */
  {
    unsigned char *stack_top = c->stack + stack_size;
    /* Align to 16 bytes */
    stack_top = (unsigned char *)((EMACS_UINT)stack_top & ~(EMACS_UINT)15);
    /* 8-byte alignment pad so trampoline sits at 16-aligned addr */
    stack_top -= sizeof (void *);
    /* Trampoline return address */
    stack_top -= sizeof (void *);
    *(void **)stack_top = (void *)coro_trampoline;
    /* 6 zero registers below */
    stack_top -= 6 * sizeof (void *); /* rbp, rbx, r12, r13, r14, r15 */
    memset (stack_top, 0, 6 * sizeof (void *));
    c->ctx.sp = stack_top;
  }

  coro_list_add (c);

  /* When spawned from outside any running coroutine (i.e. from top-level
     Lisp in the main thread), arm a wakeup so the event loop drives the
     first tick.

     When spawned from inside another coroutine with WAIT_NONE (immediately
     runnable), we also need to arm a wakeup. The Phase 2 loop that's
     currently running the spawner has already captured its snapshot of
     all_coros, so the new coro won't be picked up in this tick. Without
     this extra wakeup, a spawner that immediately waits on the child
     (e.g. via async-let → actor-join) will deadlock. */
  if (current_coro == NULL || c->wait_reason == WAIT_NONE)
    async_schedule_wakeup_tick (1);

  return c;
}

/* ---- yield ---- */

void
coro_yield (wait_reason_t reason)
{
  xemacs_coro *c = current_coro;
  assert (c != NULL);
  c->state = CORO_SUSPENDED;
  c->wait_reason = reason;

  ASYNC_DEBUG ("[coro-yield] coro=%p reason=%d\n", (void *)c, reason);

  /* Save the coroutine's current specpdl/catchlist/gcprolist positions */
  c->coro_specpdl_ptr          = specpdl_ptr;
  c->saved_catchlist           = catchlist;
  c->saved_gcprolist           = gcprolist;

  /* Detach the coroutine's Vcondition_handlers prefix from the outer
     world's chain and stash it on the coro.  Also restores
     Vcondition_handlers to sched_condition_handlers. */
  coro_detach_condition_handlers (c);

  /* Unwind the coroutine's own bindings (those above the inherit base),
     restoring the symbol values that were active when the coro was spawned. */
  specpdl_unwind_range (c->coro_specpdl + c->coro_inherit_depth,
                        c->coro_specpdl_ptr);

  /* Switch all global interpreter state back to the scheduler's */
  specpdl               = sched_specpdl;
  specpdl_ptr           = sched_specpdl_ptr;
  specpdl_depth_counter = sched_specpdl_depth;
  specpdl_size          = sched_specpdl_size;
  backtrace_list        = sched_backtrace_list;
  catchlist             = sched_catchlist;
  gcprolist             = sched_gcprolist;

  /* Arm an event-loop wakeup so something drives the next tick.
       WAIT_NONE:   voluntary yield -> resume ASAP.
       WAIT_TIMER:  wake at the timeout deadline so the tick can fire
                    coro_resume_with_error(Qasync_timeout) even if nothing
                    else on the system pokes the event loop.
       WAIT_FD:     poll every 10 ms so async_http_tick/curl_multi_perform
                    runs even in GUI (Xt) mode where poll_fds_for_input is
                    never called and the curl fds have no XtAppAddInput
                    handler.  10 ms matches libcurl's default timer cadence.
     WAIT_MAILBOX and WAIT_JOIN rely on external wakeups (actor-send or a
     dying coro), which arm wakeups at their own trigger points below. */
  if (reason == WAIT_NONE)
    async_schedule_wakeup_tick (1);
  else if (reason == WAIT_FD)
    async_schedule_wakeup_tick (10);
  else if (reason == WAIT_TIMER)
    {
      EMACS_TIME now, delta;
      long ms;
      EMACS_GET_TIME (now);
      if (EMACS_TIME_EQUAL_OR_GREATER (now, c->wait_deadline))
        ms = 1;  /* already past: fire immediately */
      else
        {
          EMACS_SUB_TIME (delta, c->wait_deadline, now);
          ms = (long) EMACS_SECS (delta) * 1000
             + (long) EMACS_USECS (delta) / 1000;
          if (ms < 1) ms = 1;
        }
      async_schedule_wakeup_tick ((unsigned int) ms);
    }

  current_coro = NULL;
  coro_swap (&c->ctx, &scheduler_ctx);
}

/* ---- resume ---- */

void
coro_resume (xemacs_coro *c, Lisp_Object result)
{
  c->result = result;
  c->error = Qnil;
  c->state = CORO_RUNNABLE;
  c->wait_reason = WAIT_NONE;
}

void
coro_resume_with_error (xemacs_coro *c, Lisp_Object error)
{
  c->result = Qnil;
  c->error = error;
  c->state = CORO_RUNNABLE;
  c->wait_reason = WAIT_NONE;
}

/* ---- Scheduler tick ---- */

/* When non-zero, async_scheduler_tick() becomes a no-op.  Bumped by
   event-stream.c around next_event_internal() and any other context where
   a coro_swap on the current C stack would corrupt caller-held state
   (GCPRO'd locals, specpdl, catchlist, backtrace, process i/o state).
   poll_fds_for_input() calls us after every select(), including the
   zero-timeout selects issued from inside next_event_internal on behalf
   of sleep-for/accept-process-output/etc.  Running coroutines from those
   nested call sites has been observed to SIGSEGV in next_event_internal
   when the coroutine's call-process / GC / specpdl activity invalidates
   state that next_event_internal cached before the select call.  We
   defer those ticks; the next top-level tick picks up any runnable coros. */
int async_tick_forbidden = 0;

static Lisp_Object
async_tick_unforbid (Lisp_Object ignored)
{
  (void) ignored;
  if (async_tick_forbidden > 0) async_tick_forbidden--;
  ASYNC_DEBUG ("[async-tick] unforbid: forbidden=%d\n", async_tick_forbidden);
  return Qnil;
}

void
async_tick_forbid_start (void)
{
  async_tick_forbidden++;
  ASYNC_DEBUG ("[async-tick] forbid_start: forbidden=%d\n", async_tick_forbidden);
  record_unwind_protect (async_tick_unforbid, Qnil);
}

void
async_scheduler_tick (void)
{
  xemacs_coro *c, *next_c;
  EMACS_TIME now;

  if (in_tick || async_tick_forbidden)
    {
      if (all_coros)
        ASYNC_DEBUG ("[async-tick] BLOCKED: in_tick=%d forbidden=%d coros=%d\n",
                    in_tick, async_tick_forbidden, coro_count);
      return;
    }
  in_tick = 1;

#ifdef HAVE_LIBCURL
  async_http_tick ();
#endif

  if (!all_coros) { in_tick = 0; return; }

  EMACS_GET_TIME (now);

  /* Phase 1a: voluntary yields (WAIT_NONE) become runnable every tick */
  for (c = all_coros; c; c = c->next)
    if (c->state == CORO_SUSPENDED && c->wait_reason == WAIT_NONE)
      coro_resume (c, c->result);

  /* Phase 1b: advance timer-waiting coroutines */
  for (c = all_coros; c; c = c->next)
    {
      if (c->state == CORO_SUSPENDED && c->wait_reason == WAIT_TIMER)
        if (EMACS_TIME_EQUAL_OR_GREATER (now, c->wait_deadline))
          coro_resume_with_error (c, list2 (Qasync_timeout,
                                            build_ascstring ("timeout")));
    }

  /* Phase 2: run all RUNNABLE coroutines */
  ASYNC_DEBUG ("[async-tick] Phase 2: checking %d coros\n", coro_count);
  for (c = all_coros; c; c = next_c)
    {
      next_c = c->next;
      if (c->state != CORO_RUNNABLE)
        {
          ASYNC_DEBUG ("[async-tick]   coro %p state=%d wait=%d (skip)\n",
                      (void *)c, c->state, c->wait_reason);
          continue;
        }
      ASYNC_DEBUG ("[async-tick]   coro %p RUNNABLE -> running\n", (void *)c);

      c->state = CORO_RUNNING;
      current_coro = c;

      /* Save scheduler's interpreter state, switch to coroutine's */
      sched_specpdl             = specpdl;
      sched_specpdl_ptr         = specpdl_ptr;
      sched_specpdl_depth       = specpdl_depth_counter;
      sched_specpdl_size        = specpdl_size;
      sched_backtrace_list      = backtrace_list;
      sched_catchlist           = catchlist;
      sched_condition_handlers  = Vcondition_handlers;
      sched_gcprolist           = gcprolist;

      specpdl               = c->coro_specpdl;
      specpdl_ptr           = c->coro_specpdl_ptr;
      specpdl_depth_counter = (int)(c->coro_specpdl_ptr - c->coro_specpdl);
      specpdl_size          = c->coro_specpdl_size;
      backtrace_list        = c->saved_backtrace_list;
      catchlist             = c->saved_catchlist;
      gcprolist             = c->saved_gcprolist;
      /* Re-thread the coro's Vcondition_handlers prefix (if any) onto the
         current outer chain.  This must happen AFTER we save the outer
         Vcondition_handlers to sched_condition_handlers above, because
         coro_reattach_condition_handlers reads Vcondition_handlers (outer)
         as the new tail for the boundary cons.  */
      coro_reattach_condition_handlers (c);

      /* Re-apply the coroutine's own bindings (entries above inherit base).
         The symbol values currently reflect the scheduler's state; rebinding
         walks forward through the coro's added entries, swapping each
         (symbol_value, old_value) so the coro sees its own bindings. */
      specpdl_rebind_range (c->coro_specpdl + c->coro_inherit_depth,
                            c->coro_specpdl_ptr);

      coro_swap (&scheduler_ctx, &c->ctx);
      current_coro = NULL;

      ASYNC_DEBUG ("[async-tick] coro %p returned, state=%d\n", (void *)c, c->state);

      /* Restore scheduler's interpreter state after coro_swap.
         coro_yield already switched state back; for death paths the trampoline
         does its own coro_swap, so we must always unconditionally restore here.

         Vcondition_handlers: on the yield path, coro_yield has already
         detached the coro's prefix and restored Vcondition_handlers to
         sched_condition_handlers.  On the death path, the trampoline's
         condition_case_1 ran to normal return and popped its own frame, so
         Vcondition_handlers now holds whatever the chain was *above* the
         trampoline cons -- which is the outer chain (modulo any conses the
         trampoline's exit handlers freed along the way).  Either way, forcing
         it back to sched_condition_handlers is the right thing: it matches
         what the tick's caller had before we started, and defensively papers
         over any imbalance in the death path.  */
      specpdl               = sched_specpdl;
      specpdl_ptr           = sched_specpdl_ptr;
      specpdl_depth_counter = sched_specpdl_depth;
      specpdl_size          = sched_specpdl_size;
      backtrace_list        = sched_backtrace_list;
      catchlist             = sched_catchlist;
      Vcondition_handlers   = sched_condition_handlers;
      gcprolist             = sched_gcprolist;

      /* Free the coroutine stack AFTER coro_swap (we're no longer running on it) */
      if (c->state == CORO_DEAD && c->stack != NULL)
        {
          xfree (c->stack);
          c->stack = NULL;
        }
    }

  /* If there are still WAIT_FD coroutines, re-arm a wakeup tick.
     In GUI (Xt) mode, add_extra_fd only sets TTY event loop masks,
     so Xt doesn't know about these fds and won't call their callbacks.
     We rely on periodic ticks to call async_http_tick() which drives
     curl_multi_perform().  10 ms matches libcurl's default timer cadence. */
  {
    xemacs_coro *c;
    int has_wait_fd = 0;
    for (c = all_coros; c; c = c->next)
      if (c->state == CORO_SUSPENDED && c->wait_reason == WAIT_FD)
        {
          has_wait_fd = 1;
          break;
        }
    if (has_wait_fd)
      async_schedule_wakeup_tick (10);
  }

  in_tick = 0;
}

/* ---- Wakeup timer: drive the scheduler from the event loop ----

   The legacy tick driver (event-unixoid.c's poll_fds_for_input) only fires
   when some fd becomes readable.  That leaves idle or timer-waiting
   coroutines stuck: a freshly-spawned coro never gets its first tick, and
   `actor-receive'-suspended worker actors never drain their mailbox until
   something else happens to poke the fd mask.

   The fix is to arm a one-shot timeout_event on the event queue whenever
   we know the scheduler has (or will soon have) work to do:
     - top-level spawn      -> 0 ms (run the new coro on the next event loop pass)
     - WAIT_NONE yield      -> 0 ms (voluntary yield should resume promptly)
     - WAIT_TIMER yield     -> deadline-ms  (wake at timeout)
     - external actor-send  -> 0 ms (recipient has a message to process)

   `event_stream_generate_wakeup' enqueues a timeout_event that the command
   loop dispatches via call1(function, object) from `execute_internal_event'.
   We use `Vasync_wakeup_tick_fn' (a symbol naming a thin wrapper around
   `async-scheduler-tick') as that function.  The tick runs OUTSIDE
   `next_event_internal', so `async_tick_forbidden' is zero and the tick
   proceeds normally. */

static Lisp_Object Vasync_wakeup_tick_fn;

void
async_schedule_wakeup_tick (unsigned int milliseconds)
{
  /* Skip in batch mode (no event loop; tests drive the tick explicitly) and
     during early init before event_stream is installed. */
  if (noninteractive || !event_stream)
    return;

  /* Fire at least 1 ms out: Xt clamps zero to 1 ms anyway, and the tty
     timeout queue treats 0 as "already expired".  Passing 0 is fine, but
     make our intent explicit. */
  if (milliseconds < 1)
    milliseconds = 1;

  ASYNC_DEBUG ("[async-wakeup] arming wakeup: %u ms, forbidden=%d\n",
              milliseconds, async_tick_forbidden);

  /* 0 for vanilliseconds = one-shot (no resignal). */
  event_stream_generate_wakeup (milliseconds, 0,
                                Vasync_wakeup_tick_fn, Qnil, 0);
}

/* Lisp callback for the timeout_event.  The dispatch layer calls
   call1(function, object) -- we accept the object arg and ignore it. */
DEFUN ("async--scheduler-tick-from-timer", Fasync__scheduler_tick_from_timer,
       1, 1, 0, /*
Internal: invoked from a timeout_event to run one scheduler tick.
OBJECT is the timeout's object slot; it is ignored.
*/
       (object))
{
  (void) object;
  ASYNC_DEBUG ("[async-timer-cb] tick-from-timer called, forbidden=%d\n",
              async_tick_forbidden);
  async_scheduler_tick ();
  return Qnil;
}

/* ---- GC integration ---- */

void
async_scheduler_mark_gcpros (void)
{
  xemacs_coro *c;
  /* If GC runs while we are context-switched INTO a coroutine, the outer
     world's Vcondition_handlers chain head lives only in
     sched_condition_handlers (the global Vcondition_handlers has been
     replaced with the coroutine's view).  */
  kkcc_gc_stack_push_lisp_object_0 (sched_condition_handlers);
  /* Similarly, the outer gcprolist is only reachable via sched_gcprolist
     when we're context-switched into a coro; walk it here so no GCPRO'd
     outer objects get reaped mid-tick. */
  {
    struct gcpro *p = sched_gcprolist;
    while (p)
      {
        int i;
        for (i = 0; i < p->nvars; i++)
          kkcc_gc_stack_push_lisp_object_0 (p->var[i]);
        p = p->next;
      }
  }
  for (c = all_coros; c; c = c->next)
    {
      kkcc_gc_stack_push_lisp_object_0 (c->mailbox);
      kkcc_gc_stack_push_lisp_object_0 (c->result);
      kkcc_gc_stack_push_lisp_object_0 (c->error);
      kkcc_gc_stack_push_lisp_object_0 (c->lisp_fn);
      kkcc_gc_stack_push_lisp_object_0 (c->actor_name);
      /* Marking the head transitively marks every cons in the detached
         coro prefix (boundary CDR is Qnil during suspension).  */
      kkcc_gc_stack_push_lisp_object_0 (c->saved_condition_handlers_head);
      /* Walk the coroutine's saved gcprolist (from its yield point) so any
         GCPRO'd Lisp_Objects on the coroutine's stack stay alive across a
         top-level GC while the coroutine is suspended.  The chain MAY
         eventually reach the outer world's gcprolist -- which is also
         reachable from the scheduler's own sched_gcprolist (or directly
         from gcprolist if no coroutine is currently running), so marking
         it multiple times is harmless. */
      {
        struct gcpro *p = c->saved_gcprolist;
        while (p)
          {
            int i;
            for (i = 0; i < p->nvars; i++)
              kkcc_gc_stack_push_lisp_object_0 (p->var[i]);
            p = p->next;
          }
      }
      /* Mark per-coroutine specpdl entries (the coroutine's own binding stack) */
      if (c->state == CORO_SUSPENDED || c->state == CORO_RUNNABLE)
        {
          struct specbinding *p;
          for (p = c->coro_specpdl; p < c->coro_specpdl_ptr; p++)
            {
              kkcc_gc_stack_push_lisp_object_0 (p->symbol);
              kkcc_gc_stack_push_lisp_object_0 (p->old_value);
            }
        }
    }
  /* Also mark dead (graveyard) coroutines — their results must survive GC */
  for (c = dead_coros; c; c = c->next)
    kkcc_gc_stack_push_lisp_object_0 (c->result);
}

/* ---- Accessor for current coroutine (used by async-http.c) ---- */

xemacs_coro *
async_scheduler_current_coro (void)
{
  return current_coro;
}

/* ---- Actor primitives ---- */

xemacs_coro *
actor_spawn_internal (Lisp_Object name, Lisp_Object fn, Lisp_Object args)
{
  /* Spawn with apply: call fn with args list using apply1 semantics.
     We use a lambda wrapper: store fn in lisp_fn, args in result. */
  xemacs_coro *c = coro_spawn (fn, NILP (args) ? Qnil : XCAR (args));
  c->is_actor = 1;
  c->actor_name = name;
  return c;
}

void
actor_send_internal (xemacs_coro *target, Lisp_Object msg)
{
  int woke_mailbox =
    (target->state == CORO_SUSPENDED && target->wait_reason == WAIT_MAILBOX);

  target->mailbox = nconc2 (target->mailbox, list1 (msg));
  if (woke_mailbox)
    coro_resume (target, msg);

  /* If we were called from outside any running coroutine (top-level Lisp)
     AND the recipient just became runnable, arm an event-loop wakeup so
     the tick actually dispatches the message.  If called from inside a
     coro, the sender's own yield will carry a wakeup -- or the in-flight
     tick will pick up the now-runnable recipient in its next phase-2
     iteration -- so we don't need to arm one here. */
  if (woke_mailbox && current_coro == NULL)
    async_schedule_wakeup_tick (1);
}

Lisp_Object
actor_receive_internal (int timeout_ms)
{
  xemacs_coro *c = current_coro;
  assert (c != NULL && c->is_actor);

  if (!NILP (c->mailbox))
    {
      Lisp_Object msg = XCAR (c->mailbox);
      c->mailbox = XCDR (c->mailbox);
      return msg;
    }

  if (timeout_ms > 0)
    {
      EMACS_TIME deadline;
      EMACS_GET_TIME (deadline);
      {
        long extra_sec  = timeout_ms / 1000;
        long extra_usec = (timeout_ms % 1000) * 1000;
        EMACS_SET_SECS  (deadline, EMACS_SECS  (deadline) + extra_sec);
        EMACS_SET_USECS (deadline, EMACS_USECS (deadline) + extra_usec);
        EMACS_NORMALIZE_TIME (deadline);
      }
      c->wait_deadline = deadline;
    }

  coro_yield (timeout_ms > 0 ? WAIT_TIMER : WAIT_MAILBOX);
  return c->result;
}

/* ---- DEFSUBR registrations ---- */

DEFUN ("async-spawn-coroutine", Fasync_spawn_coroutine, 2, 2, 0, /*
Internal: spawn a coroutine running (FUNCTION ARG).
Returns an opaque coroutine handle.
*/
       (function, arg))
{
  xemacs_coro *c = coro_spawn (function, arg);
  return make_opaque_ptr (c);
}

DEFUN ("async-scheduler-tick", Fasync_scheduler_tick, 0, 0, 0, /*
Internal: run one scheduler tick.  Called from the event loop.
*/
       ())
{
  async_scheduler_tick ();
  return Qnil;
}

DEFUN ("async-current-coroutine", Fasync_current_coroutine, 0, 0, 0, /*
Return opaque handle for the currently running coroutine, or nil.
*/
       ())
{
  return current_coro ? make_opaque_ptr (current_coro) : Qnil;
}

DEFUN ("async-coroutine-yield", Fasync_coroutine_yield, 0, 0, 0, /*
Yield the current coroutine back to the scheduler.
*/
       ())
{
  if (!current_coro)
    signal_error (Qasync_error, "async-yield called outside coroutine", Qnil);
  coro_yield (WAIT_NONE);
  return Qnil;
}

DEFUN ("async-coroutine-p", Fasync_coroutine_p, 1, 1, 0, /*
Return t if OBJECT is a live or dead coroutine handle, nil otherwise.
Used by `await' to distinguish coroutine handles from other values.
*/
       (object))
{
  xemacs_coro *c;
  if (!OPAQUE_PTRP (object))
    return Qnil;
  c = (xemacs_coro *) get_opaque_ptr (object);
  /* Validate by scanning all_coros and dead_coros */
  {
    xemacs_coro *p;
    for (p = all_coros; p; p = p->next)
      if (p == c) return Qt;
    for (p = dead_coros; p; p = p->next)
      if (p == c) return Qt;
  }
  return Qnil;
}

DEFUN ("async--coro-result", Fasync__coro_result, 1, 1, 0, /*
Internal: return the result field of coroutine HANDLE.
*/
       (handle))
{
  xemacs_coro *c;
  if (!OPAQUE_PTRP (handle))
    signal_error (Qasync_error, "not a coroutine handle", handle);
  c = (xemacs_coro *) get_opaque_ptr (handle);
  return c->result;
}

DEFUN ("async--coro-error", Fasync__coro_error, 1, 1, 0, /*
Internal: return the error field of coroutine HANDLE, or nil.
*/
       (handle))
{
  xemacs_coro *c;
  if (!OPAQUE_PTRP (handle))
    signal_error (Qasync_error, "not a coroutine handle", handle);
  c = (xemacs_coro *) get_opaque_ptr (handle);
  return c->error;
}

DEFUN ("actor-spawn-internal", Factor_spawn_internal, 2, 3, 0, /*
Internal: spawn an actor coroutine.  Returns opaque handle.
*/
       (name, function, args))
{
  xemacs_coro *c = actor_spawn_internal (name, function,
                                         NILP (args) ? Qnil : args);
  return make_opaque_ptr (c);
}

DEFUN ("actor-send-internal", Factor_send_internal, 2, 2, 0, /*
Internal: send MESSAGE to the actor identified by opaque HANDLE.
*/
       (handle, message))
{
  xemacs_coro *c;
  if (!OPAQUE_PTRP (handle))
    signal_error (Qasync_error, "not a coroutine handle", handle);
  c = (xemacs_coro *) get_opaque_ptr (handle);
  actor_send_internal (c, message);
  return Qnil;
}

DEFUN ("actor-receive-internal", Factor_receive_internal, 0, 1, 0, /*
Internal: receive next message in current actor's mailbox.
Optional TIMEOUT-MS milliseconds before signalling async-timeout.
*/
       (timeout_ms))
{
  int tms = NILP (timeout_ms) ? 0 : XFIXNUM (timeout_ms);
  return actor_receive_internal (tms);
}

DEFUN ("actor-join-internal", Factor_join_internal, 1, 1, 0, /*
Internal: suspend current coroutine until HANDLE's coroutine finishes.
*/
       (handle))
{
  xemacs_coro *target, *self;
  if (!OPAQUE_PTRP (handle))
    signal_error (Qasync_error, "not a coroutine handle", handle);
  target = (xemacs_coro *) get_opaque_ptr (handle);
  self = current_coro;

  if (!self)
    signal_error (Qasync_error, "actor-join called outside coroutine", Qnil);
  if (target->state == CORO_DEAD)
    {
      /* Already in graveyard: remove and free */
      Lisp_Object res = target->result;
      coro_graveyard_remove (target);
      xfree (target->coro_specpdl);
      if (target->monitor_list)
        xfree (target->monitor_list);
      xfree (target);
      return res;
    }

  target->join_waiter = self;
  coro_yield (WAIT_JOIN);
  return self->result;
}

DEFUN ("actor-monitor-internal", Factor_monitor_internal, 2, 2, 0, /*
Internal: add WATCHER as a monitor for TARGET actor.
When TARGET dies, WATCHER receives (:exit reason) via its mailbox.
Multiple monitors may be registered; join_waiter is not affected.
*/
       (target, watcher))
{
  xemacs_coro *t, *w;
  if (!OPAQUE_PTRP (target))
    signal_error (Qasync_error, "not a coroutine handle", target);
  if (!OPAQUE_PTRP (watcher))
    signal_error (Qasync_error, "not a coroutine handle", watcher);
  t = (xemacs_coro *) get_opaque_ptr (target);
  w = (xemacs_coro *) get_opaque_ptr (watcher);

  /* Grow monitor_list if needed */
  if (t->monitor_count >= t->monitor_cap)
    {
      int new_cap = t->monitor_cap == 0 ? 4 : t->monitor_cap * 2;
      t->monitor_list = (xemacs_coro **) xrealloc (t->monitor_list,
                                                    new_cap * sizeof (xemacs_coro *));
      t->monitor_cap = new_cap;
    }
  t->monitor_list[t->monitor_count++] = w;
  return Qnil;
}

void
syms_of_async_scheduler (void)
{
  DEFSYMBOL (Qasync_timeout);
  DEFSYMBOL (Qasync_error);
  DEFSUBR (Fasync_spawn_coroutine);
  DEFSUBR (Fasync_scheduler_tick);
  DEFSUBR (Fasync_current_coroutine);
  DEFSUBR (Fasync_coroutine_yield);
  DEFSUBR (Fasync_coroutine_p);
  DEFSUBR (Fasync__coro_result);
  DEFSUBR (Fasync__coro_error);
  DEFSUBR (Factor_spawn_internal);
  DEFSUBR (Factor_send_internal);
  DEFSUBR (Factor_receive_internal);
  DEFSUBR (Factor_join_internal);
  DEFSUBR (Factor_monitor_internal);
  DEFSUBR (Fasync__scheduler_tick_from_timer);
}

void
vars_of_async_scheduler (void)
{
  DEFVAR_INT ("async-coroutine-stack-size", &Vasync_coroutine_stack_size /*
Stack size in bytes for each coroutine.  Default is 262144 (256KB).
*/);
  Vasync_coroutine_stack_size = CORO_DEFAULT_STACK_SIZE;

  /* Staticpro the scheduler's saved handler-list slot so its head cons
     stays rooted while the interpreter is running inside a coroutine. */
  sched_condition_handlers = Qnil;
  staticpro (&sched_condition_handlers);

  /* Cache the symbol used as the timeout callback for wakeups.  We use the
     interned symbol directly so `call1' can look up its function value when
     the event loop dispatches the timeout. */
  Vasync_wakeup_tick_fn = intern ("async--scheduler-tick-from-timer");
  staticpro (&Vasync_wakeup_tick_fn);
}

void
init_async_scheduler (void)
{
  /* Nothing to do: scheduler runs on demand from event loop tick */
}
