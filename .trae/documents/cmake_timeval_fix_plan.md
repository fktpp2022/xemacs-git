# CMake 构建问题修复计划 - struct timeval 重定义问题

## 问题诊断

### 错误信息
```
/home/kai/xemacs/core/xemacs-git/src/systime.h:152:8: error: redefinition of 'struct timeval'
  152 | struct timeval
      |        ^~~~~~~
/usr/include/x86_64-linux-gnu/bits/types/struct_timeval.h:8:8: note: originally defined here
    8 | struct timeval
      |        ^~~~~~~
```

### 根本原因分析

**问题根源：`HAVE_TIMEVAL` 宏未定义**

1. **autoconf 配置**：
   - `configure.ac:2656-2659` 检查 `struct timeval` 是否存在
   - 在现代 Linux 系统上，`HAVE_TIMEVAL` 被定义为 `#define HAVE_TIMEVAL 1`

2. **CMake 配置**：
   - `cmake/XEmacsConfigure.cmake` 中没有检查 `struct timeval`
   - `src/config.h.in.cmake` 中没有 `HAVE_TIMEVAL` 的定义

3. **代码逻辑**（`src/systime.h:145-162`）：
   ```c
   #ifdef HAVE_TIMEVAL
   #define EMACS_SELECT_TIME struct timeval
   #define EMACS_TIME_TO_SELECT_TIME(time, select_time) ((select_time) = (time))
   #else /* not HAVE_TIMEVAL */
   struct timeval {
     long tv_sec;
     long tv_usec;
   };
   #endif
   ```
   
   - 如果 `HAVE_TIMEVAL` 未定义，XEmacs 会自己定义 `struct timeval`
   - 但系统头文件（`sys/time.h`）已经定义了 `struct timeval`
   - 导致重定义错误

## 对比分析

### autoconf 配置
- **检查方式**：`AC_CHECK_TYPE([struct timeval], ...)`
- **结果**：`#define HAVE_TIMEVAL 1`

### CMake 配置
- **检查方式**：无
- **结果**：`HAVE_TIMEVAL` 未定义

## 修复方案

### 1. 在 `cmake/XEmacsConfigure.cmake` 中添加检查
```cmake
check_type_size("struct timeval" SIZEOF_STRUCT_TIMEVAL)
if(HAVE_SIZEOF_STRUCT_TIMEVAL)
  set(HAVE_TIMEVAL ON)
endif()
```

或者使用 `check_c_source_compiles`：
```cmake
check_c_source_compiles("
  #include <sys/time.h>
  int main(void) {
    struct timeval tv;
    return 0;
  }
" HAVE_TIMEVAL)
```

### 2. 在 `src/config.h.in.cmake` 中添加定义
```cmake
#cmakedefine HAVE_TIMEVAL
```

## 待确认的其他问题

### 其他可能缺失的宏定义
根据 `configure.ac` 和 `src/config.h.in`，可能还需要检查：

1. **`HAVE_TIMEVAL`** - 已确认需要添加
2. **`HAVE_STRUCT_TIMEVAL`** - 检查是否需要
3. **其他类型检查** - 需要全面对比

## 修复步骤

### 步骤 1：添加 CMake 检查
在 `cmake/XEmacsConfigure.cmake` 中添加 `struct timeval` 的检查。

### 步骤 2：更新 config.h.in.cmake
在 `src/config.h.in.cmake` 中添加 `HAVE_TIMEVAL` 的定义。

### 步骤 3：验证修复
重新运行 CMake 配置和构建，确认 `struct timeval` 重定义问题已解决。

## 风险评估

### 低风险
- 这是一个标准的 CMake 检查，与 autoconf 的行为一致
- 不会影响现有功能

## 相关文件

### 需要修改的文件
1. `cmake/XEmacsConfigure.cmake` - 添加 `struct timeval` 检查
2. `src/config.h.in.cmake` - 添加 `HAVE_TIMEVAL` 定义

### 相关文件
1. `src/systime.h` - 使用 `HAVE_TIMEVAL` 的代码
2. `configure.ac` - autoconf 的检查逻辑
3. `src/config.h.in` - autoconf 的模板文件

## 总结

`struct timeval` 重定义错误的根本原因是 CMake 配置中缺少 `HAVE_TIMEVAL` 宏的检查和定义。修复方案是在 CMake 配置中添加与 autoconf 等效的检查逻辑。
