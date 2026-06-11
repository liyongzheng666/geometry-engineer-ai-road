# CMake 构建与编译笔记（Win · VS2026 篇，以 week01_A 为例）

这篇是 **[CMake构建与编译笔记（Mac·Clang篇）.md](CMake构建与编译笔记（Mac·Clang篇）.md)** 的姊妹篇，
结构一一对应，只是把平台换成 **Windows + Visual Studio 2026（18.x）+ MSVC**：
同一份 `CMakeLists.txt`，在 Windows 下 **配置 → 编译 → 链接** 各自变成了什么、生成了什么、为什么断点要 `/Zi`。

---

## 0. 一句话直觉

> CMake 在 Windows 上同样**不编译代码**。它读 `CMakeLists.txt`，针对 VS 的工具链
> **生成一套 VS 工程（.sln / .vcxproj）或 Ninja 脚本**；真正干活的是
> **MSVC 编译器 `cl.exe`** 和 **链接器 `link.exe`**，由 **MSBuild** 或 **Ninja** 驱动。

和 Mac 的最大不同就一句话：**Visual Studio 不是编译器，它是"IDE + MSVC + MSBuild + CMake + Ninja"的打包**。
你在 VS 里点的每个按钮，底层都还是上面那几个工具在跑。

---

## 1. VS2026 与 CMake 的三种用法

Windows 下同一个工程有三条路，**底层都是 CMake**，区别只是谁来调它：

| 方式 | 谁调 CMake | 生成器 | 产物位置 |
|---|---|---|---|
| **A. VS 直接"打开文件夹"** | VS 内置 CMake | 读本工程 `CMakePresets.json` → **VS 生成器** | `build/bin/<Config>/` |
| **B. 命令行 `cmake`** | 你自己 | **Visual Studio 18 2026** | `build/bin/<Config>/` |
| **C. VSCode + CMake Tools** | CMake Tools 扩展 | 建议 **Ninja** | `build/` |

> 📌 **本工程带了 [`CMakePresets.json`](CMakePresets.json)**，所以方式 A（VS 打开文件夹）会读它、
> 用 **VS 生成器**配置到 `build/`，和方式 B **完全一致**（都生成 `.sln`、产物都在 `build/bin/<Config>/`）——
> 详见下面第 1.5 节。
> （若**没有**这个预设文件，VS 会退回默认的 **Ninja + 内部预设**，产物落在 `out/build/<preset>/bin/`。）

---

## 1.5 不用手输入：IDE 打开项目 = 那两句命令

第 2 / 8 节那两句 `cmake` 命令，**Windows 下并不需要你手动敲进命令行**。它们是"方式 B（命令行）"
的底层动作；用 VS 打开项目时，VS 内置的 CMake 会自动替你跑等价的"配置 + 构建"。区别只在于跑的是哪一套：

- **没有预设文件时**，VS"打开文件夹"默认用 **Ninja + 它自己的内部预设**，配置进 `out/build/<preset>/` ——
  这跑的*不是*文档里那两句 VS 生成器命令，产物位置也对不上。
- **本工程已附带 [`CMakePresets.json`](CMakePresets.json)**，把"生成器 / 架构 / 构建目录 / 配置"固化成预设（这个文件每个字段在说什么，见下方 **1.6 节**逐行拆解）。
  VS 打开文件夹就读它，于是"点按钮"和"手敲那两句"变成同一件事：

| 你在 VS 里的动作 | 底层实际执行（= 文档第 2 / 8 节那两句） |
|---|---|
| 选配置预设 `windows-vs2026` → 自动配置 | `cmake -S . -B build -G "Visual Studio 18 2026" -A x64` |
| 选构建预设 `vs2026-debug` / `生成 → 全部生成` | `cmake --build build --config Debug` |
| 选构建预设 `vs2026-release` | `cmake --build build --config Release` |

