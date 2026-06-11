# 第 1 周补充：CMakeLists 到底在做什么

> 浓缩自仓库内两篇完整笔记：
> [CMake构建与编译笔记（Mac·Clang篇）](https://github.com/liyongzheng666/geometry-engineer-ai-road/blob/main/code/week01_A/CMake构建与编译笔记（Mac·Clang篇）.md) ·
> [CMake构建与编译笔记（Win·VS2026篇）](https://github.com/liyongzheng666/geometry-engineer-ai-road/blob/main/code/week01_A/CMake构建与编译笔记（Win·VS2026篇）.md)
> 这里只讲贯穿三周工程的主干概念；想看逐行拆解去读原文。

## 一句话直觉

> **CMake 不编译代码。** 它读 `CMakeLists.txt`，生成一套构建脚本
> （Makefile / Ninja / VS 解决方案）；真正干活的是编译器
> （clang++ / cl.exe）和链接器，由 make / ninja / MSBuild 驱动。

所以永远是两步：

```bash
cmake -S . -B build        # ① 配置：探测工具链 → 生成构建脚本到 build/
cmake --build build        # ② 构建：调底层工具把 .cpp 变成可执行文件
```

## 三个最容易混的概念

| 概念 | 是什么 | 本工程取值 |
|---|---|---|
| **生成器 Generator** | 生成哪种构建脚本 | Mac: Unix Makefiles；Win: Visual Studio 或 Ninja |
| **编译器 Compiler** | 真正把 .cpp 变 .o/.obj 的程序 | AppleClang / MSVC cl.exe |
| **构建类型 Config** | Debug 还是 Release | Mac 配置时定（单配置）；Win 构建时 `--config` 选（多配置） |

⚠️ **生成器 ≠ 编译器**：完全可以"Ninja 生成器 + MSVC 编译器"——
这正是 Windows 下给 clangd 产 `compile_commands.json` 的组合
（VS 生成器不产这个文件）。

## 逐段读 week01_A 的 CMakeLists.txt

### 1. 工程声明与 C++ 标准

```cmake
cmake_minimum_required(VERSION 3.20)
project(week01_A LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)   # 不满足就报错，而不是悄悄降级
set(CMAKE_CXX_EXTENSIONS OFF)         # 用 -std=c++17 而非 gnu++17
```

### 2. 输出目录统一

```cmake
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
```

所有 exe 收进 `build/bin/`（Windows 多配置生成器下是 `build/bin/<Config>/`）。
运行时生成的 SVG 落在 exe 旁边，好找。

### 3. MSVC 分支：把常见坑提前堵死

```cmake
if(MSVC)
    add_compile_options(/W4 /utf-8 /permissive- /Zc:__cplusplus /MP)
    add_compile_definitions(_USE_MATH_DEFINES NOMINMAX
                            _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
endif()
```

| 选项/宏 | 不加会怎样 |
|---|---|
| `/utf-8` | 中文注释报 C4819、控制台乱码 |
| `/permissive-` | MSVC 放过不合标准的写法，跨平台埋雷 |
| `_USE_MATH_DEFINES` | `M_PI` 未定义（MSVC 默认不给） |
| `NOMINMAX` | windows.h 的 min/max 宏顶掉 `std::min/max` |

Mac/Linux 走 else 分支：`-Wall -Wextra`，并且**默认 Debug**
（带 `-g -O0`，VSCode 断点才能命中）。

### 4. INTERFACE 库：纯头文件的正确姿势

```cmake
add_library(geo_common INTERFACE)
target_include_directories(geo_common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
```

`geo.h` 没有 .cpp，所以建一个 INTERFACE 库——它不产生任何编译产物，
只把"头文件搜索路径"作为**使用要求**传给链接它的 target。
三个测试 target 各自 `target_link_libraries(... PRIVATE geo_common)` 即可。

### 5. foreach 批量建 target

```cmake
set(TEST_TARGETS test_bezier test_bspline test_nurbs)
foreach(target ${TEST_TARGETS})
    add_executable(${target} ${target}.cpp)
    target_link_libraries(${target} PRIVATE geo_common)
    set_target_properties(${target} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target}>"
        FOLDER "Tests")
endforeach()
```

`VS_*` 属性只对 VS 生成器生效：F5 调试时工作目录设到 exe 所在地
（SVG 不会丢），三个 target 收进 VS 里的 "Tests" 文件夹。Mac 下被忽略，无害。

## CMakePresets.json：把命令行参数固化成名字

`CMakeLists.txt` 描述**工程怎么构建**；`CMakePresets.json` 描述**用什么参数调 CMake**。
有了它，VS"打开文件夹"、命令行 `cmake --preset windows-vs2026`、VSCode CMake Tools
跑的都是同一套参数——团队里每个人产物都落在同一个 `build/`。

```text
configurePresets  → 固化 cmake -S . -B build -G "..." -A x64
buildPresets      → 固化 cmake --build build --config Debug|Release
```

⚠️ 换生成器（VS ↔ Ninja）之前先删 `build/`，否则 CMake 报
"生成器与已有缓存不一致"。

## Debug vs Release 速查

| | Debug | Release |
|---|---|---|
| 优化 | `-O0` / `/Od` | `-O2`+ |
| 调试符号 | `-g` / `/Zi`(.pdb) | 默认无 |
| 断点/单步 | ✅ 准确 | ❌ 内联重排后乱跳、变量 `<optimized out>` |
| 用途 | 写代码、跟断点 | 测性能、发布 |

**为什么 O2 毁调试**：优化会内联、重排、删指令、把变量塞寄存器——
断点无处可落。所以**调试用 Debug，测性能才切 Release**。
