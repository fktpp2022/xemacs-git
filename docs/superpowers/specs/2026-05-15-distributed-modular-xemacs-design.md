
# Distributed Modular XEmacs Design Specification

**Date:** 2026-05-15
**Status:** Draft (Awaiting Review)
**Author:** XEmacs Architecture Team

---

## 1. Executive Summary

This document specifies the architecture to transform XEmacs from a monolithic application into a modular, distributed editor with independent processes communicating via MessagePack-RPC.

### Goals
- Transform monolithic XEmacs into independent modules
- Support multiple UX engines (TTY, X11, GTK, macOS, etc.)
- Add built-in LSP, Tree-sitter, and ACP support
- Enable distributed deployment (co-located file editors)
- Maintain backward compatibility (TRAMP for slow connections)

---

## 2. High-Level Architecture

### System Overview
```
┌─────────────────────────────────────────────────────────────────┐
│                        UX Layer (Processes)                       │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐          │
│  │ X11 GUI  │  │  TTY     │  │  GTK GUI │  │  macOS   │          │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘          │
└───────┼─────────────┼─────────────┼─────────────┼──────────────────┘
        │             │             │             │
        └──────────────┴──────┬──────┴─────────────┘
                             │
                      MessagePack-RPC
                      (Unix Domain / TCP)
                             │
┌────────────────────────────┼─────────────────────────────────────┐
│                             ▼                                     │
│              ┌───────────────────────────────┐                   │
│              │      Async I/O Core           │                   │
│              │  ┌─────────────────────────┐  │                   │
│              │  │ Event Loop (libuv)      │  │                   │
│              │  │ File Watching (inotify) │  │                   │
│              │  │ Process Management      │  │                   │
│              │  │ RPC Server/Client       │  │                   │
│              │  │ Service Discovery       │  │                   │
│              │  └─────────────────────────┘  │                   │
│              └───────────────┬───────────────┘                   │
│                             │                                     │
│         ┌───────────────────┼───────────────────┐                 │
│         ▼                   ▼                   ▼                 │
│  ┌─────────────┐   ┌─────────────┐   ┌───────────────────┐        │
│  │  Elisp      │   │  Buffer/    │   │  LSP/Tree-sitter/ │        │
│  │  Engine     │   │  File Mgr   │   │  ACP Core         │        │
│  └─────────────┘   └─────────────┘   └───────────────────┘        │
└───────────────────────────────────────────────────────────────────┘
```

### Communication
- **Protocol:** MessagePack-RPC (schema-free, dynamic)
- **Local:** Unix Domain Sockets (UDS)
- **Remote:** TCP/IP (TLS optional)
- **Topology:** Async Core is central hub

---

## 3. Module Specifications

### 3.1 Async I/O Core (`xemacs-async`)
**Current Equivalent:** `src/event-stream.c`, `src/process.c`

**Purpose:**
- Central event coordination hub (first process to start)
- Manages all asynchronous operations
- Accepts UX connections and forwards requests

**Key Responsibilities:**
1. **Event Loop:** libuv-based event loop
2. **RPC Server:** Accepts connections from UX modules
3. **RPC Client:** Connects to Elisp, Buffer, LSP modules
4. **Process Management:** Spawns, monitors, terminates other modules
5. **File Watching:** Inotify/FSEvents integration
6. **Timer Management:** Event-driven timers

**APIs (MessagePack-RPC):**
- `async.register_ux(type, connection_info)`
- `async.unregister_ux(module_id)`
- `async.subscribe_event(event_name, callback)`
- `async.request(module_id, method, params)`
- `async.notify(event_name, data)`

**Build & Deployment:**
- Independent `configure` and `Makefile`
- No GUI dependencies
- Runs as daemon or foreground process

---

### 3.2 Elisp Engine (`xemacs-elisp`)
**Current Equivalent:** `src/eval.c`, `src/bytecode.c`, `src/symbols.c`, etc.

**Purpose:**
- Execute all Elisp code (interpreter, compiler)
- Maintain Elisp state (symbols, variables, buffers)
- No direct I/O - delegates to buffer module

**Key Responsibilities:**
1. **Lisp Interpreter:** Execute Elisp code
2. **Bytecode Compiler:** Compile Elisp to bytecode
3. **Package Management:** Load, install packages
4. **Buffer Coordination:** Delegate file operations to `xemacs-buffer`
5. **UX Rendering:** Delegate display to UX modules via async core

