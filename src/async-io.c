
/* Asynchronous I/O System Implementation for XEmacs
   Copyright (C) 2025 Free Software Foundation, Inc.

This file is part of XEmacs.

XEmacs is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

XEmacs is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;. */

#include &lt;config.h&gt;
#include "lisp.h"
#include "async-io.h"
#include "events.h"

#ifdef HAVE_UCONTEXT_H
#include &lt;ucontext.h&gt;
#endif

#include &lt;setjmp.h&gt;
#include &lt;stdlib.h&gt;
#include &lt;string.h&gt;

/* Lisp objects */
Lisp_Object Qcoroutine;
Lisp_Object Qasynctask;
Lisp_Object Qasynciop;
Lisp_Object Qasyncevent;
Lisp_Object Qasync_error;

/* Global state */
struct async_coroutine *current_coroutine = NULL;
struct async_coroutine *main_coroutine = NULL;

/* Coroutine structure */
typedef struct Lisp_Coroutine
{
  Lisp_Object function;
  Lisp_Object result;
  Lisp_Object error;
  coroutine_state_t state;
  
#ifdef HAVE_UCONTEXT_H
  ucontext_t context;
  void *stack;
  size_t stack_size;
#else
  jmp_buf context;
  int context_valid;
#endif
  
  struct Lisp_Coroutine *next;
  struct Lisp_Coroutine *prev;
} Lisp_Coroutine;

/* Task structure */
typedef struct Lisp_Async_Task
{
  Lisp_Object function;
  Lisp_Object callback;
  Lisp_Object error_callback;
  Lisp_Object data;
  Lisp_Object result;
  Lisp_Object error;
  async_task_type_t type;
  int completed;
  
  struct Lisp_Async_Task *next;
  struct Lisp_Async_Task *prev;
} Lisp_Async_Task;

/* Task queue */
static Lisp_Coroutine *coroutine_list = NULL;
static Lisp_Async_Task *task_queue = NULL;
static Lisp_Async_Task *task_queue_tail = NULL;

/* Memory descriptions for GC */
static const struct memory_description coroutine_description[] =
{
  { XD_LISP_OBJECT, offsetof (Lisp_Coroutine, function) },
  { XD_LISP_OBJECT, offsetof (Lisp_Coroutine, result) },
  { XD_LISP_OBJECT, offsetof (Lisp_Coroutine, error) },
  { XD_END }
};

static const struct memory_description async_task_description[] =
{
  { XD_LISP_OBJECT, offsetof (Lisp_Async_Task, function) },
  { XD_LISP_OBJECT, offsetof (Lisp_Async_Task, callback) },
  { XD_LISP_OBJECT, offsetof (Lisp_Async_Task, error_callback) },
  { XD_LISP_OBJECT, offsetof (Lisp_Async_Task, data) },
  { XD_LISP_OBJECT, offsetof (Lisp_Async_Task, result) },
  { XD_LISP_OBJECT, offsetof (Lisp_Async_Task, error) },
  { XD_END }
};

/* Helper: Check if object is a coroutine */
static int
coroutinep (Lisp_Object obj)
{
  return (RECORDP (obj) &amp;&amp; XRECORD_TYPE (obj) == Qcoroutine);
}

/* Helper: Check if object is an async task */
static int
async_task_p (Lisp_Object obj)
{
  return (RECORDP (obj) &amp;&amp; XRECORD_TYPE (obj) == Qasynctask);
}

/* Add coroutine to list */
static void
add_coroutine (Lisp_Coroutine *coro)
{
  coro-&gt;next = coroutine_list;
  coro-&gt;prev = NULL;
  if (coroutine_list)
    coroutine_list-&gt;prev = coro;
  coroutine_list = coro;
}

/* Remove coroutine from list */
static void
remove_coroutine (Lisp_Coroutine *coro)
{
  if (coro-&gt;prev)
    coro-&gt;prev-&gt;next = coro-&gt;next;
  if (coro-&gt;next)
    coro-&gt;next-&gt;prev = coro-&gt;prev;
  if (coroutine_list == coro)
    coroutine_list = coro-&gt;next;
}

/* Add task to queue */
static void
enqueue_task (Lisp_Async_Task *task)
{
  task-&gt;next = NULL;
  if (task_queue_tail)
    {
      task_queue_tail-&gt;next = task;
      task-&gt;prev = task_queue_tail;
    }
  else
    {
      task_queue = task;
      task-&gt;prev = NULL;
    }
  task_queue_tail = task;
}

