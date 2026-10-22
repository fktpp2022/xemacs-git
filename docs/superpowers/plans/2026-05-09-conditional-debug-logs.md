# Conditional Compile Debug Logs Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Wrap all stderr debug logs with DEBUG_XEMACS conditional compilation for best performance in release builds

**Architecture:** Use project's existing DEBUG_XEMACS macro (controlled by CMake option XEMACS_WITH_DEBUG) to conditionally compile debug logs. Create ASYNC_DEBUG macro for cleaner code.

**Tech Stack:** C, CMake, XEmacs build system

---

## File Structure

| File | Responsibility |
|------|----------------|
| `src/async-scheduler.c` | Add ASYNC_DEBUG macro, replace 11 stderr_out calls |
| `src/event-stream.c` | Add EVENT_DEBUG macro, replace 7 stderr_out calls |
| `src/event-Xt.c` | Replace 1 stderr_out call with DEBUG_XEMACS guard |

---

### Task 1: Wrap async-scheduler.c debug logs

**Files:**
- Modify: `src/async-scheduler.c`

**Steps:**

- [ ] **Step 1: Add ASYNC_DEBUG macro near top of file**

After the includes and before any function definitions, add:

```c
#ifdef DEBUG_XEMACS
#define ASYNC_DEBUG(...) stderr_out (__VA_ARGS__)
#else
#define ASYNC_DEBUG(...) ((void)0)
#endif
```

- [ ] **Step 2: Replace [coro-spawn] log (line ~377)**

```c
// Before:
stderr_out ("[coro-spawn] new coro=%p fn=%p\n", (void *)c, (void *)fn);

// After:
ASYNC_DEBUG ("[coro-spawn] new coro=%p fn=%p\n", (void *)c, (void *)fn);
```

- [ ] **Step 3: Replace [coro-yield] log (line ~485)**

```c
// Before:
stderr_out ("[coro-yield] coro=%p reason=%d\n", (void *)c, reason);

// After:
ASYNC_DEBUG ("[coro-yield] coro=%p reason=%d\n", (void *)c, reason);
```

- [ ] **Step 4: Replace [async-tick] unforbid log (line ~587)**

```c
// Before:
stderr_out ("[async-tick] unforbid: forbidden=%d\n", async_tick_forbidden);

// After:
ASYNC_DEBUG ("[async-tick] unforbid: forbidden=%d\n", async_tick_forbidden);
```

- [ ] **Step 5: Replace [async-tick] forbid_start log (line ~595)**

```c
// Before:
stderr_out ("[async-tick] forbid_start: forbidden=%d\n", async_tick_forbidden);

// After:
ASYNC_DEBUG ("[async-tick] forbid_start: forbidden=%d\n", async_tick_forbidden);
```

- [ ] **Step 6: Replace [async-tick] BLOCKED log (line ~608)**

```c
// Before:
stderr_out ("[async-tick] BLOCKED: in_tick=%d forbidden=%d coros=%d\n",
            in_tick, async_tick_forbidden, coro_count);

// After:
ASYNC_DEBUG ("[async-tick] BLOCKED: in_tick=%d forbidden=%d coros=%d\n",
             in_tick, async_tick_forbidden, coro_count);
```

- [ ] **Step 7: Replace [async-tick] Phase 2 log (line ~637)**

```c
// Before:
stderr_out ("[async-tick] Phase 2: checking %d coros\n", coro_count);

// After:
ASYNC_DEBUG ("[async-tick] Phase 2: checking %d coros\n", coro_count);
```

- [ ] **Step 8: Replace [async-tick] skip log (line ~643)**

```c
// Before:
stderr_out ("[async-tick]   coro %p state=%d wait=%d (skip)\n",
            (void *)c, c->state, c->wait_reason);

// After:
ASYNC_DEBUG ("[async-tick]   coro %p state=%d wait=%d (skip)\n",
             (void *)c, c->state, c->wait_reason);
```

- [ ] **Step 9: Replace [async-tick] RUNNABLE log (line ~647)**

```c
// Before:
stderr_out ("[async-tick]   coro %p RUNNABLE -> running\n", (void *)c);

// After:
ASYNC_DEBUG ("[async-tick]   coro %p RUNNABLE -> running\n", (void *)c);
```

