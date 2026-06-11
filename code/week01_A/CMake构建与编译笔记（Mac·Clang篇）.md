# CMake 构建与编译笔记（Mac · Clang 篇，以 week01_A 为例）

这篇用本工程的真实文件，把"敲 `cmake` 两行命令"背后发生的事拆开讲清楚：
**配置 → 编译 → 链接** 三步，CMake 各自在哪、生成了什么、为什么断点需要 `-g`。

> Windows / Visual Studio 2026 的对应过程见姊妹篇 **[CMake构建与编译笔记（Win·VS2026篇）.md](CMake构建与编译笔记（Win·VS2026篇）.md)**。
> 两篇结构一一对应，方便横向对照两个平台的差异（clang↔MSVC、`.o`↔`.obj`、`-g`↔`/Zi` 等）。

---

## 0. 一句话直觉

> CMake 本身**不编译代码**。它是个"翻译官"：读你写的 `CMakeLists.txt`，
> 针对你电脑上的编译器和构建工具，**生成一套构建脚本**（Makefile / Ninja / VS 工程）。
> 真正干活的是编译器（`clang++` / MSVC）和构建工具（make / ninja）。

所以全程分两个独立阶段，命令也是两条：

```bash
cmake -S . -B build              # ① 配置(configure)：生成构建脚本
cmake --build build              # ② 构建(build)：调编译器干活
```

---

## 1. 为什么用 CMake，而不是手敲 clang++

本工程手动编译 `test_bezier` 其实就一行（这正是 CMake 最终替你跑的）：

```bash
clang++ -g -std=c++17 -I. -Wall -Wextra test_bezier.cpp -o test_bezier
```

三个 target、两个平台（Mac 的 clang / Windows 的 MSVC，编译选项完全不同）、
还要管头文件依赖、输出目录、Debug/Release 切换……手敲会失控。
CMake 让你**只描述"要什么"**（见 `CMakeLists.txt`），**怎么调编译器交给它**。

---

## 2. 两个阶段拆解

### 阶段① 配置 configure：`cmake -S . -B build`

- `-S .`：源码目录（含 `CMakeLists.txt`）= 当前目录
- `-B build`：构建目录 = `build/`（**out-of-source build**，产物全扔进 `build/`，源码目录保持干净，这也是 `.gitignore` 忽略 `build/` 的原因）

这一步 CMake 做的事：

1. 探测编译器（找到 `/usr/bin/clang++`，AppleClang 21）
2. 读 `CMakeLists.txt`，按你的平台展开所有选项
3. 把探测结果 + 你的设置**缓存**进 `build/CMakeCache.txt`
4. 生成构建脚本（本工程是 `Unix Makefiles` 生成器 → 生成 `Makefile`）

> 配置只需跑一次。之后改了 `CMakeLists.txt`，下次 `cmake --build` 会自动重新配置。

### 阶段② 构建 build：`cmake --build build`

CMake 调用底层构建工具（这里是 `make`），按生成的脚本：

```
test_bezier.cpp  --(编译)-->  test_bezier.cpp.o  --(链接)-->  bin/test_bezier
```

`cmake --build build` 是跨平台的统一入口，等价于在 `build/` 里跑 `make`
（Windows 上则等价于调 MSBuild / ninja），不用记每个平台的构建命令。

---

## 2.5 不用手输入：VSCode F5 已自动跑那两句

上面那两句 `cmake` 命令，**Mac 下也不需要每次手敲**。本工程自带
[`.vscode/tasks.json`](.vscode/tasks.json) 和 [`.vscode/launch.json`](.vscode/launch.json)，
在 VSCode 里按一下 **F5** 就会按序把它们跑完。这条链路是：

```
按 F5
  └─ launch.json: preLaunchTask = "build-debug"
       └─ tasks.json: build-debug  →  cmake --build build           （= 构建那句）
            └─ dependsOn: cmake-configure-debug
                 →  cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug   （= 配置那句）
```

所以 **F5 = 先配置、再构建、再启动 CodeLLDB 调试**，正是本篇 §0 / §8 那两句命令，外加自动起调试器。
（也可单独按 **Cmd+Shift+B** 只跑默认构建任务 `build-debug`，同样会先触发配置。）

> 📌 **和 Windows 的区别**：Mac 这套走的是 **VSCode 的 tasks/launch 机制**（VSCode 专属），
> 不依赖 `CMakePresets.json`。本工程根目录虽然有 [`CMakePresets.json`](CMakePresets.json)，
> 但里面的预设都带 `hostSystemName == Windows` 条件，**在 Mac 上自动隐藏、不生效** ——
> Mac 维持 tasks.json 链路不变。
>
> 想在 Mac 上也用预设？可装 **CMake Tools** 扩展、自己加一个 Ninja / Makefile 预设，
> 但那不是本工程默认；默认仍是 **tasks.json + CodeLLDB** 这条最省心的路。

---

## 3. 三个必须分清的概念

