/* async-http.c — libcurl multi-handle async HTTP for XEmacs */
#include <config.h>
#include "lisp.h"
#include "async-http.h"
#include "events.h"
#include "sysfile.h"
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LIBCURL

/* Refuse to chase an unbounded redirect chain. */
#define ASYNC_HTTP_MAX_REDIRS 10L

/* Always bound the TCP connect phase, even when no total timeout is given.
   A caller-supplied :timeout shorter than this wins. */
#define ASYNC_HTTP_CONNECT_TIMEOUT_MS 10000L

static CURLM *curl_multi = NULL;

Lisp_Object Qasync_http_error;

/* ---- Write callback ---- */

static size_t
http_write_callback (char *ptr, size_t size, size_t nmemb, void *userdata)
{
  async_http_req *req = (async_http_req *) userdata;
  size_t nbytes = size * nmemb;
  struct gcpro gcpro1, gcpro2;

  /* File destination: write the raw bytes straight to disk.  No Lisp
     string is built for the body on this path. */
  if (req->file)
    {
      size_t written = fwrite (ptr, 1, nbytes, req->file);
      return (written == nbytes) ? nbytes : 0;
    }

  /* libcurl hands us raw external bytes.  make_string() would treat them
     as internal Mule bytes and crash on 0x80-0xFF; build a binary external
     string instead so the body survives byte-for-byte. */
  GCPRO2 (req->response_buf, req->on_chunk);
  {
    Lisp_Object chunk = make_extstring ((Extbyte *) ptr,
                                        (EMACS_INT) nbytes, Qbinary);

    if (!NILP (req->on_chunk))
      {
        enqueue_misc_user_event (Qnil, req->on_chunk, chunk);
        signal_async_wakeup ();
      }
    else
      {
        req->response_buf = concat2 (req->response_buf, chunk);
      }
  }
  UNGCPRO;

  return nbytes;
}

/* ---- Header callback ---- */

static size_t
http_header_callback (char *ptr, size_t size, size_t nmemb, void *userdata)
{
  async_http_req *req = (async_http_req *) userdata;
  size_t nbytes = size * nmemb;
  struct gcpro gcpro1;

  GCPRO1 (req->headers_alist);
  /* Headers end in \r\n; skip the final blank line */
  if (nbytes > 2 && ptr[nbytes - 2] == '\r')
    {
      char *colon = memchr (ptr, ':', nbytes);
      if (colon)
        {
          Lisp_Object name = make_string ((Ibyte *) ptr,
                                          (Bytecount) (colon - ptr));
          char *val = colon + 1;
          while (*val == ' ') val++;
          size_t vlen = nbytes - (size_t)(val - ptr) - 2; /* strip \r\n */
          Lisp_Object value = make_string ((Ibyte *) val, (Bytecount) vlen);
          req->headers_alist = Fcons (Fcons (name, value), req->headers_alist);
        }
    }
  UNGCPRO;
  return nbytes;
}

/* ---- Socket callback (called by curl to add/remove fds) ---- */

static int
async_http_socket_cb (CURL *easy, curl_socket_t s, int action,
                      void *userp, void *socketp)
{
  (void) easy; (void) userp; (void) socketp;
  if (action == CURL_POLL_REMOVE)
    remove_extra_fd ((int) s);
  else
    add_extra_fd ((int) s, async_http_socket_ready, NULL);
  return 0;
}

/* ---- Timer callback (libcurl wants us to call socket_action soon) ---- */

static int
curl_timer_callback (CURLM *multi, long timeout_ms, void *userp)
{
  (void) multi; (void) userp; (void) timeout_ms;
  /* We poll via async_http_tick(); no explicit timer needed. */
  return 0;
}

/* ---- Called when select() says an fd is ready ---- */

void
async_http_socket_ready (int fd, void *data)
{
  (void) data;
  int running;
  curl_multi_socket_action (curl_multi, (curl_socket_t) fd,
                             CURL_CSELECT_IN | CURL_CSELECT_OUT, &running);
  async_http_check_completed ();
}

/* ---- Check for completed transfers and wake waiting coroutines ---- */