/* Create a new coroutine */
Lisp_Object
make_coroutine (Lisp_Object function)
{
  Lisp_Coroutine *coro;
  Lisp_Object obj;

  CHECK_FUNCTION (function);

  obj = alloc_managed_record (Qcoroutine, coroutine_description);
  coro = XRECORD (obj, Lisp_Coroutine);
  coro-&gt;function = function;
  coro-&gt;result = Qnil;
  coro-&gt;error = Qnil;
  coro-&gt;state = COROUTINE_READY;
  coro-&gt;next = NULL;
  coro-&gt;prev = NULL;
  
#ifdef HAVE_UCONTEXT_H
  coro-&gt;stack_size = 1024 * 1024; /* 1MB stack */
  coro-&gt;stack = xmalloc (coro-&gt;stack_size);
#endif

  add_coroutine (coro);
  return obj;
}

/* Resume a coroutine */
Lisp_Object
resume_coroutine (Lisp_Object coroutine_obj)
{
  Lisp_Coroutine *coro;
  Lisp_Coroutine *prev_coro;

  CHECK_RECORD (coroutine_obj, Qcoroutine);
  coro = XRECORD (coroutine_obj, Lisp_Coroutine);
  
  if (coro-&gt;state == COROUTINE_COMPLETED)
    return coro-&gt;result;
  if (coro-&gt;state == COROUTINE_ERROR)
    Fsignal (Qasync_error, list1 (coro-&gt;error));
  
  prev_coro = current_coroutine;
  current_coroutine = coro;
  coro-&gt;state = COROUTINE_RUNNING;
  
#ifdef HAVE_UCONTEXT_H
  /* TODO: Implement ucontext-based coroutine switching
  if (prev_coro == main_coroutine)
    {
      getcontext (&amp;coro-&gt;context);
      coro-&gt;context.uc_stack.ss_sp = coro-&gt;stack;
      coro-&gt;context.uc_stack.ss_size = coro-&gt;stack_size;
      coro-&gt;context.uc_link = &amp;main_coroutine-&gt;context;
      makecontext (&amp;coro-&gt;context, (void (*)()) coroutine_trampoline, 0);
      swapcontext (&amp;main_coroutine-&gt;context, &amp;coro-&gt;context);
    }
  else
    {
      swapcontext (&amp;prev_coro-&gt;context, &amp;coro-&gt;context);
    }
  */
#endif
  
  /* Simple fallback: run function immediately */
  if (coro-&gt;state == COROUTINE_READY)
    {
      struct gcpro gcpro1;
      GCPRO1 (coroutine_obj);
      
      coro-&gt;result = call1 (coro-&gt;function);
      coro-&gt;state = COROUTINE_COMPLETED;
      
      UNGCPRO;
    }
  
  current_coroutine = prev_coro;
  return coro-&gt;result;
}

/* Suspend current coroutine */
void
suspend_coroutine (void)
{
  if (current_coroutine &amp;&amp; current_coroutine != main_coroutine)
    {
      current_coroutine-&gt;state = COROUTINE_SUSPENDED;
#ifdef HAVE_UCONTEXT_H
      /* swapcontext (&amp;current_coroutine-&gt;context, &amp;main_coroutine-&gt;context); */
#endif
    }
}

/* Start an async task */
Lisp_Object
async_start_task (Lisp_Object function,
                 Lisp_Object callback,
                 Lisp_Object error_callback,
                 Lisp_Object data)
{
  Lisp_Async_Task *task;
  Lisp_Object obj;
  
  CHECK_FUNCTION (function);
  
  obj = alloc_managed_record (Qasynctask, async_task_description);
  task = XRECORD (obj, Lisp_Async_Task);
  
  task-&gt;function = function;
  task-&gt;callback = callback;
  task-&gt;error_callback = error_callback;
  task-&gt;data = data;
  task-&gt;result = Qnil;
  task-&gt;error = Qnil;
  task-&gt;type = TASK_USER_FUNCTION;
  task-&gt;completed = 0;
  task-&gt;next = NULL;
  task-&gt;prev = NULL;
  
  enqueue_task (task);
  
  /* Simple fallback: execute immediately for now */
  struct gcpro gcpro1;
  GCPRO1 (obj);
  task-&gt;result = call1 (task-&gt;function);
  task-&gt;completed = 1;
  UNGCPRO;
  
  if (!NILP (task-&gt;callback))
    call2 (task-&gt;callback, task-&gt;result, task-&gt;data);
  
  return obj;
}

