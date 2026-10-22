# HTTPS download path after the async merge

Research for wayfinder ticket [#17](https://github.com/fktpp2022/xemacs-git/issues/17), 2026-10-22. References to XEmacs are paths at `origin/xemacs-cmake` commit `8e6ec57c6` (which contains the async merge `48bd28ddc`). The probes below ran against the CMake build in `build-verify-merge` with system libcurl 7.81.0 / OpenSSL 3.0.2.

## 1. What the async branch provides

The merged async branch supplies a complete HTTP client in C, driven by the coroutine scheduler:

- `async-http-request-internal` starts a libcurl easy handle on a shared multi handle, registers the request's completion with the current coroutine, and yields with `WAIT_FD`. On completion it resumes the coroutine with `(STATUS HEADERS BODY)`, or resumes with `(async-http-error STRING)` on a `CURLcode` failure ([src/async-http.c:175-239](src/async-http.c), [src/async-http.c:100-151](src/async-http.c)).
- Redirects are always followed (`CURLOPT_FOLLOWLOCATION`) and an optional total timeout maps to `CURLOPT_TIMEOUT_MS` ([src/async-http.c:197-200](src/async-http.c)).
- The scheduler polls a `WAIT_FD` coroutine every 10 ms and calls `async_http_tick()`, which drives `curl_multi_perform`; in interactive TTY mode socket-ready callbacks can drive it sooner ([src/async-scheduler.c:518-532](src/async-scheduler.c), [src/async-scheduler.c:726-742](src/async-scheduler.c), [src/event-unixoid.c:364-378](src/event-unixoid.c)).
- Lisp wrappers are `http-get`, `http-post`, `http-stream`, and the `await` macro ([lisp/async-http.el:6-32](lisp/async-http.el), [lisp/async-core.el:5-46](lisp/async-core.el)). They must be called inside a coroutine: outside one, `async_http_start` has no coroutine to suspend and the call returns `nil` immediately without yielding a result ([src/async-http.c:189](src/async-http.c), [src/async-http.c:233-234](src/async-http.c)).

There is no synchronous HTTP client in core; the existing add-on `url` package builds on `network-stream-open-tls`, not on this code.

## 2. End-to-end probe

A probe program (coroutine + manual `async-scheduler-tick` loop, the same pattern as `tests/automated/async-http-tests.el`) was run against live GNU ELPA and httpbin:

| Case | Request | Result |
|---|---|---|
| index over HTTPS | `GET https://elpa.gnu.org/packages/archive-contents` | 200, 189,316 bytes (matches `Content-Length`) |
| tarball over HTTPS | `GET https://elpa.gnu.org/packages/seq-2.24.tar` | 200, 71,680 bytes |
| tarball integrity | write body to disk with `coding-system-for-write 'binary`, hash | MD5 `0d6f1228ae599ea4ce1e91f5297e5077` — identical to `curl` |
| redirects | `GET https://httpbin.org/redirect/2` | 200, body from final URL |
| redirect limit | `GET https://httpbin.org/redirect/31` | 200 — **no limit enforced** |
| timeout | `GET https://httpbin.org/delay/5` with `:timeout 1500` | `async-http-error "Timeout was reached"` after 1.52 s |
| HTTP error | `GET https://elpa.gnu.org/packages/no-such-file-xyz` | status 404, 236-byte body, no signal |
| TLS rejection | `GET https://127.0.0.1:18443/` (self-signed) | `async-http-error "SSL peer certificate or SSH remote key was not OK"` |
| concurrency | two GETs in one `async-let` | statuses `(200 200)` |
| certificate trust | ELPA HTTPS succeeded with system CA store | no override of `CURLOPT_SSL_VERIFYPEER`/`VERIFYHOST`; defaults are secure |

TLS, redirects, timeout, status handling, and concurrency therefore work as-is. Binary delivery does **not**.

## 3. Critical gap: the body accumulator corrupts or crashes on non-ASCII bytes

`http_write_callback` builds each chunk with `make_string`, which by contract requires memory already in XEmacs **internal** encoding ([src/async-http.c:23-33](src/async-http.c), [src/alloc.c:3068-3089](src/alloc.c)). libcurl delivers arbitrary external bytes: UTF-8 text, tarballs, compressed data. UTF-8 bytes such as `0xC3 0xA3` are not valid internal Mule lead bytes.

The unpatched probe fetching `archive-contents` (which contains e.g. `João Távora`) aborts XEmacs:

```
Fatal error: assertion failed, file src/text.c, line 2777, fb >= 0 && fb < 0xA0
```

The failing path is `concat2` → `bytecount_to_charcount_fun` → `rep_bytes_by_first_byte` ([src/text.c:2757-2791](src/text.c)). A tarball would hit the same path on its first high byte. This makes the current HTTP body API unusable for the ELPA download path, even for text archives.

**Verified minimal fix.** Convert external bytes with the binary coding system instead of assuming internal format:

```c
Lisp_Object chunk = make_extstring ((Extbyte *) ptr, (EMACS_INT) nbytes, Qbinary);
```

With that one-line change in both branches of `http_write_callback`, every probe above passed, and the tarball's on-disk MD5 matched `curl` byte-for-byte. `make_extstring(..., Qbinary)` is the documented external-to-Lisp constructor ([src/alloc.c:3091-3102](src/alloc.c)); `insert-file-contents-literally` already uses the same raw-byte convention elsewhere in XEmacs ([lisp/package-get.el:1097-1098](lisp/package-get.el)).

Two consequences follow:

- After the fix, a downloaded body is a byte-preserving Lisp string. Decode `archive-contents` explicitly as UTF-8 before `read`; write tarballs with `coding-system-for-write 'binary` (verified) or, preferably, stream them to a file in C (below).
- Header names/values are also built with `make_string` ([src/async-http.c:40-62](src/async-http.c)). ASCII headers are safe, but non-ASCII or HTTP/2 lowercase header values deserve the same `Qbinary` treatment. The parser also does not parse the status line and is case-sensitive, so HTTP/2's lowercase names require `assoc-string`-style lookup.

## 4. TLS and composition with `network-stream-open-tls`

The two TLS stacks are independent and should stay that way:

- `async-http` uses libcurl's OpenSSL backend and its default system CA store. Verification is on by default; the self-signed probe failed closed, and GNU ELPA validated. There is no Lisp-level CA/proxy/client-certificate configuration in `async-http`; libcurl honors `http_proxy`/`https_proxy`/`no_proxy`, and OpenSSL honors `SSL_CERT_FILE`/`SSL_CERT_DIR`. (`CURL_CA_BUNDLE` is a curl-tool variable, not a libcurl one.)
- `network-stream-open-tls` is a process/stream abstraction: it opens a network stream and calls the C `tls-negotiate`, falling back to `gnutls-cli`/`openssl s_client`, returning a stream plus greeting/capabilities ([lisp/code-process.el:311-324](lisp/code-process.el), [src/process.c:989](src/process.c)). It does not speak HTTP and is not reusable for package downloads.
- The add-on `url` package (in the `w3` package) layers HTTP over that stream abstraction. It remains unavailable in core and is not on the ELPA path.

Choose one trust configuration for both stacks eventually, but do not route libcurl through `network-stream-open-tls`. No certificate pinning or client certificates are needed for GNU ELPA.

## 5. Redirects, timeouts, cancellation

- **Redirects:** followed unconditionally, with no `CURLOPT_MAXREDIRS` and no protocol restriction. The 31-hop probe succeeded, so a redirect loop with no `:timeout` can run indefinitely. Set an explicit maximum (curl's own CLI default is 50) and keep the default timeout non-zero at the package layer.
- **Timeouts:** `:timeout` is a whole-transfer `CURLOPT_TIMEOUT_MS`. There is no connect timeout, no low-speed abort, and no per-operation timeout. `timeout_ms = 0` (the default) means no timeout at all ([src/async-http.c:199-200](src/async-http.c)).
- **DNS:** the system libcurl reports no `AsynchDNS` (`curl -V`), so name resolution runs synchronously inside `curl_multi_perform` and can stall the event loop for the resolver's timeout. This is a UI-latency issue, not a data-correctness issue; a threaded-resolver libcurl avoids it.
- **Cancellation:** there is no abort API for an in-flight request. Closing a buffer or quitting cannot cancel a download; only the timeout ends it.
- **Errors:** transport failures become `async-http-error` carrying only `curl_easy_strerror`. The curl code, URL, and status are lost. HTTP 4xx/5xx are ordinary results, not errors; callers must inspect status. The installer needs the status and URL for a useful message.
- **Chunk streaming:** `:on-chunk` dispatches through `enqueue_misc_user_event`, which only drains in the interactive command loop; the async HTTP tests explicitly skip an `:on-chunk` test in batch for this reason ([tests/automated/async-http-tests.el:205-210](tests/automated/async-http-tests.el)). Do not use `:on-chunk` for the download path in batch. `http-stream` also calls `http-post`, so it sends POST, not GET ([lisp/async-http.el:30-32](lisp/async-http.el)).
- **Memory:** the accumulator concatenates every chunk with `concat2`, which copies the growing body each time — O(n²) work and garbage. Fine for the 189 KB index and 70 KB tarball; multi-megabyte packages amplify it.

## 6. Downloading the ELPA artifacts

GNU ELPA publishes, for an archive base URL (e.g. `https://elpa.gnu.org/packages/`):

- `<base>archive-contents` — UTF-8 text, no `Content-Type` header, ~189 KB today; the version-1 alist parsed by `package.el`.
- `<base><name>-<version>.tar` for `kind` `tar`, or `<base><name>-<version>.el` for `kind` `single`; GNU ELPA serves plain uncompressed tar, not `tar.gz`.
- `<base>archive-contents.sig` and `<base><name>-<version>.tar.sig` — detached signatures, both present today.

XEmacs's existing installer expects a different artifact: `<pkg>-<version>-pkg.tar.gz`, downloaded via an EFS/ange-ftp path and `copy-file`, validated against an MD5 carried by the index, then unpacked with `gunzip -c | tar xvf -` ([lisp/package-get.el:1069-1078](lisp/package-get.el), [lisp/package-get.el:1094-1106](lisp/package-get.el), [lisp/package-admin.el:123-135](lisp/package-admin.el)). GNU ELPA has no MD5 in `archive-contents`; integrity comes from the `.sig` files. So the download layer must hand off a staged file plus status/headers, not a precompiled XEmacs package tarball.

## 7. Smallest reliable path

1. **Fix the accumulator first** (one line): `make_extstring(..., Qbinary)` in both `http_write_callback` branches. Without it, every non-ASCII index or tarball is a crash. Add a regression test that fetches a body containing a UTF-8 name and a binary blob.
2. **Add a file sink to the same C module** for package artifacts: a `:file PATH` / `async-http-download` option that `fopen`s the path and `fwrite`s each libcurl chunk, closing it on completion/error. This avoids the in-memory accumulator entirely, removes the O(n²) copy, keeps bytes exact without Lisp coding-system care, and produces the staging file the installer and MD5/signature checks want. Use it for tarballs; use the fixed in-memory path (or the same sink) for `archive-contents`.
3. **Give the request sane defaults** at the package layer: explicit `MAXREDIRS`, a non-zero default total timeout (e.g. 30 s for metadata, 300 s for tarballs), and a connect timeout. Add `CURLOPT_ACCEPT_ENCODING` only if an archive compresses responses.
4. **Read metadata correctly**: fetch `archive-contents`, insert literally, decode UTF-8, `read` the version-1 alist. Cache it per archive as `package.el` does.
5. **Check status before trusting the body**: 404/500 are results, not errors. Extend `async-http-error` (or return the curl code) so the caller can distinguish DNS, TLS, timeout, and HTTP failures.
6. **Verify integrity outside the transport**: MD5 is unavailable for ELPA; use `gpg --verify` on the `.sig` when signature policy requires it. That decision belongs to the install strategy ticket, not the HTTP layer.
7. **Run downloads inside a coroutine**: the interactive event loop drives `WAIT_FD` ticks automatically at 10 ms; batch callers must run `async-scheduler-tick` in a loop, as the automated tests do.

This is enough to fetch and stage a GNU ELPA package over HTTPS without touching `network-stream-open-tls`, EFS, or the add-on `url` package. Extraction, autoload generation, byte-compilation, and activation are the install strategy's problem.

## 8. Gaps the async branch does not cover

- External-byte construction in the body/header callbacks (critical; fix above).
- Streaming to a file; the API is memory-only.
- Redirect cap, protocol restriction, connect timeout, low-speed abort, cancellation.
- Retry/backoff and `Retry-After`; not needed for a first install but relevant to refresh reliability.
- Structured errors (curl code, URL, HTTP status).
- CA/proxy/client-certificate configuration exposed to Lisp.
- Asynchronous DNS (system libcurl limitation).
- Decompression/`Accept-Encoding` and a configurable User-Agent (some mirrors and GitHub may care; GNU ELPA does not).
- Signature verification (by design; belongs to the install layer).
- A batch-safe chunk callback (current `:on-chunk` needs the command loop).
- Windows support: `src/CMakeLists.txt` only compiles the async sources on Unix and hard-requires CURL.

## 9. What this settles for the map

- The HTTPS transport for the first ELPA milestone is **`async-http` + libcurl**, not `url-retrieve`, EFS, or `network-stream-open-tls`. No new transport dependency is needed.
- Before any installer work, the async HTTP body path needs the binary-safety fix and, preferably, a file sink. That is a small, well-scoped C change in `src/async-http.c`, not a new subsystem.
- The download contract for the installer is: a staged file on disk, HTTP status, and a transport error that distinguishes failure classes. Index parsing then reads the staged file as UTF-8; tarball integrity is checked outside the transport, by signature (and MD5 only where an index supplies it).
