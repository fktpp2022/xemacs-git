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

/* XEmacs interpreter state globals we must save/restore across coro_swap */
extern struct specbinding *specpdl;
extern struct specbinding *specpdl_ptr;
extern int specpdl_depth_counter;
extern int specpdl_size;
/* catchlist chain lives on the C stack, so must be saved/restored per coro.
   (handlerlist in backtrace.h is FSF-legacy dead code; XEmacs uses
   Vcondition_handlers which is a Lisp-object chain and thus GC-managed.) */
extern struct catchtag *catchlist;

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

/* Number of live (non-DEAD) coroutines */
static int coro_count = 0;

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
  c->gcpro_chain = NULL;
  c->join_waiter = NULL;
  c->monitor_list = NULL;
  c->monitor_count = 0;
  c->monitor_cap = 0;
  c->is_actor = 0;
  c->actor_name = Qnil;

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

  /* Save the coroutine's current specpdl/catchlist positions */
  c->coro_specpdl_ptr    = specpdl_ptr;
  c->saved_catchlist     = catchlist;

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

void
async_scheduler_tick (void)
{
  xemacs_coro *c, *next_c;
  EMACS_TIME now;

#ifdef HAVE_LIBCURL
  async_http_tick ();
#endif

  if (!all_coros) return;

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
  for (c = all_coros; c; c = next_c)
    {
      next_c = c->next;
      if (c->state != CORO_RUNNABLE) continue;

      c->state = CORO_RUNNING;
      current_coro = c;

      /* Save scheduler's interpreter state, switch to coroutine's */
      sched_specpdl        = specpdl;
      sched_specpdl_ptr    = specpdl_ptr;
      sched_specpdl_depth  = specpdl_depth_counter;
      sched_specpdl_size   = specpdl_size;
      sched_backtrace_list = backtrace_list;
      sched_catchlist      = catchlist;

      specpdl               = c->coro_specpdl;
      specpdl_ptr           = c->coro_specpdl_ptr;
      specpdl_depth_counter = (int)(c->coro_specpdl_ptr - c->coro_specpdl);
      specpdl_size          = c->coro_specpdl_size;
      backtrace_list        = c->saved_backtrace_list;
      catchlist             = c->saved_catchlist;

      /* Re-apply the coroutine's own bindings (entries above inherit base).
         The symbol values currently reflect the scheduler's state; rebinding
         walks forward through the coro's added entries, swapping each
         (symbol_value, old_value) so the coro sees its own bindings. */
      specpdl_rebind_range (c->coro_specpdl + c->coro_inherit_depth,
                            c->coro_specpdl_ptr);

      coro_swap (&scheduler_ctx, &c->ctx);
      current_coro = NULL;

      /* Restore scheduler's interpreter state after coro_swap.
         coro_yield already switched state back; for death paths the trampoline
         does its own coro_swap, so we must always unconditionally restore here. */
      specpdl               = sched_specpdl;
      specpdl_ptr           = sched_specpdl_ptr;
      specpdl_depth_counter = sched_specpdl_depth;
      specpdl_size          = sched_specpdl_size;
      backtrace_list        = sched_backtrace_list;
      catchlist             = sched_catchlist;

      /* Free the coroutine stack AFTER coro_swap (we're no longer running on it) */
      if (c->state == CORO_DEAD && c->stack != NULL)
        {
          xfree (c->stack);
          c->stack = NULL;
        }
    }
}

/* ---- GC integration ---- */

void
async_scheduler_mark_gcpros (void)
{
  xemacs_coro *c;
  for (c = all_coros; c; c = c->next)
    {
      kkcc_gc_stack_push_lisp_object_0 (c->mailbox);
      kkcc_gc_stack_push_lisp_object_0 (c->result);
      kkcc_gc_stack_push_lisp_object_0 (c->error);
      kkcc_gc_stack_push_lisp_object_0 (c->lisp_fn);
      kkcc_gc_stack_push_lisp_object_0 (c->actor_name);
      {
        struct gcpro *p = c->gcpro_chain;
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
  target->mailbox = nconc2 (target->mailbox, list1 (msg));
  if (target->state == CORO_SUSPENDED && target->wait_reason == WAIT_MAILBOX)
    coro_resume (target, msg);
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
}

void
vars_of_async_scheduler (void)
{
  DEFVAR_INT ("async-coroutine-stack-size", &Vasync_coroutine_stack_size /*
Stack size in bytes for each coroutine.  Default is 262144 (256KB).
*/);
  Vasync_coroutine_stack_size = CORO_DEFAULT_STACK_SIZE;
}

void
init_async_scheduler (void)
{
  /* Nothing to do: scheduler runs on demand from event loop tick */
}