| 概念 | 是什么 | 本工程取值 | 怎么改 |
|---|---|---|---|
| **生成器 Generator** | 决定生成哪种构建脚本 | `Unix Makefiles`（Mac） | `-G "Ninja"` / `-G "Visual Studio 17 2022"` |
| **编译器 / Kit** | 实际编译的工具 | `/usr/bin/clang++` | 配置时由环境探测，或选 Kit |
| **构建类型 Build Type** | Debug 还是 Release | `Debug`（带 `-g -O0`） | `-DCMAKE_BUILD_TYPE=Release` |

⚠️ **生成器 ≠ 编译器**，这是最容易混的：
- 生成器 = "用 Makefile 还是 Ninja 还是 VS 工程来组织构建"
- 编译器 = "用 clang 还是 MSVC 把 .cpp 变成 .o"
- 比如 Windows 上可以是 `Ninja` 生成器 + `MSVC` 编译器。

⚠️ **生成器决定有没有 `compile_commands.json`**：只有 Ninja / Makefile 生成器产出它，
Visual Studio 生成器不产出。clangd 靠它工作——这就是 README 里 Windows 要切 Ninja 的原因。

---

## 4. 逐行读懂本工程 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)         # 要求的最低 CMake 版本
project(week01_A VERSION 1.0 LANGUAGES CXX)  # 工程名 + 只用 C++（触发编译器探测）
```

```cmake
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)        # 产出 compile_commands.json（喂给 clangd）
```

```cmake
set(CMAKE_CXX_STANDARD 17)                   # 用 C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)          # 不允许降级
set(CMAKE_CXX_EXTENSIONS OFF)                # 用 -std=c++17 而非 -std=gnu++17（更可移植）
```

```cmake
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)  # 三个 exe 统一进 build/bin/
```

```cmake
if(MSVC)                                      # —— Windows / MSVC 分支 ——
    add_compile_options(/W4 /utf-8 /permissive- /Zc:__cplusplus /MP)
    add_compile_definitions(_USE_MATH_DEFINES NOMINMAX _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
else()                                        # —— Mac/Linux / clang・gcc 分支 ——
    add_compile_options(-Wall -Wextra)        # 高警告；注意这里不再硬塞 -O2
    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)  # 默认 Debug → 断点能命中
    endif()
endif()
```

> 📌 这里就是之前"断点跟不进"的修复点：原来写死了 `-O2`，把 `-g` 顶没了。
> 现在让 `CMAKE_BUILD_TYPE` 来驱动优化级别——Debug 给 `-g`（含隐式 `-O0`），Release 才给 `-O3 -DNDEBUG`。

```cmake
add_library(geo_common INTERFACE)                                    # 纯头文件"库"
target_include_directories(geo_common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})  # 谁链它谁就能 #include "geo.h"
```

> **INTERFACE 库**：`geo.h` 没有 `.cpp`，编不出 `.o`，所以不是真正会被编译的库。
> `INTERFACE` 表示"我没有要编译的东西，只把头文件搜索路径这个**使用要求**传给链接我的人"。

```cmake
set(TEST_TARGETS test_bezier test_bspline test_nurbs)
foreach(target ${TEST_TARGETS})
    add_executable(${target} ${target}.cpp)                  # 每个 .cpp 编一个可执行
    target_link_libraries(${target} PRIVATE geo_common)      # 继承 geo_common 的 include 路径
    set_target_properties(${target} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target}>"  # VS 调试工作目录=exe 所在（SVG 好找）
        FOLDER "Tests")
endforeach()
```

```cmake
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT test_bezier)  # VS 里 F5 默认跑哪个
```

---

## 5. 配置完，build/ 里生成了什么

```
build/
├── CMakeCache.txt          ← 配置缓存：编译器路径、BUILD_TYPE、所有选项（改它=改配置）
├── compile_commands.json   ← 每个 .cpp 的完整编译命令（clangd 的"地图"）
├── Makefile                ← 顶层构建脚本（make 的入口）
├── cmake_install.cmake     ← install 阶段用（本工程没用到）
├── bin/                    ← 最终产物：test_bezier / test_bspline / test_nurbs + 运行时产的 *.svg
└── CMakeFiles/
    └── test_bezier.dir/    ← 这个 target 的中间产物
        ├── flags.make      ← 编译用的标志（见下）
        ├── link.txt        ← 链接命令（见下）
        ├── test_bezier.cpp.o   ← 编译出的目标文件（中间产物）
        └── *.d / depend.*  ← 头文件依赖追踪（geo.h 改了会触发重编）
```

**`flags.make`** —— 编译阶段实际用的标志（本工程 Debug）：

```make
CXX_DEFINES  =
CXX_INCLUDES = -I/.../week01_A
CXX_FLAGS    = -g -std=c++17 -arch arm64 -Wall -Wextra
```

注意有 `-g`、没有 `-O*`（Debug 隐式 `-O0`）——**断点能命中的根本原因就在这一行**。

**`link.txt`** —— 链接阶段命令：

```bash
/usr/bin/clang++ -g -arch arm64 -Wl,-search_paths_first ... test_bezier.cpp.o -o bin/test_bezier
```

**`compile_commands.json`** —— 每个源文件一条记录，clangd 读它来精确还原你工程的编译环境，
从而给出正确的补全 / 跳转 / 报错（否则它只能瞎猜头文件路径和标准）。

---

## 6. 一次编译到底发生了什么

`test_bezier.cpp` → `bin/test_bezier`，clang 内部走四步（前三步合称"编译"）：

```
test_bezier.cpp
   │  ① 预处理 (Preprocess)   展开 #include "geo.h"、#include <cmath>，处理 #ifdef _WIN32 等宏
   ▼