产物都回到 `build\bin\<Config>\`，和方式 B 一致。

**在 VS 里怎么用：**

1. VS 打开 `week01_A` 文件夹 → 顶部"配置"下拉自动出现 `windows-vs2026`（还有 `windows-vs2022`、`windows-ninja`）。
2. 选中即触发配置（生成 `build\week01_A.sln`）；在"构建预设"里选 `vs2026-debug` / `vs2026-release` 切 Debug ↔ Release。
3. F5 调试、Ctrl+F5 直接运行，和原来一样。

> 💡 **要给 clangd 用 `compile_commands.json`？** 改选 `windows-ninja` 预设 —— VS 生成器不产这个文件
> （见第 3 节末的⚠️、README 方法 C），Ninja 生成器才产。
>
> ⚠️ **切换预设 / 生成器前，若 `build/` 是用别的生成器配过的，先删掉 `build/`**，
> 否则 CMake 会报"生成器与已有缓存不一致"。

---

## 1.6 逐行读懂本工程 CMakePresets.json（预设文件本身在说什么）

第 1.5 节说"VS 打开文件夹 = 自动跑那两句命令"，靠的就是这个 [`CMakePresets.json`](CMakePresets.json)。
它的作用一句话：**把第 2 / 8 节里那些 `-G`、`-A`、`--config` 参数，固化成一个个带名字的"预设"**。
于是命令行能 `cmake --preset xxx` 一把梭，IDE 也能在下拉里直接认出来 —— 团队里每个人、每个工具跑的都是同一套参数，不再靠手敲、不再各记各的。

> 📌 它和 `CMakeLists.txt` 是两回事，别混：
> `CMakeLists.txt` 描述**这个工程怎么构建**（有哪些源文件、库、编译选项）；
> `CMakePresets.json` 描述**用什么参数去调 CMake**（哪个生成器、哪种架构、构建目录、哪个配置）。
> 前者是"要盖什么楼"，后者是"用哪套命令开工"。

文件分两大块，正好对应第 2 节的**两个阶段**：

| 顶层字段 | 对应阶段 | 固化的是哪句命令 |
|---|---|---|
| `configurePresets` | 阶段① 配置 | `cmake -S . -B build -G ... -A ...` 的参数 |
| `buildPresets` | 阶段② 构建 | `cmake --build build --config ...` 的参数 |

**开头两行 —— 版本与门槛：**

| 字段 | 取值 | 含义 |
|---|---|---|
| `version` | `3` | 预设文件的 schema 版本；v3 才支持下面的 `condition` 写法 |
| `cmakeMinimumRequired` | `3.21.0` | 用这套预设至少要 CMake 3.21（`CMakePresets.json` 始于 3.19，`condition` 始于 3.21） |

### configurePresets：四个"配置阶段"预设

**`windows-base`（隐藏基类，只供继承，自己不能直接选）：**

| 字段 | 取值 | 含义 |
|---|---|---|
| `hidden` | `true` | 不在 IDE 下拉 / `--list-presets` 里露面；只作为别人 `inherits` 的公共基类 |
| `binaryDir` | `${sourceDir}/build` | 构建目录 = 源码目录下的 `build/`（`${sourceDir}` 是 CMake 内置变量）—— **这就是产物落在 `build/` 而非默认 `out/build/` 的根因** |
| `condition` | `${hostSystemName}` 等于 `Windows` | 仅当宿主系统是 Windows 才启用；在 Mac/Linux 上这几个预设自动隐藏，防止误选 |

> 把 `binaryDir` 和 `condition` 抽进 base，下面三个预设 `inherits` 它就**不用各写一遍** —— 这正是 `hidden` 基类的用途：抽公共字段、给别人继承。

**三个能直接用的配置预设（都 `inherits: windows-base`，所以也都落在 `build/`、也都只在 Windows 出现）：**

| 预设名 | generator | architecture | 等价命令 | 备注 |
|---|---|---|---|---|
| `windows-vs2026` | `Visual Studio 18 2026` | `x64`（`strategy:set` → `-A x64`） | `cmake -S . -B build -G "Visual Studio 18 2026" -A x64` | 主力 |
| `windows-vs2022` | `Visual Studio 17 2022` | `x64` | `cmake -S . -B build -G "Visual Studio 17 2022" -A x64` | 退一档 VS |
| `windows-ninja` | `Ninja Multi-Config` | （无） | `cmake -S . -B build -G "Ninja Multi-Config"` | 产 `compile_commands.json` 给 clangd |

> - `displayName` / `description` 只是 IDE 下拉里显示的友好名 + 悬停说明（本文件干脆把等价命令写进了 `description`，方便对照）。
> - `architecture.strategy: "set"` 表示"通过生成器的 `-A` 开关来设架构"，所以它就等价于命令行 `-A x64`。VS 生成器强烈建议显式写架构（见第 2 节，否则可能默认成 Win32）。
> - `windows-ninja` **没有 `architecture` 字段**：Ninja 不吃 `-A`，架构由你所在的命令行环境（x64 Native Tools 命令行）决定 —— 这也对应第 3 节"Ninja 生成器 + MSVC 编译器"的组合。

### buildPresets：六个"构建阶段"预设

每个构建预设把"**用哪个配置预设** + **构建哪个配置（Debug/Release）**"绑成一个名字。
因为三个配置预设的 `binaryDir` 都是 `build/`，所以六个构建预设的底层命令统一是 `cmake --build build --config <Debug|Release>`，区别只在于**前提**（先跑过哪个 configurePreset）：

| 构建预设 | configurePreset（前提：先配置好它） | configuration | 底层命令 |
|---|---|---|---|
| `vs2026-debug` | `windows-vs2026` | `Debug` | `cmake --build build --config Debug` |
| `vs2026-release` | `windows-vs2026` | `Release` | `cmake --build build --config Release` |
| `vs2022-debug` | `windows-vs2022` | `Debug` | `cmake --build build --config Debug` |
| `vs2022-release` | `windows-vs2022` | `Release` | `cmake --build build --config Release` |
| `ninja-debug` | `windows-ninja` | `Debug` | `cmake --build build --config Debug` |
| `ninja-release` | `windows-ninja` | `Release` | `cmake --build build --config Release` |

> - `configurePreset` 字段指明"这条构建建立在哪套配置之上" —— **必须先配置、再构建**，这跟第 2 节两阶段的先后顺序是同一回事。
> - `configuration` 对应多配置生成器构建时的 `--config`：Debug / Release **构建时**选、不在配置时定（见第 7 节那条⚠️）。
> - 于是命令行就能两句搞定：`cmake --preset windows-vs2026` 配置，再 `cmake --build --preset vs2026-debug` 构建 —— 这正是第 8 节开头那几句。
> - ⚠️ 三套配置预设共用同一个 `build/`，**不能同时存在**（VS2026 / VS2022 / Ninja 的缓存会打架）。换预设前先删 `build/`，同第 1.5 节末尾那条提醒。

---

## 2. 两个阶段拆解（以方式 B 命令行为例）

和 Mac 一样，全程分**配置**和**构建**两步，命令几乎相同，只是构建时多一个 `--config`：

```cmd
cmake -S . -B build -G "Visual Studio 18 2026" -A x64   :: ① 配置：生成 .sln + .vcxproj
cmake --build build --config Debug                       :: ② 构建：调 MSBuild → cl.exe
```

### 阶段① 配置 configure

- `-G "Visual Studio 18 2026"`：用 VS2026 生成器（不写则用机器上最新的 VS）
- `-A x64`：目标架构 64 位（VS 生成器**强烈建议显式写**，否则可能默认成 Win32）
- CMake 做的事：探测 MSVC 工具集 → 读 `CMakeLists.txt` 走 `if(MSVC)` 分支 → 缓存进 `build/CMakeCache.txt` → 生成 `week01_A.sln` 和每个 target 的 `.vcxproj`

### 阶段② 构建 build

`cmake --build build --config Debug` 调用 **MSBuild**，按 `.vcxproj` 把 `.cpp` 编成 `.obj` 再链成 `.exe`。

> ⚠️ **VS 生成器是"多配置"的**：Debug / Release **在构建时用 `--config` 选**，不在配置时定。
> 这跟 Mac 的 Unix Makefiles（单配置，配置时 `-DCMAKE_BUILD_TYPE=` 定死）正好相反 —— 见第 7 节。
>
> 💡 `cl.exe` / `link.exe` 需要一堆环境变量（`INCLUDE` / `LIB` / `PATH`）。所以命令行方式要在
> **"x64 Native Tools Command Prompt for VS 2026"** 里跑（开始菜单搜得到）；
> 方式 A/C 由 VS / CMake Tools 自动注入，不用管。

---

## 3. 三个必须分清的概念（Windows 版）

| 概念 | 是什么 | 本工程取值 | 怎么改 |
|---|---|---|---|
| **生成器 Generator** | 生成哪种构建脚本 | `Visual Studio 18 2026`（多配置）或 `Ninja` | `-G "Ninja"` |
| **工具集 / 编译器** | 实际编译的 MSVC 版本 | VS2026 自带的 MSVC（`cl.exe`） | VS 安装器里装/选工具集 |
| **构建类型 Config** | Debug 还是 Release | **构建时**用 `--config` 选 | `--config Release` |

⚠️ **生成器 ≠ 编译器**（和 Mac 同理）：
- 生成器 = "用 VS 解决方案 还是 Ninja 来组织构建"
- 编译器 = "用 `cl.exe` 把 `.cpp` 变成 `.obj`"
- 完全可以是 **Ninja 生成器 + MSVC 编译器**（这正是 VS"打开文件夹"和 clangd 想要的组合）。

⚠️ **只有 Ninja 生成器产 `compile_commands.json`，VS 生成器不产**。
clangd 靠它工作 —— 这就是 README 里"Windows 用 clangd 要切 Ninja"的原因。

---

## 4. 逐行读懂本工程 CMakeLists.txt 的 MSVC 分支

跨平台部分（工程名、C++17、`bin/` 输出目录、`geo_common` INTERFACE 库）两个平台**完全共用**，
见 Mac 篇第 4 节。这里只看 Windows 专属的 `if(MSVC)` 分支：

```cmake
if(MSVC)
    add_compile_options(/W4 /utf-8 /permissive- /Zc:__cplusplus /MP)
    add_compile_definitions(_USE_MATH_DEFINES NOMINMAX _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
endif()
```

**编译选项 `add_compile_options`：**

| 选项 | 作用 | 不加会怎样 |
|---|---|---|
| `/W4` | 高警告级别 | 漏掉潜在问题 |
| `/utf-8` | 源码 + 执行字符集都按 UTF-8 | 中文源码报 `C4819`、`std::cout` 中文乱码 |
| `/permissive-` | 严格遵循 C++ 标准 | MSVC 放过一些不合标准的写法，跨平台埋雷 |
| `/Zc:__cplusplus` | 让 `__cplusplus` 宏报真实值 | MSVC 默认永远报 `199711L`，版本判断失效 |
| `/MP` | 多进程并行编译 | 多文件编译变慢 |

**预处理宏 `add_compile_definitions`：**

| 宏 | 作用 |
|---|---|
| `_USE_MATH_DEFINES` | 让 `<cmath>` 暴露 `M_PI` 等常量（MSVC 默认不给） |
| `NOMINMAX` | 禁掉 `windows.h` 里的 `min`/`max` 宏，避免顶掉 `std::min/std::max` |
| `_CRT_SECURE_NO_WARNINGS` | 关掉"请改用 `xxx_s` 安全函数"的警告 |
| `WIN32_LEAN_AND_MEAN` | 精简 `windows.h`，少拖一堆没用的头 |

> 这些就是 README"常见问题"表里 `C4819` / `M_PI 未定义` / `min/max 报错` 全都**不会发生**的原因 —— 已经在 CMakeLists 里提前堵上了。

```cmake
set_target_properties(${target} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target}>"  # VS 调试工作目录 = exe 所在
    FOLDER "Tests")
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT test_bezier)   # VS 里 F5 默认跑哪个
```

> 这几行 `VS_*` 属性**只对 VS 生成器生效**：把调试工作目录设到 exe 旁边（生成的 SVG 好找），
> 把三个 target 收进 "Tests" 文件夹，并让 F5 默认跑 `test_bezier`。Mac 下这些属性被忽略。

---

## 5. 配置完，build/ 里生成了什么

**方式 B（VS 生成器）** —— 一套 VS 解决方案：

```
build/
├── CMakeCache.txt          ← 配置缓存（编译器路径、所有选项）
├── week01_A.sln            ← VS 解决方案（双击用 VS 打开）
├── test_bezier.vcxproj     ← 每个 target 一个工程文件（+ .filters）
├── ALL_BUILD.vcxproj       ← CMake 加的伪 target：一键构建全部
├── ZERO_CHECK.vcxproj      ← CMake 加的伪 target：检测 CMakeLists 改没改、要不要重新生成
└── bin/
    ├── Debug/              ← --config Debug 的产物：*.exe + *.pdb + 运行时产的 *.svg
    └── Release/            ← --config Release 的产物