void
async_http_check_completed (void)
{
  CURLMsg *msg;
  int msgs_left;

  while ((msg = curl_multi_info_read (curl_multi, &msgs_left)) != NULL)
    {
      if (msg->msg == CURLMSG_DONE)
        {
          CURL *easy = msg->easy_handle;
          async_http_req *req = NULL;
          curl_easy_getinfo (easy, CURLINFO_PRIVATE, &req);
          if (!req) continue;

          curl_easy_getinfo (easy, CURLINFO_RESPONSE_CODE, &req->status_code);
          req->curl_result = msg->data.result;

          curl_multi_remove_handle (curl_multi, easy);
          curl_easy_cleanup (easy);
          req->easy = NULL;

          if (req->req_headers)
            {
              curl_slist_free_all (req->req_headers);
              req->req_headers = NULL;
            }

          if (req->file)
            {
              if (fclose (req->file) != 0
                  && req->curl_result == CURLE_OK)
                req->curl_result = CURLE_WRITE_ERROR;
              req->file = NULL;
            }

          /* Do not leave a partial download behind on failure. */
          if (req->curl_result != CURLE_OK && req->destination_path)
            qxe_unlink ((const Ibyte *) req->destination_path);

          if (req->waiting_coro)
            {
              xemacs_coro *coro = req->waiting_coro;
              struct gcpro gcpro1, gcpro2;

              req->waiting_coro = NULL;
              GCPRO2 (req->headers_alist, req->response_buf);

              if (req->curl_result == CURLE_OK)
                {
                  Lisp_Object third =
                    req->destination_path
                    ? make_extstring ((Extbyte *) req->destination_path,
                                      (EMACS_INT) strlen (req->destination_path),
                                      Qfile_name)
                    : req->response_buf;

                  coro_resume (coro,
                               list3 (make_fixnum (req->status_code),
                                      req->headers_alist,
                                      third));
                }
              else
                coro_resume_with_error (
                  coro,
                  list2 (Qasync_http_error,
                         build_ascstring (
                           curl_easy_strerror (req->curl_result))));
              UNGCPRO;
            }

          if (req->destination_path)
            xfree (req->destination_path);
          xfree (req);
        }
    }
}

/* ---- Periodic tick called from the scheduler ----
   In batch mode we do not have a real select() loop registering curl's
   sockets, so we drive the transfer synchronously via curl_multi_perform,
   which internally selects and progresses all handles.  In interactive mode
   the socket_action callback still registers fds with add_extra_fd, and
   async_http_socket_ready calls curl_multi_socket_action — but calling
   curl_multi_perform here is also safe and harmless (it is a no-op when no
   handles need work). */

void
async_http_tick (void)
{
  int running;

  if (!curl_multi) return;

  curl_multi_perform (curl_multi, &running);
  async_http_check_completed ();
}

/* ---- Start an async HTTP request from the current coroutine ---- */

