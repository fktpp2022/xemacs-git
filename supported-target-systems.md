# XEmacs 目标平台支持评估文档（CMake 迁移版）

*文档版本: 1.0*
*创建日期: 2026-04-20*

---

## 1. 概述

本文档评估 XEmacs 当前支持的所有目标平台，分析它们在迁移到 CMake 构建系统时的维护价值，并给出是否应该继续支持的建议。

### 1.1 评估维度

每个平台将按以下维度进行评估：

| 维度 | 说明 |
|------|------|
| **活跃程度** | 平台当前是否仍在积极开发和维护 |
| **用户基数** | 使用该平台的用户数量估计 |
| **维护成本** | 维护该平台所需的开发和测试资源 |
| **CMake 支持** | CMake 对该平台的原生支持程度 |
| **测试可行性** | 是否有可用的 CI/测试环境 |

### 1.2 支持等级定义

| 等级 | 说明 | 迁移建议 |
|------|------|----------|
| **P0 - 核心支持** | 必须保留，迁移优先级最高 | 优先迁移，完整测试 |
| **P1 - 重要支持** | 有活跃用户，但非核心 | 可迁移，但可简化 |
| **P2 - 边缘支持** | 用户稀少，维护成本高 | 考虑弃用或仅社区维护 |
| **P3 - 历史遗留** | 无实际用户，仅历史意义 | 移除支持 |

---

## 2. 操作系统平台评估

### 2.1 P0 - 核心支持平台

#### 2.1.1 GNU/Linux

| 属性 | 评估 |
|------|------|
| **头文件** | `src/s/linux.h` |
| **当前状态** | ✅ 活跃维护 |
| **用户基数** | 极高 |
| **维护成本** | 低 |
| **CMake 支持** | 优秀（原生支持） |
| **测试可行性** | 高（GitHub Actions, GitLab CI 等） |

**详细分析：**

Linux 是 XEmacs 最主要的运行平台，拥有最大的用户基数。`linux.h` 文件相对简洁，主要定义：
- `LINUX`, `USG` 等系统标识
- `SYSTEM_TYPE = "linux"`
- 一些信号和链接相关的定义

**CMake 迁移考量：**
- CMake 对 Linux 有原生且优秀的支持
- 大多数检测可以用 CMake 内置模块替代
- 可以利用 CMake 的 `CMAKE_SYSTEM_NAME STREQUAL "Linux"` 判断

**建议：** ✅ **必须保留，优先迁移**

---

#### 2.1.2 Windows (Native / MinGW / Cygwin)

| 属性 | 评估 |
|------|------|
| **头文件** | `src/s/windowsnt.h`, `src/s/mingw32.h`, `src/s/cygwin32.h`, `src/s/win32-native.h` |
| **当前状态** | ✅ 活跃维护 |
| **用户基数** | 高 |
| **维护成本** | 中高 |
| **CMake 支持** | 优秀（Visual Studio 原生生成器） |
| **测试可行性** | 中高（GitHub Actions 支持 Windows） |

**详细分析：**

Windows 平台有多个变体：
- **Windows Native (MSVC)**：使用 Visual Studio 编译器，`windowsnt.h`
- **MinGW**：GCC for Windows，`mingw32.h`
- **Cygwin**：POSIX 兼容层，`cygwin32.h`

相关文件包含大量平台特定定义：
- 类型定义（`mode_t`, `uid_t`, `gid_t` 等）
- 编译器特性（MSVC 特定的 pragma）
- 信号和 I/O 处理
- 窗口系统集成（`HAVE_DRAGNDROP` 等）

**CMake 迁移考量：**
- CMake 可以生成原生 Visual Studio 工程，这是巨大优势
- CMake 有 `WIN32`, `MINGW`, `CYGWIN` 等内置变量
- 可以简化为主要支持 MSVC 和 MinGW，Cygwin 可降级

**建议：** 
- ✅ **MSVC (Native Windows)**: 必须保留，CMake 优势明显
- ✅ **MinGW**: 保留，用户较多
- ⚠️ **Cygwin**: 考虑降级为 P2，维护成本高但用户少

---