- [ ] **Step 10: Replace [async-tick] returned log (line ~686)**

```c
// Before:
stderr_out ("[async-tick] coro %p returned, state=%d\n", (void *)c, c->state);

// After:
ASYNC_DEBUG ("[async-tick] coro %p returned, state=%d\n", (void *)c, c->state);
```

- [ ] **Step 11: Replace [async-wakeup] arming log (line ~778)**

```c
// Before:
stderr_out ("[async-wakeup] arming wakeup: %u ms, forbidden=%d\n",
            milliseconds, async_tick_forbidden);

// After:
ASYNC_DEBUG ("[async-wakeup] arming wakeup: %u ms, forbidden=%d\n",
             milliseconds, async_tick_forbidden);
```

- [ ] **Step 12: Replace [async-timer-cb] log (line ~796)**

```c
// Before:
stderr_out ("[async-timer-cb] tick-from-timer called, forbidden=%d\n",
            async_tick_forbidden);

// After:
ASYNC_DEBUG ("[async-timer-cb] tick-from-timer called, forbidden=%d\n",
             async_tick_forbidden);
```

- [ ] **Step 13: Compile to verify**

Run: `cmake --build build --parallel 4 2>&1 | tail -20`
Expected: Build successful, no errors

---

### Task 2: Wrap event-stream.c debug logs

**Files:**
- Modify: `src/event-stream.c`

**Steps:**

- [ ] **Step 1: Add EVENT_DEBUG macro near top of file**

After the includes and before any function definitions (or in the same area as other DEBUG_XEMACS code), add:

```c
#ifdef DEBUG_XEMACS
#define EVENT_DEBUG(...) stderr_out (__VA_ARGS__)
#else
#define EVENT_DEBUG(...) ((void)0)
#endif
```

Note: Check if there's already a similar pattern in the file around line 278 where DEBUG_XEMACS is used.

- [ ] **Step 2: Replace [event-wakeup] generate log (line ~1129)**

```c
// Before:
stderr_out ("[event-wakeup] generate: id=%ld interval_id=%ld function=%p object=%p async=%d\n",
            (long)timeout->id, (long)timeout->interval_id,
            (void *)timeout->function, (void *)timeout->object, async_p);

// After:
EVENT_DEBUG ("[event-wakeup] generate: id=%ld interval_id=%ld function=%p object=%p async=%d\n",
             (long)timeout->id, (long)timeout->interval_id,
             (void *)timeout->function, (void *)timeout->object, async_p);
```

- [ ] **Step 3: Replace [event-wakeup] resignal log (line ~1165)**

```c
// Before:
stderr_out ("[event-wakeup] resignal: interval_id=%ld async=%d\n",
            (long)interval_id, async_p);

// After:
EVENT_DEBUG ("[event-wakeup] resignal: interval_id=%ld async=%d\n",
             (long)interval_id, async_p);
```

- [ ] **Step 4: Replace [event-wakeup] checking log (line ~1170)**

```c
// Before:
stderr_out ("[event-wakeup]   checking: id=%ld interval_id=%ld match=%d\n",
            (long)timeout->id, (long)timeout->interval_id,
            timeout->interval_id == interval_id);

// After:
EVENT_DEBUG ("[event-wakeup]   checking: id=%ld interval_id=%ld match=%d\n",
             (long)timeout->id, (long)timeout->interval_id,
             timeout->interval_id == interval_id);
```

- [ ] **Step 5: Replace [event-wakeup] ERROR log (line ~1178)**

```c
// Before:
stderr_out ("[event-wakeup]   ERROR: no match found!\n");

// After:
EVENT_DEBUG ("[event-wakeup]   ERROR: no match found!\n");
```

- [ ] **Step 6: Replace [event-wakeup] found log (line ~1188)**

```c
// Before:
stderr_out ("[event-wakeup]   found: id=%ld function=%p object=%p\n",
            (long)id, (void *)*function, (void *)*object);

// After:
EVENT_DEBUG ("[event-wakeup]   found: id=%ld function=%p object=%p\n",
             (long)id, (void *)*function, (void *)*object);
```

