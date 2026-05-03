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

/* Default coroutine stack size: 256KB */
#define CORO_DEFAULT_STACK_SIZE (256 * 1024)

/* The currently running coroutine (NULL = main Lisp thread) */
static xemacs_coro *current_coro = NULL;

/* Doubly-linked list of all live coroutines */
static xemacs_coro *all_coros = NULL;

/* Scheduler's own context — the "main" context we return to after each tick */
static coro_context_t scheduler_ctx;

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

/* ---- Trampoline: entry point for a new coroutine ---- */
static void
coro_trampoline (void)
{
  xemacs_coro *c = current_coro;
  Lisp_Object ret = Qnil;
  Lisp_Object (*fn)(Lisp_Object) = c->fn;
  Lisp_Object arg = c->result;

  c->result = Qnil;

  /* Run the function */
  ret = fn (arg);

  /* Normal completion: wake any join_waiter */
  c->result = ret;
  if (c->join_waiter)
    {
      coro_resume (c->join_waiter, ret);
      c->join_waiter = NULL;
    }

  c->state = CORO_DEAD;
  coro_list_remove (c);

  /* Return to scheduler */
  coro_swap (&c->ctx, &scheduler_ctx);
  /* Never reached */
}

/* ---- spawn ---- */

xemacs_coro *
coro_spawn (Lisp_Object (*fn)(Lisp_Object), Lisp_Object arg)
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
  c->fn = fn;
  c->gcpro_chain = NULL;
  c->join_waiter = NULL;
  c->is_actor = 0;
  c->actor_name = Qnil;

  /* Set up initial stack frame.
     coro_swap on x86-64 pops: r15, r14, r13, r12, rbx, rbp then does ret.
     So the stack (growing downward) must look like, from low addr to high:
       [r15=0][r14=0][r13=0][r12=0][rbx=0][rbp=0][trampoline_addr]
     with sp pointing at r15 (the lowest, first-popped slot).  */
  {
    unsigned char *stack_top = c->stack + stack_size;
    /* Align to 16 bytes */
    stack_top = (unsigned char *)((EMACS_UINT)stack_top & ~(EMACS_UINT)15);
    /* Place trampoline return address at top, then 6 zero registers below */
    stack_top -= sizeof (void *);
    *(void **)stack_top = (void *)coro_trampoline;
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

  if (!all_coros) return;

  EMACS_GET_TIME (now);

  /* Phase 1: advance timer-waiting coroutines */
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

      coro_swap (&scheduler_ctx, &c->ctx);
      current_coro = NULL;

      if (c->state == CORO_DEAD)
        {
          xfree (c->stack);
          xfree (c);
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
    }
}

/* ---- Actor primitives ---- */

xemacs_coro *
actor_spawn_internal (Lisp_Object name, Lisp_Object fn, Lisp_Object args)
{
  xemacs_coro *c = coro_spawn (
    (Lisp_Object (*)(Lisp_Object)) Ffuncall,
    Fcons (fn, args));
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
Internal: spawn a coroutine running FUNCTION called with ARG.
Returns an opaque coroutine handle.
*/
       (function, arg))
{
  xemacs_coro *c = coro_spawn (
    (Lisp_Object (*)(Lisp_Object)) Ffuncall,
    list2 (function, arg));
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
    return target->result;

  target->join_waiter = self;
  coro_yield (WAIT_JOIN);
  return self->result;
}

DEFUN ("actor-monitor-internal", Factor_monitor_internal, 2, 2, 0, /*
Internal: set WATCHER as the monitor for TARGET actor.
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
  t->join_waiter = w;
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