```

> - `geo_common` 是 **INTERFACE 库**（纯头、无 `.cpp`），所以**不会生成 `.vcxproj`**，只把头文件路径传给链接它的 target。
> - `ALL_BUILD` / `ZERO_CHECK` 是 CMake 在 VS 工程里**自动加的**，第一次看到别奇怪 —— Mac 的 Makefile 里对应的是 `all` 和依赖检查逻辑。
> - **没有 `compile_commands.json`**（VS 生成器不产）。要给 clangd 用就改 `-G "Ninja"`。

**方式 A 的退回默认**（仅当项目**没有** `CMakePresets.json` 时，VS 才退回 Ninja）—— 产物在另一处：
本工程已提供预设，方式 A 实际落在 `build/`（见上方第 1.5 节）；下面这套 `out/build/` 仅供对照：

```
out/build/<preset>/
├── CMakeCache.txt
├── build.ninja             ← Ninja 构建脚本
├── compile_commands.json   ← ✅ Ninja 会产（clangd 直接能用）
└── bin/                    ← *.exe + *.pdb + *.svg（单配置，无 Debug/ 子目录）
```

---

## 6. 一次编译到底发生了什么

`test_bezier.cpp` → `bin\Debug\test_bezier.exe`，MSVC 工具链走这几步：

```
test_bezier.cpp
   │  ① 预处理          展开 #include "geo.h"、<cmath>；处理 #ifdef _WIN32
   ▼                    （Windows 下 enable_utf8_console() 走真实现，调 SetConsoleOutputCP）
