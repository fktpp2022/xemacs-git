# 修复 CMake 构建配置问题计划

## 问题诊断结果

通过对比 configure 和 CMake 生成的 `config.h` 文件，发现了以下问题：

### 问题 1：`#cmakedefine01` 与 `#ifdef` 不兼容（已修复）

**根本原因：**
- CMake 使用 `#cmakedefine01 VAR` 定义宏时，总是生成 `#define VAR 0` 或 `#define VAR 1`
- 但代码中使用 `#ifdef VAR` 或 `#if defined(VAR)` 检查宏是否存在，而不是检查宏的值
- 这导致即使宏值为 0，`#ifdef` 也会评估为真，错误地包含了不需要的代码

**具体表现：**
- `lwlib/lwlib.c` 中出现 `xm_create_dialog`, `xaw_create_dialog`, `XmNfontList`, `XmFontList` 等未声明错误
- 原因：`#ifdef NEED_MOTIF` 和 `#ifdef NEED_ATHENA` 错误地评估为真，导致包含了不需要的 Motif 和 Athena 代码

**对比示例：**
```
configure 生成：  /* #undef LWLIB_USES_MOTIF */  → #ifdef 为假
CMake 生成：      #define LWLIB_USES_MOTIF 0    → #ifdef 为真！（错误）
```

**已修复的文件：**
1. `lwlib/config.h.in.cmake`:
   - 将 `NEED_MOTIF`, `NEED_ATHENA`, `NEED_LUCID` 从 `#cmakedefine01` 改为 `#cmakedefine`

2. `src/config.h.in.cmake`:
   - 将 `LWLIB_USES_MOTIF`, `LWLIB_USES_ATHENA` 从 `#cmakedefine01` 改为 `#cmakedefine`
   - 将 `LWLIB_MENUBARS_*`, `LWLIB_SCROLLBARS_*`, `LWLIB_DIALOGS_*`, `LWLIB_WIDGETS_*`, `LWLIB_TABS_*` 从 `#cmakedefine01` 改为 `#cmakedefine`
   - 将 `HAVE_ATHENA_3D`, `HAVE_ATHENA_I18N` 从 `#cmakedefine01` 改为 `#cmakedefine`

---

### 问题 2：缺少硬编码的默认宏定义（待修复）

**问题描述：**
`config.h.in` 中有一些宏是硬编码的默认值，不是通过 `#undef` 定义的（即不是通过 configure 检测的）。CMake 的 `config.h.in.cmake` 缺少这些定义。

**缺少的宏：**

| 宏名 | autoconf config.h.in 中的定义 | 说明 |
|------|-------------------------------|------|
| `ALLOC_TYPE_STATS` | `# define ALLOC_TYPE_STATS 1`（条件定义） | 类型分配统计 |
| `XCDECL` | `#define XCDECL`（默认为空） | 函数调用约定（Windows 上为 `__cdecl`） |
| `SIGTYPE` | `#define SIGTYPE void XCDECL` | 信号处理器类型 |
| `SIGRETURN` | `#define SIGRETURN return` | 信号处理器返回值 |

**具体错误：**
```
/home/kai/xemacs/core/xemacs-git/src/syssignal.h:108:21: error: expected ')' before '*' token
  108 | typedef void (XCDECL * signal_handler_t) (int);
      |                     ^~
```

**需要修复的文件：**
- `src/config.h.in.cmake`：添加上述宏的定义

---

## 修复步骤

### 步骤 1：添加缺少的宏定义到 src/config.h.in.cmake

在 `src/config.h.in.cmake` 的 `ERROR_CHECK_TYPES` 之后、`SIZEOF_SHORT` 之前添加：

```c
/* This enables type based information (updated during gc). */
#if !defined (MC_ALLOC) || 1
# define ALLOC_TYPE_STATS 1
#endif

#ifndef XCDECL
#define XCDECL
#endif

/* SIGTYPE is the macro we actually use. */
#ifndef SIGTYPE
#define SIGTYPE void XCDECL
#define SIGRETURN return
#endif
```

### 步骤 2：重新运行 CMake 配置和构建

```bash
cd build-cmake
cmake -S .. -B . -DXEMACS_WITH_MENUBARS=ON -DXEMACS_WITH_MENUBARS_TYPE=lucid -DXEMACS_WITH_SCROLLBARS=ON -DXEMACS_WITH_SCROLLBARS_TYPE=lucid -DXEMACS_WITH_WIDGETS=ON -DXEMACS_WITH_WIDGETS_TYPE=lucid
cmake --build . -j4
```

### 步骤 3：验证构建成功

检查是否生成了 `xemacs` 可执行文件。

---

## 风险和注意事项

1. **`#cmakedefine` vs `#cmakedefine01` 的选择**：
   - 如果代码中使用 `#ifdef VAR` 或 `#if defined(VAR)` 检查 → 使用 `#cmakedefine VAR`
   - 如果代码中使用 `#if VAR` 检查 → 使用 `#cmakedefine01 VAR`

2. **需要全面检查**：
   - `src/config.h.in.cmake` 中还有其他 `#cmakedefine01` 的宏，需要确认它们在代码中的使用方式
   - 但当前优先修复导致构建失败的关键宏

---

## 当前状态

| 任务 | 状态 |
|------|------|
| 诊断根本原因 | ✅ 完成 |
| 修复 `#cmakedefine01` 问题 | ✅ 完成 |
| 发现缺少的宏定义 | ✅ 完成 |
| 添加缺少的宏定义 | ⏳ 待执行 |
| 重新配置和构建 | ⏳ 待执行 |
| 验证构建成功 | ⏳ 待执行 |
