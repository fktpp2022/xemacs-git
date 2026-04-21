# 构建失败根本原因分析计划

## 1. 问题背景

当前 CMake 构建在编译 `casefiddle.c` 和 `callint.c` 时遇到以下错误：

```
warning: control reaches end of non-void function [-Wreturn-type]
```

这些警告被当作错误处理，导致构建失败。

**关键观察**：

* 错误行号超出文件实际行数：

  * `casefiddle.c:389` - 实际文件只有 388 行

  * `callint.c:1080` - 实际文件只有 1079 行

* 这表明问题来自**宏展开后的代码**

## 2. 怀疑的原因

### 2.1 `wrong_type_argument` 函数的 `noreturn` 属性问题

根据之前的分析：

* `dead_wrong_type_argument` 被声明为 `DECLARE_DOESNT_RETURN`（有 `noreturn` 属性）

* `wrong_type_argument` 没有被声明为 `DECLARE_DOESNT_RETURN`

代码中使用模式：

```c
// CONCHECK_* 宏使用 wrong_type_argument（允许条件处理程序修复问题）
#define CONCHECK_INTEGER(x) do {            \
 if (!INTEGERP (x))                          \
   x = wrong_type_argument (Qintegerp, x);  \
}  while (0)

// CHECK_* 宏使用 dead_wrong_type_argument（无条件终止）
#define CHECK_INTEGER(x) do {                \
 if (!INTEGERP (x))                          \
   dead_wrong_type_argument (Qintegerp, x);  \
} while (0)
```

### 2.2 内联函数宏展开问题

`DEFUN` 宏展开后的函数末尾可能缺少 return 语句，导致编译器警告。

### 2.3 编译器标志差异

* CMake 构建使用了 `-Wall` 和 `-Wextra`

* 可能缺少 `FORCE_INLINE_FUNCTION_DEFINITION` 宏定义

* 可能缺少某些抑制警告的标志

## 3. 诊断策略

### 阶段 1：生成对比基准

**目标**：创建 configure 和 cmake 两个构建环境，生成可对比的产出物

#### 步骤 1.1：运行 configure 构建（基准）

```bash
# 创建 configure 构建目录
mkdir -p build-configure
cd build-configure

# 运行 configure（与目标参数一致）
../configure --with-menubars=lucid --with-scrollbars=lucid --with-widgets=lucid --with-mule

# 保存关键产出物
cp src/config.h ../config-h-configure.h
cp config.log ../config-log-configure.txt
cp config.status ../config-status-configure.txt
```

#### 步骤 1.2：运行 CMake 构建（问题环境）

```bash
# 创建 CMake 构建目录
mkdir -p build-cmake
cd build-cmake

# 运行 CMake 配置
cmake -S .. -B . \
  -DXEMACS_WITH_MENUBARS=ON \
  -DXEMACS_WITH_MENUBARS_TYPE=lucid \
  -DXEMACS_WITH_SCROLLBARS=ON \
  -DXEMACS_WITH_SCROLLBARS_TYPE=lucid \
  -DXEMACS_WITH_WIDGETS=ON \
  -DXEMACS_WITH_WIDGETS_TYPE=lucid \
  -DXEMACS_WITH_MULE=ON

# 尝试构建直到失败
cmake --build . -j4 2>&1 | tee build-log.txt

# 保存关键产出物
cp src/config.h ../config-h-cmake.h
cp CMakeCache.txt ../cmake-cache.txt
```

### 阶段 2：对比分析产出物

#### 步骤 2.1：对比 `config.h` 文件

**关键对比项**：

| 宏定义类别             | 需要检查的宏                                                                                       | 说明                   |
| ----------------- | -------------------------------------------------------------------------------------------- | -------------------- |
| **内联函数**          | `INLINE_HEADER`, `DECLARE_INLINE_HEADER`                                                     | 内联函数定义方式             |
| **不返回函数**         | `DECLARE_DOESNT_RETURN`, `DOESNT_RETURN`, `DECLARE_DOESNT_RETURN_TYPE`, `DOESNT_RETURN_TYPE` | 影响 noreturn 属性       |
| **FORCE\_INLINE** | `FORCE_INLINE_FUNCTION_DEFINITION`                                                           | 内联函数强制定义             |
| **编译器相关**         | `__GNUC__`, `__STDC_VERSION__` 相关宏                                                           | 影响 INLINE\_HEADER 定义 |

#### 步骤 2.2：对比编译标志

检查两个构建系统使用的编译标志：

* `CFLAGS` / `CMAKE_C_FLAGS`

* 是否有 `-Wreturn-type` 或 `-Wno-return-type`

* 是否有 `-Wall`, `-Wextra`

* 优化级别

#### 步骤 2.3：检查 `compiler.h` 的包含