（展开后的纯 C++ 源）
   │  ② 编译 cl /c      cl.exe 直接产出 test_bezier.obj（机器码）
   ▼                    /Zi 把"源码行号↔机器码"调试信息写进独立的 .pdb 文件
test_bezier.obj
   │  ③ 链接 link.exe   把 .obj + C++ 运行时库(MSVCRT) 拼成 .exe；解析 std::cout 等外部符号
   ▼
test_bezier.exe  +  test_bezier.pdb   ✅ 可执行 + 调试数据库
```

对本工程的具体说明：

- 第①步：`geo.h` 的 `#pragma once` 保证只展开一次；`enable_utf8_console()` 在 Windows 下因 `#ifdef _WIN32` 编成**调用 `SetConsoleOutputCP(CP_UTF8)` 的真实现**（Mac 下是空函数）—— 这就是中文 `std::cout` 不乱码的原因。
- 第②步：和 clang 不同，`cl.exe` 内部一步出 `.obj`（不单独暴露汇编阶段）；`/Zi` 把调试符号写进**独立的 `.pdb`**（program database），而不是像 DWARF 那样塞进 `.obj` 里。
- 第③步：`geo_common` 是纯头库，不贡献 `.obj`；真正链进来的是 C++ 标准库 / CRT。三个 target 各自独立链接，各产一个 `.exe` + `.pdb`。

