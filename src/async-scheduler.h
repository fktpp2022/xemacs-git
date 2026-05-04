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

  /* GC: Lisp objects live on the C stack — must be rooted manually */
  struct gcpro   *gcpro_chain;   /* chain head for this coro's live objects */

  /* Saved interpreter state (restored on each coro_swap) */
  struct specbinding *saved_specpdl_ptr;   /* specpdl_ptr at yield time */
  int                 saved_specpdl_depth;
  struct backtrace   *saved_backtrace_list;
  struct catchtag    *saved_catchlist;

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

  /* Join: other coroutines waiting on this one */
  struct xemacs_coro *join_waiter;

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
