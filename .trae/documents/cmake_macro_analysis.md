# CMake 配置宏定义问题全面分析

## 问题根本原因

### `#cmakedefine01` vs `#cmakedefine` 的区别

| 宏定义方式 | CMake 变量为真时生成 | CMake 变量为假时生成 |
|-----------|---------------------|---------------------|
| `#cmakedefine VAR` | `#define VAR` | `/* #undef VAR */` |
| `#cmakedefine01 VAR` | `#define VAR 1` | `#define VAR 0` |

### 代码中的检查方式

| 检查方式 | 含义 | `#cmakedefine01 VAR 0` 时的结果 |
|---------|------|--------------------------------|
| `#ifdef VAR` | 检查宏是否被定义 | **真**（错误！宏被定义了，即使值为 0） |
| `#if defined(VAR)` | 检查宏是否被定义 | **真**（错误！） |
| `#if VAR` | 检查宏的值是否为真 | 假（正确） |

## 需要从 `#cmakedefine01` 改为 `#cmakedefine` 的宏

### 高优先级（导致编译错误）

| 宏名 | 代码中的使用方式 | 当前错误影响 |
|------|-----------------|-------------|
| `HAVE_MS_WINDOWS` | `#if defined(HAVE_MS_WINDOWS)` | 错误启用 Windows 特定代码，如 `__declspec(dllexport)` |
| `HAVE_SHLIB` | `#if defined(HAVE_MS_WINDOWS) && defined(HAVE_SHLIB)` | 与 `HAVE_MS_WINDOWS` 一起导致错误 |

### 中优先级（功能错误，但可能不立即导致编译错误）

| 宏名 | 代码中的使用方式 |
|------|-----------------|
| `HAVE_SCROLLBARS` | `#ifdef HAVE_SCROLLBARS` |
| `HAVE_TOOLBARS` | `#ifdef HAVE_TOOLBARS` |
| `HAVE_MENUBARS` | `#ifdef HAVE_MENUBARS` |
| `HAVE_TTY` | `#ifdef HAVE_TTY` |
| `HAVE_GTK` | `#ifdef HAVE_GTK`（已修复为 `#cmakedefine`） |
| `HAVE_X_WINDOWS` | `#ifdef HAVE_X_WINDOWS` |
| `HAVE_WINDOW_SYSTEM` | `#ifdef HAVE_WINDOW_SYSTEM` |
| `HAVE_XLIKE` | `#if defined(HAVE_GTK)` 等内部使用 |
| `HAVE_X_WIDGETS` | 可能用 `#ifdef` 检查 |
| `HAVE_GUI_OBJECTS` | 可能用 `#ifdef` 检查 |
| `HAVE_POPUPS` | 可能用 `#ifdef` 检查 |
| `HAVE_DIALOGS` | 可能用 `#ifdef` 检查 |
| `HAVE_WIDGETS` | 可能用 `#ifdef` 检查 |

### 图像格式相关宏

| 宏名 | 代码中的使用方式 |
|------|-----------------|
| `HAVE_XPM` | 可能用 `#ifdef` 检查 |
| `HAVE_JPEG` | 可能用 `#ifdef` 检查 |
| `HAVE_TIFF` | 可能用 `#ifdef` 检查 |
| `HAVE_GIF` | 可能用 `#ifdef` 检查 |
| `HAVE_PNG` | 可能用 `#ifdef` 检查 |
| `HAVE_ZLIB` | 可能用 `#ifdef` 检查 |

### TLS/加密相关宏

| 宏名 | 代码中的使用方式 |
|------|-----------------|
| `HAVE_OPENSSL` | `#ifdef HAVE_OPENSSL`（已修复为 `#cmakedefine`） |
| `HAVE_GNUTLS` | `#ifdef HAVE_GNUTLS`（已修复为 `#cmakedefine`） |
| `HAVE_NSS` | `#ifdef HAVE_NSS`（已修复为 `#cmakedefine`） |

### 数据库相关宏

| 宏名 | 代码中的使用方式 |
|------|-----------------|
| `HAVE_DATABASE` | 可能用 `#ifdef` 检查 |
| `HAVE_DBM` | 可能用 `#ifdef` 检查 |
| `HAVE_BERKELEY_DB` | 可能用 `#ifdef` 检查 |
| `HAVE_LDAP` | 可能用 `#ifdef` 检查 |
| `HAVE_POSTGRESQL` | 可能用 `#ifdef` 检查 |

### 声音相关宏

| 宏名 | 代码中的使用方式 |
|------|-----------------|
| `HAVE_SOUND` | 可能用 `#ifdef` 检查 |
| `HAVE_NATIVE_SOUND` | 可能用 `#ifdef` 检查 |
| `HAVE_ALSA_SOUND` | 可能用 `#ifdef` 检查 |
| `HAVE_NAS_SOUND` | 可能用 `#ifdef` 检查 |
| `HAVE_ESD_SOUND` | 可能用 `#ifdef` 检查 |

### 模块/动态加载相关宏

| 宏名 | 代码中的使用方式 |
|------|-----------------|
| `HAVE_MODULES` | 可能用 `#ifdef` 检查 |
| `HAVE_DLOPEN` | 可能用 `#ifdef` 检查 |

