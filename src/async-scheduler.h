/* async-scheduler.h — XEmacs cooperative coroutine scheduler */
#ifndef XEMACS_ASYNC_SCHEDULER_H
#define XEMACS_ASYNC_SCHEDULER_H

#include "lisp.h"
#include "systime.h"
#include "backtrace.h"

/* Coroutine state machine */
typedef enum {
  CORO_RUNNABLE  = 0,
  CORO_RUNNING   = 1,
  CORO_SUSPENDED = 2,
  CORO_DEAD      = 3
} coro_state_t;

/* Suspension reason — determines what will wake this coroutine */
typedef enum {
  WAIT_NONE     = 0,  /* runnable */
  WAIT_FD       = 1,  /* waiting for an fd to become ready */
  WAIT_TIMER    = 2,  /* waiting for a timeout */
  WAIT_MAILBOX  = 3,  /* waiting for an actor message */
  WAIT_JOIN     = 4,  /* waiting for another coroutine to finish */
} wait_reason_t;

struct xemacs_coro;

/* Context: just the saved stack pointer (coro_swap uses this) */
typedef struct {
  void *sp;
} coro_context_t;

typedef struct xemacs_coro {
  coro_context_t  ctx;           /* saved CPU state (stack pointer) */
  coro_context_t  caller_ctx;    /* scheduler's context to return to */
  unsigned char  *stack;         /* malloc'd coroutine stack */
  Bytecount       stack_size;    /* stack size in bytes */
  coro_state_t    state;
  wait_reason_t   wait_reason;

  /* Result/error delivered on resume */
  Lisp_Object     result;
  Lisp_Object     error;         /* non-nil -> re-signal on resume */

  /* Lisp function to call on first entry (GC-visible) */
  Lisp_Object     lisp_fn;

  /* GC: Lisp objects live on the C stack — must be rooted manually.

     The global `gcprolist' chains GCPRO'd stack-allocated Lisp_Objects for
     the mark phase.  Every Ffuncall frame (and many internal helpers) pushes
     onto it and pops on return.  When a coroutine runs Lisp, the pushes
     land on the coroutine's malloc stack.  On yield we must save and
     restore this pointer analogously to specpdl/catchlist, otherwise the
     scheduler (and anything it calls) runs with the coroutine's stale
     gcpros still at the head of gcprolist -- and GC walks those entries,
     dereferencing `var' pointers that may be scribbled-over coroutine
     stack memory.  */
  struct gcpro   *saved_gcprolist;

  /* Saved interpreter state (restored on each coro_swap) */
  struct specbinding *saved_specpdl_ptr;   /* specpdl_ptr at yield time */
  int                 saved_specpdl_depth;
  struct backtrace   *saved_backtrace_list;
  struct catchtag    *saved_catchlist;

  /* Vcondition_handlers is a global cons chain mutated by every
     condition_case_1 / call_with_condition_handler invocation.  Each push
     allocates a noseeum cons whose CDR snapshots Vcondition_handlers at
     entry.  When the coroutine runs, its trampoline's condition_case_1 (and
     any nested user condition-case) prepend cons cells to the chain whose
     deepest CDR points into the OUTER world's chain (e.g. the test harness's
     `call-with-condition-handler' frame).  If the coroutine yields while
     those inner frames are active, the outer cells it transitively references
     may be `free_cons'd when the outer frames unwind -- leaving the
     coroutine's saved chain with a dangling CDR, which crashes the next GC.

     At yield we DETACH the coroutine-owned prefix from the outer world:
     walk the chain from Vcondition_handlers down to the boundary cons whose
     CDR equals `sched_condition_handlers' (the outer snapshot), NULL that
     CDR, and save (head, boundary) for later.  At swap-in we RE-ATTACH by
     setting the saved boundary's CDR to the current outer Vcondition_handlers
     and setting Vcondition_handlers to the saved head.  The coroutine-owned
     cells stay self-contained during suspension; GC marks them transitively
     starting from `saved_condition_handlers_head'.  */
  Lisp_Object         saved_condition_handlers_head;
  Lisp_Object         saved_condition_handlers_boundary;

  /* Per-coroutine specpdl: each coroutine has its own binding stack */
  struct specbinding *coro_specpdl;        /* malloc'd per-coro specpdl array */
  struct specbinding *coro_specpdl_ptr;    /* current position in coro_specpdl */
  int                 coro_specpdl_size;   /* allocated size */
  int                 coro_inherit_depth;  /* count of inherited (shared) entries */

  /* Suspension details */
  int             wait_fd;       /* fd we're blocked on (WAIT_FD) */
  EMACS_TIME      wait_deadline; /* absolute timeout (WAIT_TIMER) */

  /* Actor mailbox */
  Lisp_Object     mailbox;       /* list of pending messages */
  Lisp_Object     actor_name;    /* symbol or nil */
  int             is_actor;

  /* Join: single coroutine waiting for this one to finish */
  struct xemacs_coro *join_waiter;

  /* Monitors: list of coroutines to notify with (:exit reason) on death */
  struct xemacs_coro **monitor_list;
  int                  monitor_count;
  int                  monitor_cap;

  /* Linked list of all coroutines */
  struct xemacs_coro *next;
  struct xemacs_coro *prev;
} xemacs_coro;

/* Assembly primitive — swaps stack pointer between two coro contexts */
extern void coro_swap (coro_context_t *from, coro_context_t *to);

/* Lifecycle */
extern xemacs_coro *coro_spawn (Lisp_Object fn, Lisp_Object arg);
extern void         coro_yield (wait_reason_t reason);
extern void         coro_resume (xemacs_coro *c, Lisp_Object result);
extern void         coro_resume_with_error (xemacs_coro *c, Lisp_Object error);

/* Scheduler tick — called by event-unixoid.c after select() */
extern void async_scheduler_tick (void);

/* Arm a one-shot event-loop timeout that runs a scheduler tick.  In
   interactive mode this registers with event_stream_generate_wakeup so the
   tick fires from the top-level event loop regardless of whether any fd
   activity happens.  No-op in batch mode (tests drive the tick explicitly)
   and harmless during early init before event_stream is installed.  */
extern void async_schedule_wakeup_tick (unsigned int milliseconds);

/* When non-zero, async_scheduler_tick() becomes a no-op.  Bumped around
   next_event_internal() (and any other caller frame holding GCPRO'd or
   other invalidatable state) so that nested poll_fds_for_input → tick
   calls cannot coro_swap underneath an active event-processing stack. */
extern int async_tick_forbidden;

/* Bump async_tick_forbidden and register an unwind-protect that decrements
   it.  Must be called from a context with an active specpdl frame. */
extern void async_tick_forbid_start (void);

/* Actor primitives (used by actor.el via DEFSUBR) */
extern xemacs_coro *actor_spawn_internal (Lisp_Object name, Lisp_Object fn,
                                          Lisp_Object args);
extern void         actor_send_internal (xemacs_coro *target, Lisp_Object msg);
extern Lisp_Object  actor_receive_internal (int timeout_ms);

/* GC integration — called by the garbage collector */
extern void async_scheduler_mark_gcpros (void);

/* Returns the currently executing coroutine, or NULL if in main thread */
extern xemacs_coro *async_scheduler_current_coro (void);

/* Module init */
extern void syms_of_async_scheduler (void);
extern void vars_of_async_scheduler (void);
extern void init_async_scheduler (void);

/* Lisp-visible error symbols */
extern Lisp_Object Qasync_timeout;
extern Lisp_Object Qasync_error;

#endif /* XEMACS_ASYNC_SCHEDULER_H */