（展开后的纯 C++ 源）
   │  ② 编译 (Compile)        语法/语义分析，生成目标平台汇编（arm64）
   ▼
（汇编代码）
   │  ③ 汇编 (Assemble)       汇编 → 机器码，产出 test_bezier.cpp.o（目标文件）
   ▼
test_bezier.cpp.o
   │  ④ 链接 (Link)           把 .o 和需要的库符号拼成可执行文件；解析 std::cout 等外部符号
   ▼
bin/test_bezier   ✅ 可执行
```

对本工程的具体说明：

- 第①步：`geo.h` 是头文件，`#pragma once` 保证只展开一次；`enable_utf8_console()` 在 Mac 下因 `#ifndef _WIN32` 编成空函数。
- 第②③步：`-g` 让编译器把**源码行号 ↔ 机器码地址**的映射写进 `.o`（DWARF 调试信息）。这就是调试器能"停在第 39 行"的依据。
- 第④步：`geo_common` 是纯头库，没贡献任何 `.o`；真正链接进来的是 C++ 标准库（`std::ofstream`、`std::cout`、`std::filesystem` 等）。三个 target 各自独立链接。

> **为什么 `-O2` 会毁掉调试体验**：优化会内联函数、重排/删除指令、把变量放进寄存器。
> 于是"第 39 行"可能根本不存在对应的连续机器码，断点无处可落，单步乱跳，变量显示 `<optimized out>`。
> 所以**调试用 Debug（`-g -O0`），测性能才用 Release（`-O3`）**。

---

## 7. Debug vs Release 速查

| | Debug（本工程默认） | Release |
|---|---|---|
| 优化 | `-O0`（不优化） | `-O3`（激进优化） |
| 调试符号 | `-g`（有） | 默认无 |
| 宏 | — | `-DNDEBUG`（关掉 `assert`） |
| 断点 / 单步 | ✅ 准确 | ❌ 乱跳、变量看不全 |
| 运行速度 | 慢 | 快 |
| 用途 | 写代码、查 bug、跟断点 | 跑性能、发布 |

切换：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release    # 重新配置成 Release
cmake --build build
```

> 单配置生成器（Makefiles/Ninja）的 BUILD_TYPE 在**配置时**定死；
> 多配置生成器（Visual Studio）则在**构建时**用 `--config Debug/Release` 选，所以 Windows 产物在 `bin/<Config>/` 下。

---

## 8. 常用命令速查

```bash
# 配置（Mac，默认 Debug）
cmake -S . -B build -G "Unix Makefiles"

# 构建全部
cmake --build build

# 只构建一个 target
cmake --build build --target test_bezier

# 并行构建（用满 CPU）
cmake --build build -j

# 切 Release
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build

# 彻底清理（最干净的"重来"方式）
rm -rf build

# 看某个 target 实际的编译标志 / 链接命令
cat build/CMakeFiles/test_bezier.dir/flags.make
cat build/CMakeFiles/test_bezier.dir/link.txt
```

---

## 9. 名词对照表

| 名词 | 含义 |
|---|---|
| configure / 配置 | `cmake -S -B`，读 CMakeLists 生成构建脚本，不编译 |
| build / 构建 | `cmake --build`，调编译器真正编译链接 |
| generator / 生成器 | 生成哪种构建脚本：Unix Makefiles / Ninja / Visual Studio |
| target / 目标 | 一个构建产物：可执行文件或库（本工程的三个 test_* + geo_common） |
| build type / 构建类型 | Debug / Release，决定优化与调试符号 |
| object file / 目标文件 | `.o`，单个源文件编译后的中间产物 |
| linking / 链接 | 把多个 `.o` 和库拼成最终可执行文件 |
| `-g` | 写入调试符号（DWARF），调试器靠它映射源码行 ↔ 机器码 |
| `compile_commands.json` | 每个源文件的编译命令清单，clangd 的工作依据 |
| INTERFACE library | 无源码的"库"，只向使用者传递头文件路径等使用要求（如 geo_common） |
| out-of-source build | 产物全放独立的 `build/`，不污染源码目录 |

---

## 相关

- 工程使用说明、断点调试操作步骤见 [README.md](README.md)
- Windows / VS2026 对应过程见 [CMake构建与编译笔记（Win·VS2026篇）.md](CMake构建与编译笔记（Win·VS2026篇）.md)
- VSCode 调试技巧（变量增强 / watchpoint）见 [VSCode调试技巧笔记.md](VSCode调试技巧笔记.md)
- 调试器配置见 `.vscode/launch.json`、`.vscode/tasks.json`