**APIs (MessagePack-RPC):**
- `elisp.eval(code)` - evaluate Elisp code string
- `elisp.eval_sexp(sexp)` - evaluate Elisp S-expression
- `elisp.load(filename)` - load Elisp file
- `elisp.call(function_name, args)` - call Elisp function
- `elisp.get_variable(var_name)` - get Elisp variable
- `elisp.set_variable(var_name, value)` - set Elisp variable

**APIs (Elisp - exposed to users):**
- Existing Elisp API maintained for backward compatibility
- New functions for RPC-aware operations (e.g., `buffer-open-remote`)

**Build & Deployment:**
- Independent `configure` and `Makefile`
- Uses `libmsgpack` for RPC
- Can be started by async core or manually

---

### 3.3 Buffer/File Manager (`xemacs-buffer`)
**Current Equivalent:** `src/buffer.c`, `src/fileio.c`, `src/filelock.c`

**Purpose:**
- Manage all file operations and buffers
- File watching, remote file editing
- Co-located on same machine as target file for fast access

**Key Responsibilities:**
1. **File I/O:** Read, write files
2. **Buffer Management:** Buffer creation, modification
3. **File Watching:** Monitor for external changes
4. **Remote Files:** Co-located RPC-based editing (fast connections)
5. **File Locking:** Coordinate multi-user access

**Remote File Strategy:**
- **Fast connections:** Co-located `xemacs-buffer` on remote host
- **Slow connections:** Fall back to TRAMP package (unchanged)

**APIs (MessagePack-RPC):**
- `buffer.open(filepath, options)` - open/create buffer
- `buffer.read(filepath, options)` - read file into buffer
- `buffer.write(buffer_id, filepath)` - write buffer to file
- `buffer.get_content(buffer_id)` - get buffer content
- `buffer.set_content(buffer_id, content)` - set buffer content
- `buffer.watch(filepath, callback)` - start watching file
- `buffer.unwatch(filepath)` - stop watching file
- `buffer.remote_open(host, filepath, options)` - open file remotely

**Build & Deployment:**
- Independent `configure` and `Makefile`
- Can be deployed on remote hosts
- Supports multiple concurrent instances

---

### 3.4 LSP/Tree-sitter/ACP Core (`xemacs-lang`)
**Current Equivalent:** None (NEW module)

**Purpose:**
- Language Server Protocol (LSP) client
- Tree-sitter integration for syntax highlighting, parsing
- ACP (Advanced Code Processing) protocol support (future)

**Key Responsibilities:**
1. **LSP Client:** Connect to LSP servers, handle protocol
2. **Tree-sitter:** Fast syntax tree parsing and analysis
3. **ACP:** Advanced code analysis (future)

**APIs (MessagePack-RPC):**
- `lsp.start_server(language, config)`
- `lsp.text_document_did_open(buffer_id, filepath, lang)`
- `lsp.text_document_did_change(buffer_id, version, changes)`
- `lsp.text_document_hover(buffer_id, position)`
- `lsp.text_document_completion(buffer_id, position)`
- `treesitter.parse(buffer_id, language)` - return syntax tree
- `treesitter.query(buffer_id, query_string)` - query AST
- `treesitter.set_grammar(grammar_name, grammar_data)`

**Build & Deployment:**
- Independent `configure` and `Makefile`
- Links to tree-sitter (Rust), LSP client libraries
- Can run on separate core/thread for performance

---

### 3.5 UX Modules
**Current Equivalent:** `src/console-tty.c`, `src/console-x.c`, `src/console-gtk.c`, etc.

**Available Modules:**
- `xemacs-tty`: TTY/console UX
- `xemacs-gtk`: GTK 3+ GUI UX
- `xemacs-x11`: X11 GUI UX
- `xemacs-msw`: Windows GUI UX
- `xemacs-cocoa`: macOS GUI UX

**Common Purpose:**
- Render editor UI (display)
- Handle user input (keyboard, mouse)
- Communicate with async core via RPC

**APIs (UX → Async Core):**
- `ux.register(module_type, connection_info)`
- `ux.input(key_event, mouse_event)`
- `ux.command(command_name, args)`
- `ux.render_complete()`