### 其他功能宏

| 宏名 | 代码中的使用方式 |
|------|-----------------|
| `HAVE_XFT` | 可能用 `#ifdef` 检查 |
| `HAVE_FONTCONFIG` | 可能用 `#ifdef` 检查 |
| `HAVE_XIM` | 可能用 `#ifdef` 检查 |
| `HAVE_CANNA` | 可能用 `#ifdef` 检查 |
| `HAVE_WNN` | 可能用 `#ifdef` 检查 |
| `HAVE_BALLOON_HELP` | 可能用 `#ifdef` 检查 |
| `HAVE_DRAGNDROP` | 可能用 `#ifdef` 检查 |
| `HAVE_TOOLTALK` | `#ifdef HAVE_TOOLTALK` |
| `HAVE_SOCKS` | 可能用 `#ifdef` 检查 |
| `HAVE_UNIX_PROCESSES` | 可能用 `#ifdef` 检查 |

## 可以保留 `#cmakedefine01` 的宏（代码中用 `#if VAR` 检查）

需要确认代码中确实使用 `#if VAR` 检查的宏：

| 宏名 | 说明 |
|------|------|
| `USE_GCC` | 可能用 `#if USE_GCC` 检查 |
| `USE_GPLUSPLUS` | 可能用 `#if USE_GPLUSPLUS` 检查 |
| `HAVE_GLIBC` | 可能用 `#if HAVE_GLIBC` 检查 |
| `REL_ALLOC` | 可能用 `#if REL_ALLOC` 检查 |
| `WORDS_BIGENDIAN` | 通常用 `#if WORDS_BIGENDIAN` 检查 |
| `HAVE_SNPRINTF` | 可能用 `#if HAVE_SNPRINTF` 检查 |
| `HAVE_ALLOCA` | 可能用 `#if HAVE_ALLOCA` 检查 |
| `HAVE_ALLOCA_H` | 可能用 `#if HAVE_ALLOCA_H` 检查 |
| `DEBUG_XEMACS` | 可能用 `#if DEBUG_XEMACS` 检查 |
| `USE_ASSERTIONS` | 可能用 `#if USE_ASSERTIONS` 检查 |

## 修复策略

### 阶段 1：修复导致编译错误的宏（立即执行）

1. `HAVE_MS_WINDOWS` - 改为 `#cmakedefine`
2. `HAVE_SHLIB` - 改为 `#cmakedefine`

### 阶段 2：修复功能相关宏

所有 `HAVE_*` 开头的宏（除了确认用 `#if` 检查的）都应该改为 `#cmakedefine`。

### 阶段 3：全面验证

1. 对比 configure 和 CMake 生成的 config.h
2. 确保所有 `#cmakedefine01` 的宏在代码中确实用 `#if` 检查
3. 确保所有 `#cmakedefine` 的宏在代码中用 `#ifdef` 或 `#if defined()` 检查

## 当前已修复的宏

| 宏名 | 文件 | 修复状态 |
|------|------|---------|
| `NEED_MOTIF` | lwlib/config.h.in.cmake | ✅ 已修复 |
| `NEED_ATHENA` | lwlib/config.h.in.cmake | ✅ 已修复 |
| `NEED_LUCID` | lwlib/config.h.in.cmake | ✅ 已修复 |
| `LWLIB_USES_MOTIF` | src/config.h.in.cmake | ✅ 已修复 |
| `LWLIB_USES_ATHENA` | src/config.h.in.cmake | ✅ 已修复 |
| `LWLIB_MENUBARS_*` | src/config.h.in.cmake | ✅ 已修复 |
| `LWLIB_SCROLLBARS_*` | src/config.h.in.cmake | ✅ 已修复 |
| `LWLIB_DIALOGS_*` | src/config.h.in.cmake | ✅ 已修复 |
| `LWLIB_WIDGETS_*` | src/config.h.in.cmake | ✅ 已修复 |
| `LWLIB_TABS_LUCID` | src/config.h.in.cmake | ✅ 已修复 |
| `HAVE_ATHENA_3D` | src/config.h.in.cmake | ✅ 已修复 |
| `HAVE_ATHENA_I18N` | src/config.h.in.cmake | ✅ 已修复 |
| `HAVE_GTK` | src/config.h.in.cmake | ✅ 已修复 |
| `HAVE_GTK2` | src/config.h.in.cmake | ✅ 已修复 |
| `HAVE_GTK3` | src/config.h.in.cmake | ✅ 已修复 |
| `HAVE_GTK_*` | src/config.h.in.cmake | ✅ 已修复 |
| `HAVE_OPENSSL` | src/config.h.in.cmake | ✅ 已修复 |
| `HAVE_GNUTLS` | src/config.h.in.cmake | ✅ 已修复 |
| `HAVE_NSS` | src/config.h.in.cmake | ✅ 已修复 |
| `ALLOC_TYPE_STATS` | src/config.h.in.cmake | ✅ 已添加（硬编码） |
| `XCDECL` | src/config.h.in.cmake | ✅ 已添加（硬编码） |
| `SIGTYPE` | src/config.h.in.cmake | ✅ 已添加（硬编码） |
| `SIGRETURN` | src/config.h.in.cmake | ✅ 已添加（硬编码） |
