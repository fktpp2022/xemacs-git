
/* Asynchronous I/O System for XEmacs
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

#ifndef INCLUDED_async_io_h_
#define INCLUDED_async_io_h_

#include &lt;config.h&gt;
#include "lisp.h"

/* Forward declarations */
struct async_coroutine;
struct async_task;
struct async_io_event;

/* Coroutine state */
typedef enum {
  COROUTINE_READY,
  COROUTINE_RUNNING,
  COROUTINE_SUSPENDED,
  COROUTINE_COMPLETED,
  COROUTINE_ERROR
} coroutine_state_t;

/* Async task types */
typedef enum {
  TASK_FILE_READ,
  TASK_FILE_WRITE,
  TASK_NET_CONNECT,
  TASK_NET_READ,
  TASK_NET_WRITE,
  TASK_USER_FUNCTION
} async_task_type_t;

/* Callback types */
typedef void (*async_callback_t)(Lisp_Object result, Lisp_Object data);
typedef void (*async_error_callback_t)(Lisp_Object error, Lisp_Object data);

/* Initialize async I/O system */
extern void init_async_io (void);

/* Coroutine functions */
extern Lisp_Object make_coroutine (Lisp_Object function);
extern Lisp_Object resume_coroutine (Lisp_Object coroutine);
extern void suspend_coroutine (void);

/* Task functions */
extern Lisp_Object async_start_task (Lisp_Object function, 
                                     Lisp_Object callback,
                                     Lisp_Object error_callback,
                                     Lisp_Object data);

/* Async I/O functions */
extern Lisp_Object async_read_file (Lisp_Object filename,
                                    Lisp_Object callback,
                                    Lisp_Object error_callback);
extern Lisp_Object async_write_file (Lisp_Object filename,
                                     Lisp_Object content,
                                     Lisp_Object callback,
                                     Lisp_Object error_callback);
extern Lisp_Object async_network_connect (Lisp_Object host,
                                          Lisp_Object port,
                                          Lisp_Object callback,
                                          Lisp_Object error_callback);

/* Event integration */
extern void enqueue_async_event (Lisp_Object event);

/* Lisp symbols */
extern Lisp_Object Qcoroutine;
extern Lisp_Object Qasynctask;
extern Lisp_Object Qasynciop;
extern Lisp_Object Qasyncevent;
extern Lisp_Object Qasync_error;

#endif /* INCLUDED_async_io_h_ */