### 2.2 P1 - 重要支持平台

#### 2.2.1 macOS (Darwin)

| 属性 | 评估 |
|------|------|
| **头文件** | （configure.ac 中动态检测，无单独 darwin.h） |
| **当前状态** | ✅ 活跃维护 |
| **用户基数** | 高 |
| **维护成本** | 中 |
| **CMake 支持** | 优秀（Xcode 生成器） |
| **测试可行性** | 中（GitHub Actions 支持 macOS） |

**详细分析：**

macOS 是一个重要的桌面平台，用户基数较大。有趣的是，代码库中没有单独的 `darwin.h`，而是通过 configure.ac 动态检测。

从 configure.ac 可以看到：
- Darwin/macOS 使用类似 BSD 的系统调用
- 需要处理 Frameworks 和 bundles
- 有特殊的动态模块加载机制（`--with-modules` 在 Darwin 上有特殊处理）

**CMake 迁移考量：**
- CMake 支持生成 Xcode 工程，对 macOS 开发友好
- CMake 有 `APPLE` 内置变量
- 需要处理 Frameworks 和 bundles

**建议：** ✅ **保留，高优先级**

---

#### 2.2.2 NetBSD / OpenBSD / FreeBSD

| 属性 | 评估 |
|------|------|
| **头文件** | `src/s/netbsd.h`, `src/s/openbsd.h`, `src/s/bsd-common.h` |
| **当前状态** | ✅ 活跃维护（BSD 社区） |
| **用户基数** | 中 |
| **维护成本** | 低到中 |
| **CMake 支持** | 良好 |
| **测试可行性** | 中（需要专用 CI 环境） |

**详细分析：**

BSD 系列操作系统：
- **NetBSD**: 强调可移植性，支持多种硬件
- **OpenBSD**: 强调安全性
- **FreeBSD**: 最流行的 BSD 发行版

`bsd-common.h` 提供基础支持：
- 定义 `BSD4_3`, `BSD_SYSTEM`
- `SYSTEM_TYPE = "berkeley-unix"`
- 邮件锁定机制等

**CMake 迁移考量：**
- CMake 对 BSD 支持良好
- 可以用 `CMAKE_SYSTEM_NAME` 判断 (`NetBSD`, `OpenBSD`, `FreeBSD`)
- 维护成本相对较低，因为 BSD 遵循 POSIX 标准

**建议：** ✅ **保留，中等优先级**
- 但可以考虑合并处理（统一 BSD 代码路径）

---

### 2.3 P2 - 边缘支持平台

#### 2.3.1 Solaris (Oracle Solaris / illumos)

| 属性 | 评估 |
|------|------|
| **头文件** | `src/s/sol2.h` |
| **当前状态** | ⚠️ 低活跃度（Oracle Solaris）/ ✅ 社区活跃（illumos） |
| **用户基数** | 低 |
| **维护成本** | 中 |
| **CMake 支持** | 良好 |
| **测试可行性** | 低（难以获得测试环境） |

**详细分析：**

`sol2.h` 包含：
- Solaris 2.x 特定定义
- 套接字库 (`-lsocket -lnsl -lelf`)
- 一些 GCC 在 Solaris 上的兼容性修复

现状：
- **Oracle Solaris**: 活跃度低，用户基数持续下降
- **illumos 衍生版** (OpenIndiana, OmniOS 等): 有一定社区活跃度

**CMake 迁移考量：**
- CMake 支持 Solaris/illumos
- 但测试环境难以获得
- 维护成本与用户收益不成比例

**建议：** ⚠️ **考虑降级**
- 如果有社区贡献者愿意维护，可保留
- 否则标记为"社区支持"或逐步移除

---

#### 2.3.2 GNU/Hurd

| 属性 | 评估 |
|------|------|
| **头文件** | `src/s/gnu.h` |
| **当前状态** | ⚠️ 学术项目，低活跃度 |
| **用户基数** | 极低 |
| **维护成本** | 低 |
| **CMake 支持** | 未知（可能有限） |
| **测试可行性** | 极低 |

**详细分析：**

