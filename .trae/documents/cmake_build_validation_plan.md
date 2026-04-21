# CMake 构建配置验证与修正计划

## 1. 任务目标

使用 CMake 构建 XEmacs 工程，要求构建参数与以下 configure 命令等效：

```bash
./configure --with-menubars=lucid --with-scrollbars=lucid --with-widgets=lucid --with-mule
```

目的是检查 CMake 构建配置框架的正确性，如果发现问题尝试修正，直到能够正确构建出 xemacs 可执行文件。

## 2. 现有 CMake 配置分析

### 2.1 配置参数对应关系

| configure 参数              | CMake 选项                      | 当前默认值   | 状态     |
| ------------------------- | ----------------------------- | ------- | ------ |
| `--with-menubars=lucid`   | `XEMACS_WITH_MENUBARS_TYPE`   | "lucid" | ✓ 已正确  |
| `--with-scrollbars=lucid` | `XEMACS_WITH_SCROLLBARS_TYPE` | "lucid" | ✓ 已正确  |
| `--with-widgets=lucid`    | `XEMACS_WITH_WIDGETS_TYPE`    | "lucid" | ✓ 类型正确 |
| `--with-widgets=lucid`    | `XEMACS_WITH_WIDGETS`         | **OFF** | ✗ 需启用  |
| `--with-mule`             | (无对应选项)                       | -       | ✗ 缺失   |

### 2.2 已发现的潜在问题

#### 问题1: Widgets 支持默认关闭

* **位置**: `cmake/XEmacsOptions.cmake:12`

* **问题**: `option(XEMACS_WITH_WIDGETS "Enable widget support" OFF)`

* **影响**: 即使 `XEMACS_WITH_WIDGETS_TYPE="lucid"`，如果 `XEMACS_WITH_WIDGETS=OFF`，widgets 功能不会被启用

#### 问题2: 缺少 MULE 支持选项

* **位置**: `cmake/XEmacsOptions.cmake` (无对应选项)

* **问题**:

  * configure 中的 `--with-mule` 对应设置 `UNICODE_INTERNAL` 宏

  * `src/config.h.in.cmake:148` 有 `#cmakedefine01 UNICODE_INTERNAL`

  * 但 CMake 配置中没有选项来启用它

* **影响**: 无法构建支持多语言 (MULE) 的版本

#### 问题3: X11 库查找可能不完整

* **位置**: `cmake/XEmacsConfigure.cmake`

* **问题**:

  * 没有使用 `find_package(X11 REQUIRED)` 来查找 X11 库

  * 代码中使用了 `X11_LIBRARIES`, `X11_Xt_LIB`, `X11_Xmu_LIB` 等变量

  * 这些变量需要通过 `find_package(X11)` 正确设置

* **影响**: 链接时可能找不到 X11 相关库

#### 问题4: Lucid 相关宏设置检查

* **位置**: `src/config.h.in.cmake`, `lwlib/config.h.in.cmake`

* **需要检查的宏**:

  * `LWLIB_MENUBARS_LUCID` (src/config.h.in.cmake:100)

  * `LWLIB_SCROLLBARS_LUCID` (src/config.h.in.cmake:102)

  * `LWLIB_TABS_LUCID` (src/config.h.in.cmake:109)

  * `NEED_LUCID` (lwlib/config.h.in.cmake:9)

## 3. 实施步骤

### 阶段1: 初始配置测试

1. 创建构建目录 `build-cmake`
2. 运行 CMake 配置，使用等效参数:

   ```bash
   cmake -S . -B build-cmake \
     -DXEMACS_WITH_MENUBARS=ON \
     -DXEMACS_WITH_MENUBARS_TYPE=lucid \
     -DXEMACS_WITH_SCROLLBARS=ON \
     -DXEMACS_WITH_SCROLLBARS_TYPE=lucid \
     -DXEMACS_WITH_WIDGETS=ON \
     -DXEMACS_WITH_WIDGETS_TYPE=lucid
   ```
3. 记录配置过程中的错误和警告

### 阶段2: 识别并修复问题

#### 修复 A: 启用 MULE 支持

* 修改 `cmake/XEmacsOptions.cmake`: 添加 `XEMACS_WITH_MULE` 选项

* 修改 `cmake/XEmacsConfigure.cmake`: 根据 `XEMACS_WITH_MULE` 设置 `UNICODE_INTERNAL`

#### 修复 B: 完善 X11 库查找

* 在 `cmake/XEmacsConfigure.cmake` 或主 `CMakeLists.txt` 中添加:

  ```cmake
  if(XEMACS_WITH_X11)
    find_package(X11 REQUIRED)
    # 检查 Xt, Xmu, Xext, Xpm 等组件
  endif()
  ```

#### 修复 C: 确保 Lucid 宏正确设置

* 检查 `cmake/XEmacsConfigure.cmake` 中是否正确设置:

  * `HAVE_LUCID_WIDGETS`

  * `HAVE_LUCID_MENUBARS`

  * `HAVE_LUCID_SCROLLBARS`

  * `HAVE_LUCID_DIALOGS`

  * 对应的 `LWLIB_*_LUCID` 宏

  * `NEED_LUCID` 宏

### 阶段3: 构建验证

1. 运行 `cmake --build build-cmake`
2. 检查是否成功生成:

   * `bin/xemacs` 可执行文件

   * 必要的库文件
3. 记录构建错误并继续修复

### 阶段4: 迭代修复

根据构建错误，继续修复以下可能的问题:

* 缺少的源文件

* 不正确的编译定义

* 库链接问题

* 头文件包含路径问题

## 4. 涉及的主要文件

| 文件路径                          | 用途            |
| ----------------------------- | ------------- |
| `CMakeLists.txt`              | 主 CMake 配置文件  |
| `cmake/XEmacsOptions.cmake`   | 构建选项定义        |
| `cmake/XEmacsConfigure.cmake` | 配置检测逻辑        |
| `cmake/XEmacsOS.cmake`        | 操作系统相关配置      |
| `src/CMakeLists.txt`          | 源文件构建配置       |
| `lwlib/CMakeLists.txt`        | lwlib 库构建配置   |
| `src/config.h.in.cmake`       | 配置头文件模板       |
| `lwlib/config.h.in.cmake`     | lwlib 配置头文件模板 |

## 5. 成功标准

1. CMake 配置成功完成，无错误
2. `cmake --build` 成功完成
3. 生成 `bin/xemacs` 可执行文件
4. 构建配置与 `./configure --with-menubars=lucid --with-scrollbars=lucid --with-widgets=lucid --with-mule` 等效

## 6. 风险与注意事项

1. **修改风险**: 修改 CMake 配置可能影响其他构建配置
2. **兼容性**: 确保修改不会破坏其他平台 (Windows, macOS 等) 的构建
3. **增量修复**: 建议每次只修复一个问题，然后重新配置构建验证
4. **对比参考**: 可以对比 autoconf 构建生成的 `config.h` 来验证 CMake 配置的正确性