确认 `compiler.h` 是否正确包含了 `DECLARE_DOESNT_RETURN` 等宏的定义。

### 阶段 3：深入分析代码

#### 步骤 3.1：检查 `compiler.h` 文件

查找 `DECLARE_DOESNT_RETURN` 宏的定义位置和定义方式。

#### 步骤 3.2：检查 `casefiddle.c` 和 `callint.c`

分析这些文件末尾的 `DEFUN` 宏展开后的代码结构：

```c
// DEFUN 宏展开后的典型结构
DEFUN("function-name", Ffunction_name, 1, 2, 0, /*
Documentation...
*/
    (Lisp_Object arg1, Lisp_Object arg2))
{
    // 函数体...
    // 可能缺少 return 语句？
}
```

#### 步骤 3.3：生成预处理后的代码

使用 `-E` 选项生成预处理后的代码，查看宏展开后的实际代码：

```bash
# 对 casefiddle.c 生成预处理代码
gcc -E -Ibuild-cmake/src -Isrc -DHAVE_CONFIG_H src/casefiddle.c > casefiddle-preprocessed.c

# 查看文件末尾
tail -100 casefiddle-preprocessed.c
```

### 阶段 4：对比测试

#### 步骤 4.1：测试使用 autoconf 生成的 config.h

在 CMake 构建中使用 autoconf 生成的 `config.h`，看是否能成功构建：

```bash
# 备份 cmake 生成的 config.h
mv build-cmake/src/config.h build-cmake/src/config.h.cmake

# 使用 autoconf 生成的 config.h
cp config-h-configure.h build-cmake/src/config.h

# 重新构建
cmake --build build-cmake -j4
```

#### 步骤 4.2：测试添加 `-Wno-return-type`

在 CMake 中添加 `-Wno-return-type` 标志，看是否能通过构建：

```cmake
# 在 XEmacsConfigure.cmake 中添加
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-return-type")
```

#### 步骤 4.3：测试添加 `FORCE_INLINE_FUNCTION_DEFINITION`

检查是否需要定义 `FORCE_INLINE_FUNCTION_DEFINITION` 宏：

```cmake
# 在 CMake 中添加
add_definitions(-DFORCE_INLINE_FUNCTION_DEFINITION)
```

## 4. 预期发现与修复方案

### 场景 A：`INLINE_HEADER` 定义不一致

**发现**：autoconf 和 cmake 生成的 `INLINE_HEADER` 宏定义不同

**修复**：确保 `config.h.in.cmake` 中的 `INLINE_HEADER` 定义与 autoconf 一致

### 场景 B：缺少 `FORCE_INLINE_FUNCTION_DEFINITION`

**发现**：CMake 构建中缺少 `FORCE_INLINE_FUNCTION_DEFINITION` 宏定义

**修复**：在 CMake 配置中添加适当的宏定义

### 场景 C：`DECLARE_DOESNT_RETURN` 未定义

**发现**：`compiler.h` 中的宏定义未被正确包含或定义

**修复**：确保 `compiler.h` 正确包含或在 `config.h.in.cmake` 中添加缺失的宏

### 场景 D：编译器标志过于严格

**发现**：CMake 使用了 `-Wextra` 而 autoconf 没有

**修复**：添加 `-Wno-return-type` 抑制警告（如果确认代码正确）

## 5. 执行步骤清单

* [ ] 步骤 1.1：运行 configure 构建生成基准

* [ ] 步骤 1.2：运行 CMake 构建复现问题

* [ ] 步骤 2.1：对比两个 config.h 文件

* [ ] 步骤 2.2：对比编译标志

* [ ] 步骤 3.1：检查 compiler.h

* [ ] 步骤 3.2：分析问题文件的 DEFUN 宏

* [ ] 步骤 3.3：生成预处理代码查看宏展开

* [ ] 步骤 4.1：测试使用 autoconf 的 config.h

* [ ] 步骤 4.2：测试添加 -Wno-return-type

* [ ] 步骤 4.3：测试添加 FORCE\_INLINE\_FUNCTION\_DEFINITION

* [ ] 根据发现实施修复

* [ ] 验证构建成功

## 6. 涉及的关键文件

| 文件路径                          | 用途                            |
| ----------------------------- | ----------------------------- |
| `src/config.h.in.cmake`       | CMake 配置头模板                   |
| `src/config.h.in`             | autoconf 配置头模板                |
| `cmake/XEmacsConfigure.cmake` | CMake 配置逻辑                    |
| `src/lisp.h`                  | 包含 DEFUN 宏定义                  |
| `src/compiler.h`              | 包含 DECLARE\_DOESNT\_RETURN 等宏 |
| `src/casefiddle.c`            | 出现错误的文件                       |
| `src/callint.c`               | 出现错误的文件                       |