`gnu.h` 非常简洁，主要包含：
- 引用 `bsd4-3.h` (注意：代码中有 `#include "bsd4-3.h"`，但实际文件是 `bsd-common.h`)
- `SYSTEM_TYPE = "gnu"`
- 一些链接器和数据段定义

GNU/Hurd 是一个学术/自由软件项目，几乎没有实际用户。

**建议：** ❌ **考虑移除或标记为实验性**
- 几乎没有实际用户
- 维护成本虽然低，但无收益
- CMake 支持不确定

---

### 2.4 P3 - 历史遗留平台（建议移除）

#### 2.4.1 IRIX 6.5 (SGI)

| 属性 | 评估 |
|------|------|
| **头文件** | `src/s/irix6-5.h` |
| **当前状态** | ❌ 已停止 |
| **用户基数** | 为零 |
| **维护成本** | 中 |
| **CMake 支持** | 无 |
| **测试可行性** | 不可能 |

**详细分析：**

SGI 公司早已倒闭，IRIX 系统已停止维护多年。`irix6-5.h` 包含：
- 大量特定于 MIPS 架构和 IRIX 的定义
- 编译器选项（`-xansi` 等）
- 已知的 bug 修复（`BROKEN_SIGIO` 等）

**建议：** ❌ **立即移除**
- 无实际用户
- 硬件不可得
- 维护代码是负担

---

#### 2.4.2 HP-UX 11

| 属性 | 评估 |
|------|------|
| **头文件** | `src/s/hpux11.h`, `src/s/hpux11-shr.h` |
| **当前状态** | ❌ 已停止 |
| **用户基数** | 接近零 |
| **维护成本** | 中 |
| **CMake 支持** | 有限（历史支持） |
| **测试可行性** | 极低 |

**详细分析：**

HP-UX 是惠普的 Unix 系统，现在几乎只在遗留系统中使用。`hpux11.h` 包含：
- PA-RISC 架构特定定义
- 系统 V 兼容性 (`USG`, `USG5`)
- 特殊的链接器和 PTY 处理

**建议：** ❌ **移除**
- 用户基数可忽略
- 难以获得测试环境

---

#### 2.4.3 AIX 4.2 (IBM)

| 属性 | 评估 |
|------|------|
| **头文件** | `src/s/aix4-2.h` |
| **当前状态** | ❌ 已停止 |
| **用户基数** | 接近零 |
| **维护成本** | 中 |
| **CMake 支持** | 有限 |
| **测试可行性** | 极低 |

**详细分析：**

AIX 4.2 是非常老的版本（1990 年代）。`aix4-2.h` 包含：
- IBM XL C/C++ 编译器特定选项
- 已知的 bug 修复（`BROKEN_SIGIO`, `HAVE_GETADDRINFO` 问题）
- X11 国际化问题的 workaround

**建议：** ❌ **移除**
- 即使有 AIX 用户，也会使用更新的 AIX 版本
- 代码中有大量针对旧版本 bug 的 workaround

---

#### 2.4.4 System V 衍生版 (usg5-4, usg5-4-2)

| 属性 | 评估 |
|------|------|
| **头文件** | `src/s/usg5-4.h`, `src/s/usg5-4-2.h` |
| **当前状态** | ❌ 历史遗留 |
| **用户基数** | 为零 |
| **维护成本** | 中 |
| **CMake 支持** | 无 |
| **测试可行性** | 不可能 |

**详细分析：**

这些是 AT&T System V Release 4 的通用支持文件，被多个具体平台引用：
- `sol2.h` 包含 `usg5-4-2.h`
- `irix6-5.h` 包含 `usg5-4.h`
- 其他可能的 SVR4 衍生版

**建议：** ❌ **与依赖它们的平台一起移除**
- 如果保留 Solaris/illumos，需要保留部分
- 否则可以完全移除

---

#### 2.4.5 Mach BSD 4.3

| 属性 | 评估 |
|------|------|
| **头文件** | `src/s/mach-bsd4-3.h` |
| **当前状态** | ❌ 历史遗留 |
| **用户基数** | 为零 |
| **维护成本** | 低 |
| **CMake 支持** | 无 |
| **测试可行性** | 不可能 |