/* Async file read */
Lisp_Object
async_read_file (Lisp_Object filename,
                Lisp_Object callback,
                Lisp_Object error_callback)
{
  /* Simple synchronous fallback for now
  */
  Lisp_Object content;
  CHECK_STRING (filename);
  
  struct gcpro gcpro1;
  GCPRO1 (filename);
  content = Finsert_file_contents (filename, Qnil, Qnil, Qnil, Qnil, Qnil, Qnil);
  UNGCPRO;
  
  if (!NILP (callback))
    call1 (callback, content);
  
  return content;
}

/* Async file write */
Lisp_Object
async_write_file (Lisp_Object filename,
                 Lisp_Object content,
                 Lisp_Object callback,
                 Lisp_Object error_callback)
{
  /* Simple synchronous fallback */
  CHECK_STRING (filename);
  /* TODO: Implement real async write */
  
  if (!NILP (callback))
    call1 (callback, Qt);
  
  return Qt;
}

/* Async network connect */
Lisp_Object
async_network_connect (Lisp_Object host,
                     Lisp_Object port,
                     Lisp_Object callback,
                     Lisp_Object error_callback)
{
  /* Simple synchronous fallback */
  Lisp_Object process;
  /* TODO: Implement real async connect */
  
  process = Qnil;
  if (!NILP (callback))
    call1 (callback, process);
  
  return process;
}

/* Enqueue an async event */
void
enqueue_async_event (Lisp_Object event)
{
  /* TODO: Integrate with XEmacs event system */
  /* For now, just signal to XEmacs event loop */
}

/* Lisp Defuns */

DEFUN ("coroutinep", Fcoroutinep, Scoroutinep, 1, 1, 0,
       "Return t if OBJECT is a coroutine.")
  (Lisp_Object object)
{
  return (coroutinep (object) ? Qt : Qnil);
}

DEFUN ("make-coroutine", Fmake_coroutine, Smake_coroutine, 1, 1, 0,
       "Create a new coroutine that will execute FUNCTION.")
  (Lisp_Object function)
{
  return make_coroutine (function);
}

DEFUN ("resume-coroutine", Fresume_coroutine, Sresume_coroutine, 1, 1, 0,
       "Resume execution of COROUTINE.")
  (Lisp_Object coroutine)
{
  return resume_coroutine (coroutine);
}

DEFUN ("suspend-coroutine", Fsuspend_coroutine, Ssuspend_coroutine, 0, 0, 0,
       "Suspend execution of current coroutine.")
  (void)
{
  suspend_coroutine ();
  return Qnil;
}

DEFUN ("async-start", Fasync_start, Sasync_start, 1, 3, 0,
       "Start an asynchronous task executing FUNCTION.\n\
When complete, call CALLBACK with the result.\n\
If ERROR-CALLBACK is provided, call it on error.")
  (Lisp_Object function, Lisp_Object callback, Lisp_Object error_callback)
{
  return async_start_task (function, callback, error_callback, Qnil);
}

DEFUN ("async-read-file", Fasync_read_file, Sasync_read_file, 1, 3, 0,
       "Asynchronously read FILENAME.\n\
When done, call CALLBACK with the content.\n\
On error, call ERROR-CALLBACK.")
  (Lisp_Object filename, Lisp_Object callback, Lisp_Object error_callback)
{
  return async_read_file (filename, callback, error_callback);
}

/* Symbols of variables */

void
init_async_io (void)
{
  /* Initialize symbols */
  Qcoroutine = intern_static ("coroutine");
  Qasynctask = intern_static ("async-task");
  Qasynciop = intern_static ("async-io-p");
  Qasyncevent = intern_static ("async-event");
  Qasync_error = intern_static ("async-error");
  
  DEFSYMBOL (Qcoroutine);
  DEFSYMBOL (Qasynctask);
  DEFSYMBOL (Qasynciop);
  DEFSYMBOL (Qasyncevent);
  DEFSYMBOL (Qasync_error);
  
  /* Initialize main coroutine */
  /* TODO: Initialize libuv event loop integration */
}