> **为什么 `/O2` 会毁掉调试体验**（和 Mac 的 `-O2` 同理）：优化会内联、重排、删指令、把变量塞进寄存器。
> 于是断点无处可落、单步乱跳、变量显示 `<optimized out>`。
> 所以**调试用 Debug（`/Od /Zi`），测性能才用 Release（`/O2`）**。

---

## 7. Debug vs Release 速查（MSVC）

CMake 给 MSVC 的默认标志：

| | Debug | Release |
|---|---|---|
| 优化 | `/Od`（不优化） | `/O2`（最大化速度） |
| 调试符号 | `/Zi`（产 `.pdb`） | 默认无 |
| 运行时库 | `/MDd`（调试版 CRT） | `/MD`（发布版 CRT） |
| 额外检查 | `/RTC1`（运行时错误检查） | — |
| 宏 | — | `/DNDEBUG`（关掉 `assert`） |
| 断点 / 单步 | ✅ 准确 | ❌ 乱跳、变量看不全 |
| 用途 | 写代码、查 bug、跟断点 | 跑性能、发布 |

切换（多配置生成器，**构建时**选）：

```cmd
cmake --build build --config Release
```

> 想要"优化版 + 还能看调试信息"，用 **RelWithDebInfo**（`/O2 /Zi /DNDEBUG`）：
> `cmake --build build --config RelWithDebInfo`。
> 注意产物在 `bin\<Config>\` 下，每个配置一个子目录 —— 这是多配置生成器和 Mac 单配置最直观的差别。

---

## 8. 常用命令速查（在 x64 Native Tools 命令行里）

```cmd
:: 用预设（CMakePresets.json）—— 命令行和 VS 按钮跑的是同一套
cmake --preset windows-vs2026            :: 配置（= 下面那句显式生成器命令）
cmake --build --preset vs2026-debug      :: 构建 Debug
cmake --build --preset vs2026-release    :: 构建 Release
cmake --list-presets                     :: 看有哪些预设
```

```cmd
:: 不用预设、手写完整命令（与上面等价）
:: 配置：生成 VS 解决方案
cmake -S . -B build -G "Visual Studio 18 2026" -A x64