**详细分析：**

Mach 是一个微内核操作系统，曾用于：
- 早期的 macOS (NeXTSTEP)
- GNU Hurd (部分)
- 其他研究项目

现在没有任何实际用户。

**建议：** ❌ **立即移除**

---

## 3. 硬件架构评估

### 3.1 P0 - 核心架构

#### 3.1.1 x86 / x86_64 (AMD64)

| 属性 | 评估 |
|------|------|
| **头文件** | `src/m/intel386.h` |
| **当前状态** | ✅ 主导地位 |
| **用户基数** | 极高 |
| **维护成本** | 低 |
| **CMake 支持** | 优秀 |
| **测试可行性** | 高 |

**详细分析：**

`intel386.h` 支持：
- 32位 x86 (i386, i486, Pentium 等)
- 64位 x86_64/AMD64 (通过扩展)

包含：
- 加载平均计算的类型定义
- 段界限定义
- Solaris x86 的特定 workaround

**CMake 迁移考量：**
- CMake 对 x86/x86_64 有完美支持
- 可以用 `CMAKE_SIZEOF_VOID_P` 判断 32/64 位
- 大多数架构特定定义可以用 CMake 内置变量替代

**建议：** ✅ **必须保留**

---

#### 3.1.2 ARM / AArch64

| 属性 | 评估 |
|------|------|
| **头文件** | `src/m/arm.h` (32位), 无单独 AArch64 文件 |
| **当前状态** | ✅ 快速增长 |
| **用户基数** | 高（嵌入式、服务器、桌面） |
| **维护成本** | 低 |
| **CMake 支持** | 优秀 |
| **测试可行性** | 中高（GitHub Actions 支持 ARM） |

**详细分析：**

`arm.h` 目前只支持 32位 ARM。需要扩展支持：
- AArch64 (ARM64) - 现代服务器和桌面
- ARMv7, ARMv8 等

ARM 架构的重要性持续增长：
- Raspberry Pi 等单板计算机
- Apple Silicon (M1/M2/M3 系列)
- ARM 服务器 (AWS Graviton 等)

**CMake 迁移考量：**
- CMake 对 ARM 支持良好
- 需要添加 AArch64 的检测
- Apple Silicon 需要特殊处理（Rosetta 2 等）

**建议：** ✅ **保留并扩展**
- 添加 AArch64 支持
- 测试 Apple Silicon

---

### 3.2 P1 - 重要架构

#### 3.2.1 PowerPC / PowerPC64

| 属性 | 评估 |
|------|------|
| **头文件** | `src/m/powerpc.h` |
| **当前状态** | ⚠️ 下降中 |
| **用户基数** | 中低 |
| **维护成本** | 低 |
| **CMake 支持** | 良好 |
| **测试可行性** | 低 |

**详细分析：**

`powerpc.h` 包含：
- 加载平均计算
- 链接器脚本引用 (`ppc.ldscript`)
- Linux/PowerPC 特定定义

现状：
- 旧款 Macintosh (PowerPC Mac) - 已淘汰
- IBM POWER 服务器 - 仍有使用但在下降
- 嵌入式 PowerPC - 仍有使用

**建议：** ⚠️ **考虑保留但标记为社区支持**
- 用户基数在下降
- 如果有社区贡献者，可保留

---

#### 3.2.2 MIPS

| 属性 | 评估 |
|------|------|
| **头文件** | `src/m/mips.h` |
| **当前状态** | ⚠️ 嵌入式为主 |
| **用户基数** | 低（作为桌面/服务器） |
| **维护成本** | 低 |
| **CMake 支持** | 良好 |
| **测试可行性** | 低 |

**详细分析：**

MIPS 架构：
- 曾经用于 SGI IRIX 工作站
- 现在主要用于嵌入式设备
- 龙芯 (LoongArch) 是 MIPS 衍生，但需要单独支持

**建议：** ⚠️ **考虑标记为社区支持或移除**
- 几乎没有桌面/服务器用户
- 嵌入式用户通常使用专门的工具链

---

### 3.3 P2 - 边缘架构