**APIs (Async Core → UX):**
- `ux.render(display_data)`
- `ux.update_buffer(buffer_id, content, metadata)`
- `ux.notify(message, type)`
- `ux.quit()`

**Build & Deployment:**
- Each UX module has independent `configure` and `Makefile`
- Each links to required GUI libraries (GTK, X11, etc.)
- Multiple UX modules can connect to same async core

---

## 4. MessagePack-RPC Protocol

### 4.1 Message Format

#### Request
```python
{
  "type": "request",
  "id": 12345,  # Unique request ID
  "method": "buffer.get",
  "params": {
    "buffer_id": "abc123",
    "start_line": 1,
    "end_line": 100
  }
}
```

#### Response
```python
{
  "type": "response",
  "id": 12345,
  "result": {
    "content": "def hello(): ...",
    "lines": ["def hello():", "    return 'hi'"],
    "metadata": {"modified": false, "encoding": "utf-8"}
  },
  "error": null
}
```

#### Notification (No Response)
```python
{
  "type": "notification",
  "method": "buffer.changed",
  "params": {
    "buffer_id": "abc123",
    "changes": [{"start": 10, "end": 15, "text": "new content"}]
  }
}
```

### 4.2 Core Method Registry

#### Async Core Methods
| Method | Description |
|--------|-------------|
| `async.register_ux` | Register a UX module |
| `async.unregister_ux` | Unregister a UX module |
| `async.subscribe_event` | Subscribe to events |
| `async.request` | Forward request to another module |
| `async.notify` | Send event notification |

#### Elisp Engine Methods
| Method | Description |
|--------|-------------|
| `elisp.eval` | Evaluate Elisp code string |
| `elisp.eval_sexp` | Evaluate Elisp S-expression |
| `elisp.load` | Load Elisp file |
| `elisp.call` | Call Elisp function |
| `elisp.get_variable` | Get variable value |
| `elisp.set_variable` | Set variable value |

#### Buffer/File Methods
| Method | Description |
|--------|-------------|
| `buffer.open` | Open/create a buffer |
| `buffer.read` | Read file into buffer |
| `buffer.write` | Write buffer to file |
| `buffer.get_content` | Get buffer content |
| `buffer.set_content` | Set buffer content |
| `buffer.watch` | Watch file for changes |
| `buffer.unwatch` | Stop watching file |
| `buffer.remote_open` | Open file remotely |

#### LSP/Tree-sitter Methods
| Method | Description |
|--------|-------------|
| `lsp.start_server` | Start LSP server |
| `lsp.text_document_did_open` | Notify LSP of opened file |
| `lsp.text_document_did_change` | Notify LSP of changes |
| `lsp.text_document_hover` | Get hover info |
| `lsp.text_document_completion` | Get completions |
| `treesitter.parse` | Parse with tree-sitter |
| `treesitter.query` | Query AST |

---

## 5. Data Flow Examples

### 5.1 Opening a File
```
1. User (TTY UX): M-x find-file /path/file.el
2. TTY UX → Async Core: elisp.eval("(find-file \"/path/file.el\")")
3. Async Core → Elisp Engine: elisp.eval("(find-file \"/path/file.el\")")
4. Elisp Engine → Async Core: buffer.open("/path/file.el")
5. Async Core → Buffer Module: buffer.open("/path/file.el")
6. Buffer Module: Reads file, creates buffer, starts file watcher
7. Buffer Module → Async Core: {buffer_id, content, lines, metadata}
8. Async Core → Elisp Engine: Response (buffer created)
9. Elisp Engine: Creates buffer object, runs hooks
10. Elisp Engine → Async Core: ux.update(buffer_id, display_data)
11. Async Core → TTY UX: Render display data
12. TTY UX: Displays file
```

### 5.2 External File Change
```
1. External process modifies /path/file.el
2. Inotify/FSEvents → Async Core: file.changed event
3. Async Core → Buffer Module: file.changed notification
4. Buffer Module: Reads new content, calculates diff
5. Buffer Module → Async Core: buffer.external_change notification
6. Async Core → Elisp Engine: Run file watch hooks, auto-revert
7. Elisp Engine: Updates buffer content
8. Elisp Engine → Async Core: ux.update
9. Async Core → All UX Modules: Render updated content
```

---

## 6. Directory Structure (Proposed)