:: 构建 Debug / Release（多配置，构建时选）
cmake --build build --config Debug
cmake --build build --config Release

:: 只构建一个 target
cmake --build build --target test_bezier --config Debug

:: 并行构建（用满 CPU；/MP 已开，再加 -j 更稳）
cmake --build build --config Release -j

:: 用 Ninja（产 compile_commands.json 给 clangd）
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build build

:: 在 VS 里打开生成的解决方案
start build\week01_A.sln

:: 彻底清理重来
rmdir /s /q build
```

---

## 9. 名词对照表（Mac/Clang ↔ Windows/MSVC）

| Mac / Clang | Windows / MSVC | 含义 |
|---|---|---|
| `clang++` | `cl.exe` | 编译器驱动 |
| `ld`（由 clang++ 调） | `link.exe` | 链接器 |
| `.o` | `.obj` | 目标文件（中间产物） |
| `-g` / DWARF（内嵌 `.o`） | `/Zi` / `.pdb`（独立文件） | 调试符号 |
| `-O0` / `-O2` | `/Od` / `/O2` | 优化级别 |
| `-std=c++17` | `/std:c++17` | 语言标准 |
| `-I 路径` | `/I 路径` | 头文件搜索路径 |
| `-D 宏` | `/D 宏` | 预处理宏定义 |
| `-Wall -Wextra` | `/W4` | 警告级别 |
| `Unix Makefiles`（单配置） | `Visual Studio`（多配置） | 生成器 |
| `make` | `MSBuild` / `ninja` | 底层构建工具 |
| `build/bin/` | `build\bin\<Config>\` | 产物目录 |

---

## 相关

- 同结构的 Mac 版见 [CMake构建与编译笔记（Mac·Clang篇）.md](CMake构建与编译笔记（Mac·Clang篇）.md)
- 工程使用说明（Windows 三种方法 A/B/C）见 [README.md](README.md) 的「一、Windows 环境」
- VSCode 调试技巧（变量增强 / watchpoint）见 [VSCode调试技巧笔记.md](VSCode调试技巧笔记.md)