#### 3.2.3 SPARC

| 属性 | 评估 |
|------|------|
| **头文件** | `src/m/sparc.h` |
| **当前状态** | ❌ 已停止 |
| **用户基数** | 极低 |
| **维护成本** | 低 |
| **CMake 支持** | 有限 |
| **测试可行性** | 极低 |

**详细分析：**

SPARC 曾用于：
- Sun Solaris 工作站
- 现在几乎只在遗留系统中

**建议：** ❌ **考虑移除**

---

### 3.4 P3 - 历史遗留架构（建议移除）

| 架构 | 头文件 | 建议 | 理由 |
|------|--------|------|------|
| **Alpha** | `src/m/alpha.h` | ❌ 移除 | DEC 已倒闭，无硬件 |
| **Motorola 68k** | `src/m/m68k.h` | ❌ 移除 | 极其老旧，仅复古计算 |
| **HP PA-RISC** | `src/m/hp800.h` | ❌ 移除 | HP-UX 专用，无用户 |
| **IBM RS/6000** | `src/m/ibmrs6000.h` | ❌ 移除 | 旧 POWER 架构，被 powerpc.h 覆盖 |

---

## 4. 迁移到 CMake 后的平台支持矩阵

### 4.1 建议的支持矩阵

| 平台 | 架构 | 支持等级 | CMake 优先级 | 说明 |
|------|------|----------|--------------|------|
| **Linux** | x86_64 | P0 | 最高 | 核心平台 |
| **Linux** | ARM64 | P0 | 高 | 快速增长 |
| **Linux** | x86_32 | P1 | 中 | 仍有用户但下降 |
| **Linux** | ARM32 | P1 | 中 | 嵌入式 |
| **Windows (MSVC)** | x86_64 | P0 | 高 | CMake 优势明显 |
| **Windows (MinGW)** | x86_64 | P1 | 中 | 替代方案 |
| **Windows (Cygwin)** | x86_64 | P2 | 低 | 维护成本高 |
| **macOS** | x86_64 | P0 | 高 | 重要桌面 |
| **macOS** | ARM64 (Apple Silicon) | P0 | 高 | 现代 Mac |
| **FreeBSD** | x86_64 | P1 | 中 | BSD 代表 |
| **NetBSD** | 多架构 | P2 | 低 | 社区维护 |
| **OpenBSD** | 多架构 | P2 | 低 | 社区维护 |
| **illumos** | x86_64 | P2 | 低 | 如有社区支持 |

### 4.2 建议移除的平台

#### 完全移除（无实际用户）：
- **操作系统**: IRIX, HP-UX, AIX 4.2, System V 通用, Mach BSD, GNU/Hurd
- **架构**: Alpha, M68k, PA-RISC, RS/6000, SPARC

#### 标记为"社区支持"（无核心维护）：
- **操作系统**: Solaris, Cygwin, NetBSD, OpenBSD
- **架构**: PowerPC, MIPS

---

## 5. CMake 迁移的平台处理策略

### 5.1 简化平台检测

CMake 提供了内置变量，可以大幅简化平台检测：

```cmake
# 操作系统检测
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # Linux 特定处理
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  # macOS 特定处理
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  # Windows 特定处理
  if(MINGW)
    # MinGW 特定
  elseif(CYGWIN)
    # Cygwin 特定
  else()
    # MSVC 原生
  endif()
elseif(CMAKE_SYSTEM_NAME MATCHES "BSD")
  # BSD 通用处理 (FreeBSD, NetBSD, OpenBSD)
endif()

# 架构检测
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
  # x86_64
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i[3-6]86")
  # x86 32位
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64")
  # ARM
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # ARM64/AArch64
  else()
    # ARM 32位
  endif()
endif()
```

### 5.2 消除硬编码的平台头文件

当前代码通过 `config_opsysfile` 和 `config_machfile` 包含平台特定头文件：

```c
// 当前方式 (lisp.h 中)
#ifdef config_opsysfile
#include "$srcdir/src/$opsysfile"
#endif
```

**建议迁移到 CMake 方式：**