Lisp_Object
async_http_start (const char *url, const char *method,
                  Lisp_Object req_headers_alist,
                  Lisp_Object body_string,
                  Lisp_Object on_chunk,
                  int timeout_ms,
                  Lisp_Object destination)
{
  async_http_req *req = xnew_and_zero (async_http_req);
  CURL *easy = curl_easy_init ();
  long connect_timeout_ms = ASYNC_HTTP_CONNECT_TIMEOUT_MS;

  req->easy          = easy;
  req->response_buf  = Qnil;
  req->on_chunk      = on_chunk;
  req->headers_alist = Qnil;
  req->waiting_coro  = async_scheduler_current_coro ();

  if (!NILP (destination))
    {
      req->destination_path =
        (char *) LISP_STRING_TO_EXTERNAL_MALLOC (destination, Qfile_name);
      req->file = fopen (req->destination_path, "wb");
      if (!req->file)
        {
          xfree (req->destination_path);
          xfree (req);
          curl_easy_cleanup (easy);
          signal_error (Qfile_error, "Cannot open HTTP output file",
                        destination);
        }
    }
  else
    req->response_buf = build_ascstring ("");

  curl_easy_setopt (easy, CURLOPT_URL,            url);
  curl_easy_setopt (easy, CURLOPT_WRITEFUNCTION,  http_write_callback);
  curl_easy_setopt (easy, CURLOPT_WRITEDATA,      req);
  curl_easy_setopt (easy, CURLOPT_HEADERFUNCTION, http_header_callback);
  curl_easy_setopt (easy, CURLOPT_HEADERDATA,     req);
  curl_easy_setopt (easy, CURLOPT_PRIVATE,        req);
  curl_easy_setopt (easy, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt (easy, CURLOPT_MAXREDIRS,      ASYNC_HTTP_MAX_REDIRS);

  if (timeout_ms > 0)
    {
      curl_easy_setopt (easy, CURLOPT_TIMEOUT_MS, (long) timeout_ms);
      if ((long) timeout_ms < connect_timeout_ms)
        connect_timeout_ms = (long) timeout_ms;
    }
  curl_easy_setopt (easy, CURLOPT_CONNECTTIMEOUT_MS, connect_timeout_ms);

  if (strcmp (method, "POST") == 0)
    curl_easy_setopt (easy, CURLOPT_POST, 1L);
  else if (strcmp (method, "PUT") == 0)
    curl_easy_setopt (easy, CURLOPT_UPLOAD, 1L);

  if (!NILP (req_headers_alist))
    {
      EXTERNAL_LIST_LOOP_2 (elt, req_headers_alist)
        {
          char hdr[512];
          Lisp_Object name = XCAR (elt);
          Lisp_Object val  = XCDR (elt);
          snprintf (hdr, sizeof (hdr), "%s: %s",
                    (char *) XSTRING_DATA (name),
                    (char *) XSTRING_DATA (val));
          req->req_headers = curl_slist_append (req->req_headers, hdr);
        }
      curl_easy_setopt (easy, CURLOPT_HTTPHEADER, req->req_headers);
    }

  if (!NILP (body_string))
    {
      curl_easy_setopt (easy, CURLOPT_POSTFIELDSIZE,
                        (long) XSTRING_LENGTH (body_string));
      curl_easy_setopt (easy, CURLOPT_COPYPOSTFIELDS,
                        (char *) XSTRING_DATA (body_string));
    }

  curl_multi_add_handle (curl_multi, easy);

  /* Suspend the calling coroutine until the transfer completes */
  if (req->waiting_coro)
    coro_yield (WAIT_FD);

  /* If we reach here after being resumed, the result is in the coroutine's
     result field — the caller (async-core.el await) retrieves it. */
  return Qnil;
}

/* ---- Lisp interface ---- */

DEFUN ("async-http-request-internal", Fasync_http_request_internal,
       2, 7, 0, /*
Internal: start an async HTTP request; suspend coroutine until done.
URL is a string.  METHOD is \"GET\" or \"POST\".
Optional: HEADERS (alist of (name . value) strings), BODY (string),
ON-CHUNK (function called for each response chunk), TIMEOUT-MS (integer),
DESTINATION (file name).  When DESTINATION is non-nil the response body is
written directly to that file by C and the result's third element is the
file name instead of the body string.  A partial file is removed when the
transfer fails.  The TCP connect phase is bounded by 10 seconds, or by
TIMEOUT-MS when that is shorter; TIMEOUT-MS also remains the total limit.
*/
       (url, method, headers, body, on_chunk, timeout_ms, destination))
{
  CHECK_STRING (url);
  CHECK_STRING (method);
  if (!NILP (destination))
    CHECK_STRING (destination);
  int tms = NILP (timeout_ms) ? 0 : XFIXNUM (timeout_ms);
  return async_http_start ((const char *) XSTRING_DATA (url),
                           (const char *) XSTRING_DATA (method),
                           headers, body, on_chunk, tms, destination);
}

void
syms_of_async_http (void)
{
  DEFSYMBOL (Qasync_http_error);
  DEFSUBR (Fasync_http_request_internal);
}

void
vars_of_async_http (void)
{
}

void
init_async_http (void)
{
  curl_global_init (CURL_GLOBAL_DEFAULT);
  curl_multi = curl_multi_init ();
  curl_multi_setopt (curl_multi, CURLMOPT_SOCKETFUNCTION, async_http_socket_cb);
  curl_multi_setopt (curl_multi, CURLMOPT_TIMERFUNCTION,  curl_timer_callback);
}

#endif /* HAVE_LIBCURL */