- [ ] **Step 7: Replace [next-event] returning log (line ~2172)**

```c
// Before:
stderr_out ("[next-event] returning: event_type=%d forbidden=%d\n",
            EVENT_TYPE (XEVENT (target_event)), async_tick_forbidden);

// After:
EVENT_DEBUG ("[next-event] returning: event_type=%d forbidden=%d\n",
             EVENT_TYPE (XEVENT (target_event)), async_tick_forbidden);
```

- [ ] **Step 8: Replace [execute-event] timeout_event log (line ~3237)**

```c
// Before:
stderr_out ("[execute-event] timeout_event: function=%p object=%p\n",
            (void *)EVENT_TIMEOUT_FUNCTION (e),
            (void *)EVENT_TIMEOUT_OBJECT (e));

// After:
EVENT_DEBUG ("[execute-event] timeout_event: function=%p object=%p\n",
             (void *)EVENT_TIMEOUT_FUNCTION (e),
             (void *)EVENT_TIMEOUT_OBJECT (e));
```

- [ ] **Step 9: Compile to verify**

Run: `cmake --build build --parallel 4 2>&1 | tail -20`
Expected: Build successful, no errors

---

### Task 3: Wrap event-Xt.c debug log

**Files:**
- Modify: `src/event-Xt.c`

**Steps:**

- [ ] **Step 1: Wrap [Xt-timeout] log with DEBUG_XEMACS (line ~3098)**

This file already uses DEBUG_XEMACS pattern (around line 103 and 1982). Follow the same pattern:

```c
// Before:
stderr_out ("[Xt-timeout] to_emacs_event: timeout=%p interval_id=%ld\n",
            (void *)timeout, (long)interval_id);

// After:
#ifdef DEBUG_XEMACS
  stderr_out ("[Xt-timeout] to_emacs_event: timeout=%p interval_id=%ld\n",
              (void *)timeout, (long)interval_id);
#endif
```

Note: The log is inside a function. Use `#ifdef DEBUG_XEMACS` directly since this is a single log statement in a file that already uses this pattern.

- [ ] **Step 2: Compile to verify**

Run: `cmake --build build --parallel 4 2>&1 | tail -20`
Expected: Build successful, no errors

---

### Task 4: Verify release build has no debug logs

**Files:**
- None (build verification)

**Steps:**

- [ ] **Step 1: Configure with debug disabled**

Run: `cmake -S . -B build-release -DXEMACS_WITH_DEBUG=OFF 2>&1 | tail -20`
Expected: CMake configures successfully

- [ ] **Step 2: Build release version**

Run: `cmake --build build-release --parallel 4 2>&1 | tail -20`
Expected: Build successful

- [ ] **Step 3: Verify debug symbols/logs are not present**

Run: `grep -c "async-tick\|coro-spawn\|event-wakeup" build-release/bin/xemacs 2>/dev/null || echo "No debug strings found (expected)"`
Expected: "No debug strings found (expected)" or 0 matches

- [ ] **Step 4: Clean up release build (optional)**

Run: `rm -rf build-release`

---

### Task 5: Final verification

**Files:**
- None (functional verification)

**Steps:**

- [ ] **Step 1: Build with debug enabled (default)**

Run: `cmake --build build --parallel 4 2>&1 | tail -20`
Expected: Build successful

- [ ] **Step 2: Quick sanity test**

Run: `./build/bin/xemacs -batch -q -l lisp/actor.el -l lisp/async-core.el --eval '(message "OK")' 2>&1`
Expected: Output includes "OK" and no errors

---

## Self-Review

**1. Spec coverage:**
- ✅ All 19 debug log statements identified and planned for wrapping
- ✅ Uses project's existing DEBUG_XEMACS mechanism
- ✅ Release build has zero runtime overhead
- ✅ Follows existing code patterns in the project

**2. Placeholder scan:**
- ✅ No TBD/TODO placeholders
- ✅ All code snippets are complete
- ✅ All file paths and line numbers are specific

**3. Type consistency:**
- ✅ ASYNC_DEBUG and EVENT_DEBUG macros follow same pattern
- ✅ All log format strings preserved exactly
- ✅ DEBUG_XEMACS usage consistent with project conventions