```cmake
# CMakeLists.txt 中
if(LINUX)
  target_compile_definitions(xemacs PRIVATE LINUX)
  target_include_directories(xemacs PRIVATE src/s)
elseif(WIN32)
  target_compile_definitions(xemacs PRIVATE WINDOWSNT)
  # ...
endif()
```

或者更好的方式：**将平台特定逻辑移入 CMake 检测，消除对单独头文件的依赖**。

### 5.3 现代 POSIX 兼容

大多数现代平台都遵循 POSIX 标准。可以：
1. 优先使用 CMake 的 `check_include_file()`, `check_function_exists()` 等
2. 减少对平台特定 `#ifdef` 的依赖
3. 使用标准的 `_POSIX_C_SOURCE`, `_XOPEN_SOURCE` 等特性测试宏

### 5.4 编译器特性检测

CMake 有丰富的编译器特性检测：

```cmake
include(CheckCCompilerFlag)
include(CheckCSourceCompiles)
include(CheckTypeSize)

# 检测类型大小
check_type_size("void *" SIZEOF_VOID_P)
check_type_size("long" SIZEOF_LONG)
check_type_size("long long" SIZEOF_LONG_LONG)

# 检测编译器标志
check_c_compiler_flag("-Wall" HAVE_WALL)

# 检测内联支持
check_c_source_compiles("
  static inline int foo() { return 0; }
  int main() { return foo(); }
" HAVE_C_INLINE)
```

---

## 6. 风险与缓解

### 6.1 主要风险

| 风险 | 影响 | 缓解措施 |
|------|------|----------|
| **移除平台引发社区不满** | 中 | 清晰的沟通、渐进式弃用 |
| **CMake 检测不如 Autoconf 精确** | 中 | 编写 comprehensive 的测试，保留 fallback |
| **Windows 特定代码复杂** | 高 | 优先测试 Windows，分阶段实现 |
| **Apple Silicon 支持** | 中 | 利用 GitHub Actions 的 macOS ARM 环境 |

### 6.2 渐进式迁移策略

建议分阶段调整平台支持：

1. **阶段一（CMake 迁移初期）**
   - 保留所有当前平台，但标记弃用通知
   - 优先实现 P0 平台（Linux, Windows, macOS）

2. **阶段二（CMake 稳定后）**
   - 正式移除 P3 平台（IRIX, HP-UX, AIX 等）
   - 将 P2 平台标记为"社区支持"

3. **阶段三（长期维护）**
   - 根据实际使用情况调整 P1/P2 边界
   - 持续简化平台检测逻辑

---

## 7. 总结

### 7.1 关键结论

1. **核心平台必须保留**：
   - Linux (x86_64, ARM64)
   - Windows (MSVC, MinGW)
   - macOS (x86_64, ARM64)

2. **历史遗留平台应该移除**：
   - IRIX, HP-UX, AIX 4.2, System V, Mach
   - Alpha, M68k, PA-RISC, SPARC 等老旧架构

3. **边缘平台需要评估**：
   - BSD 系列（FreeBSD 保留，NetBSD/OpenBSD 社区维护）
   - Solaris/illumos（如有社区贡献者）
   - PowerPC, MIPS（用户基数下降）

4. **CMake 是正确的选择**：
   - 对核心平台支持优秀
   - Windows 原生支持是巨大优势
   - 可以大幅简化平台检测逻辑

### 7.2 最终建议

| 行动项 | 优先级 | 时间线 |
|--------|--------|--------|
| 实现 Linux/x86_64 的 CMake 构建 | 最高 | 立即开始 |
| 实现 Windows (MSVC) 的 CMake 构建 | 高 | 阶段一 |
| 实现 macOS 的 CMake 构建 | 高 | 阶段一 |
| 添加 ARM64 支持 | 高 | 阶段一后期 |
| 弃用通知（P3 平台） | 中 | 阶段一完成后 |
| 正式移除 P3 平台 | 中 | 阶段二 |
| 实现 FreeBSD 支持 | 中低 | 阶段二 |
| 评估 P2 平台的社区支持 | 低 | 长期 |

---

## 附录

### A. 当前平台文件清单

#### 操作系统文件 (src/s/)

