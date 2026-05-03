/* async-http.h — libcurl multi-handle async HTTP for XEmacs */
#ifndef XEMACS_ASYNC_HTTP_H
#define XEMACS_ASYNC_HTTP_H

#ifdef HAVE_LIBCURL

#include "lisp.h"
#include "async-scheduler.h"
#include <curl/curl.h>

typedef struct async_http_req {
  CURL              *easy;
  xemacs_coro       *waiting_coro;
  Lisp_Object        response_buf;
  Lisp_Object        on_chunk;
  Lisp_Object        headers_alist;
  long               status_code;
  CURLcode           curl_result;
  struct curl_slist *req_headers;
} async_http_req;

extern Lisp_Object async_http_start (const char *url,
                                     const char *method,
                                     Lisp_Object req_headers_alist,
                                     Lisp_Object body_string,
                                     Lisp_Object on_chunk,
                                     int timeout_ms);

extern void async_http_socket_ready (int fd, void *data);
extern void async_http_tick (void);
extern void async_http_check_completed (void);
extern void init_async_http (void);
extern void syms_of_async_http (void);
extern void vars_of_async_http (void);

extern Lisp_Object Qasync_http_error;

#endif /* HAVE_LIBCURL */
#endif /* XEMACS_ASYNC_HTTP_H */