```
/workspace/
├── xemacs-async/          # Async I/O Core
│   ├── src/
│   │   ├── event_loop.c   # libuv integration
│   │   ├── rpc_server.c   # MessagePack-RPC server
│   │   ├── rpc_client.c   # MessagePack-RPC client
│   │   ├── file_watch.c   # inotify/FSEvents
│   │   └── service_disc.c # Service discovery
│   ├── include/
│   │   └── xemacs-async.h
│   ├── Makefile
│   └── configure
│
├── xemacs-elisp/          # Elisp Engine
│   ├── src/
│   │   ├── eval.c         # From monolithic src/
│   │   ├── bytecode.c
│   │   ├── symbols.c
│   │   ├── ...
│   │   └── rpc_glue.c     # RPC glue for Elisp APIs
│   ├── include/
│   │   └── xemacs-elisp.h
│   ├── Makefile
│   └── configure
│
├── xemacs-buffer/         # Buffer/File Manager
│   ├── src/
│   │   ├── buffer.c       # From monolithic src/
│   │   ├── fileio.c
│   │   ├── remote.c       # Remote file support
│   │   └── ...
│   ├── include/
│   │   └── xemacs-buffer.h
│   ├── Makefile
│   └── configure
│
├── xemacs-lang/           # LSP/Tree-sitter/ACP
│   ├── src/
│   │   ├── lsp_client.c   # LSP client
│   │   ├── tree_sitter.c  # Tree-sitter bindings
│   │   └── acp.c          # ACP protocol (future)
│   ├── include/
│   │   └── xemacs-lang.h
│   ├── Makefile
│   └── configure
│
├── ux/
│   ├── xemacs-tty/        # TTY UX
│   ├── xemacs-gtk/        # GTK UX
│   ├── xemacs-x11/        # X11 UX
│   ├── xemacs-msw/        # Windows UX
│   └── xemacs-cocoa/      # macOS UX
│
├── lib/                   # Shared libraries
│   ├── msgpack-rpc/       # MessagePack-RPC library
│   └── common/            # Shared utilities
│
├── lisp/                  # Existing Elisp packages (unchanged)
│   └── ...
│
├── docs/
│   └── superpowers/
│       └── specs/
│           └── 2026-05-15-distributed-modular-xemacs-design.md
│
└── ...                    # Other directories (unchanged)
```

---

## 7. Migration Strategy

### Phase 1: Async I/O Core
- Extract `src/event-stream.c` into `xemacs-async`
- Add libuv and MessagePack-RPC
- Keep Elisp/Buffer/UX in monolith for compatibility
- Test: Single UX connects to async core

### Phase 2: Elisp Engine
- Extract Elisp-related files (`eval.c`, `bytecode.c`, etc.) into `xemacs-elisp`
- Implement RPC communication between async core and elisp
- Test: Existing Elisp packages still work

### Phase 3: Buffer Manager
- Extract `buffer.c`, `fileio.c`, etc. into `xemacs-buffer`
- Implement file watching, remote file APIs
- Test: File I/O, remote editing

### Phase 4: Language Support
- Add LSP client integration
- Add Tree-sitter bindings
- Implement basic ACP structure
- Test: Language features

### Phase 5: Multiple UX Modules
- Extract each console backend into separate UX module
- Test: Multiple UX connections to same core

---

## 8. Open Questions

1. **Elisp Engine Deployment:** Should `xemacs-elisp` always be co-located with `xemacs-async`, or can it run remotely?
2. **ACP Protocol:** Is ACP a priority for initial release, or should it be deferred to later phases?
3. **UX Remoting:** Should UX modules be able to run remotely, or are they always local?
4. **Monolithic Fallback:** Should we maintain a monolithic build for debugging and portability?
5. **Module Versioning:** How should we handle version mismatches between modules?

---

## 9. References

1. [Neovim RPC Documentation](https://neovim.io/doc/user/api.html#rpc)
2. [MessagePack-RPC Specification](https://github.com/msgpack-rpc/msgpack-rpc/blob/master/spec.md)
3. [XEmacs Source Code](https://github.com/xemacs/xemacs)
4. [Tree-sitter Documentation](https://tree-sitter.github.io/tree-sitter/)
5. [Language Server Protocol](https://microsoft.github.io/language-server-protocol/)

---

**End of Document**