| 文件名 | 平台 | 建议 |
|--------|------|------|
| `linux.h` | Linux | ✅ 保留 |
| `netbsd.h` | NetBSD | ⚠️ 社区支持 |
| `openbsd.h` | OpenBSD | ⚠️ 社区支持 |
| `mingw32.h` | MinGW | ✅ 保留 |
| `windowsnt.h` | Windows Native | ✅ 保留 |
| `win32-native.h` | Windows 通用 | ✅ 保留 |
| `win32-common.h` | Windows 通用 | ✅ 保留 |
| `cygwin32.h` | Cygwin | ⚠️ 社区支持/考虑移除 |
| `cygwin64.h` | Cygwin 64位 | ⚠️ 同上 |
| `cygwin-common.h` | Cygwin 通用 | ⚠️ 同上 |
| `sol2.h` | Solaris 2 | ⚠️ 社区支持/考虑移除 |
| `hpux11.h` | HP-UX 11 | ❌ 移除 |
| `hpux11-shr.h` | HP-UX 11 共享 | ❌ 移除 |
| `irix6-5.h` | IRIX 6.5 | ❌ 移除 |
| `aix4-2.h` | AIX 4.2 | ❌ 移除 |
| `gnu.h` | GNU/Hurd | ❌ 移除 |
| `bsd-common.h` | BSD 通用 | ✅ 保留（供 FreeBSD 等使用） |
| `mach-bsd4-3.h` | Mach BSD | ❌ 移除 |
| `usg5-4.h` | System V Release 4 | ❌ 移除 |
| `usg5-4-2.h` | System V R4.2 | ❌ 移除 |

#### 架构文件 (src/m/)

| 文件名 | 架构 | 建议 |
|--------|------|------|
| `intel386.h` | x86/x86_64 | ✅ 保留 |
| `arm.h` | ARM 32位 | ✅ 保留并扩展 |
| `powerpc.h` | PowerPC | ⚠️ 社区支持 |
| `mips.h` | MIPS | ⚠️ 社区支持/考虑移除 |
| `sparc.h` | SPARC | ❌ 移除 |
| `alpha.h` | Alpha | ❌ 移除 |
| `m68k.h` | Motorola 68k | ❌ 移除 |
| `hp800.h` | PA-RISC | ❌ 移除 |
| `ibmrs6000.h` | RS/6000 | ❌ 移除 |
| `windowsnt.h` | Windows | ✅ 保留（与平台共享） |
| `template.h` | 模板 | ⚠️ 保留作为参考 |

### B. CMake 平台检测参考

```cmake
# CMake 内置平台变量

# 操作系统
CMAKE_SYSTEM_NAME       # "Linux", "Darwin", "Windows", "FreeBSD", etc.
CMAKE_SYSTEM_VERSION    # 操作系统版本
CMAKE_HOST_SYSTEM_NAME  # 构建主机的操作系统

# 架构
CMAKE_SYSTEM_PROCESSOR  # "x86_64", "amd64", "i386", "arm", "aarch64", etc.
CMAKE_SIZEOF_VOID_P     # 4 (32位) or 8 (64位)

# 编译器
CMAKE_C_COMPILER_ID     # "GNU", "Clang", "MSVC", "Intel", etc.
CMAKE_C_COMPILER_VERSION # 编译器版本

# Windows 特定
WIN32                   # 任何 Windows 平台
MSVC                    # Microsoft Visual C++
MINGW                   # MinGW
CYGWIN                  # Cygwin

# macOS 特定
APPLE                   # 任何 Apple 平台
IOS                     # iOS (CMake 3.14+)

# BSD 特定
CMAKE_SYSTEM_NAME MATCHES "BSD" # 匹配 FreeBSD, NetBSD, OpenBSD
```

### C. 参考资源

- CMake 平台检测文档: https://cmake.org/cmake/help/latest/manual/cmake-variables.7.html
- CMake 编译器检测: https://cmake.org/cmake/help/latest/module/CheckCCompilerFlag.html
- CMake 类型大小检测: https://cmake.org/cmake/help/latest/module/CheckTypeSize.html
