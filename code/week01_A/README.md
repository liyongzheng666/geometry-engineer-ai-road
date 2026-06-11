# week01_A — 几何代码验证闭环（CMake 工程）

第 1 周的"粘贴即验证"工程，配套 [docs/week01_实操_AI代码验证闭环.md](../../docs/week01_实操_AI代码验证闭环.md)。
把 AI 生成的几何函数粘到 `test_*.cpp` 的 **PASTE ZONE**，
重新编译运行，立刻得到 PASS/FAIL 数值判定 + SVG 可视化。

支持两套环境，按你的系统直接跳转：

- 👉 [一、Windows 环境](#一windows-环境)（Visual Studio 2022/2026 + MSVC）
- 👉 [二、macOS 环境](#二macos-环境)（AppleClang + VSCode + 断点调试）

> 想搞懂 CMake 到底在做什么（配置 / 编译 / 链接）？见用本工程逐项解读的两篇姊妹笔记：
> **[CMake构建与编译笔记（Mac·Clang篇）.md](CMake构建与编译笔记（Mac·Clang篇）.md)** 和 **[CMake构建与编译笔记（Win·VS2026篇）.md](CMake构建与编译笔记（Win·VS2026篇）.md)**。
> 调试器使用技巧（变量增强 / watchpoint）见 **[VSCode调试技巧笔记.md](VSCode调试技巧笔记.md)**。

---

## 目录结构

```
week01_A/
├── CMakeLists.txt              ← CMake 工程定义（两套环境共用）
├── CMakePresets.json           ← VS/IDE 打开即用的配置+构建预设（Windows）
├── geo.h                       ← 公共头：Point3D + SVG writer（不要改）
├── test_bezier.cpp             ← Bezier 求值测试 harness
├── test_bspline.cpp            ← B 样条基函数测试 harness
├── test_nurbs.cpp              ← NURBS 求值测试 harness
├── .vscode/                    ← VSCode 配置（两平台共用，开箱即用）
│   ├── tasks.json              ←   配置 + 构建 Debug 的预启动任务（Mac · Unix Makefiles）
│   ├── launch.json             ←   CodeLLDB 启动配置（含 Point3D 显示增强）
│   ├── settings.json           ←   调试内联值 / LLDB 控制台命令模式
│   └── extensions.json         ←   推荐扩展（CodeLLDB / Debug Visualizer / Error Lens / CMake Tools）
├── README.md                   ← 本文档
├── CMake构建与编译笔记（Mac·Clang篇）.md   ← CMake 构建/编译过程详解（Mac/Clang）
├── CMake构建与编译笔记（Win·VS2026篇）.md  ← 同上，Windows/VS2026/MSVC 对应版
├── VSCode调试技巧笔记.md        ← 变量显示增强 / Debug Visualizer / watchpoint
├── .markdownlint.jsonc         ← 笔记的 markdownlint 规则（中文排版放行说明在文件内）
└── .gitignore                  ← build/、*.svg、.cache/ 等产物不入库（见文末「仓库说明」）
```

---

## 这个工程在做什么（与平台无关）

`geo.h` 提供基础类型 `Point3D` 和一个轻量 `SVG` 写出器（不要修改）。
每个 `test_*.cpp` 里：

- **PASTE ZONE**：放被测函数（你从 AI 拿到的实现，整体替换占位实现）。
- **独立参考实现**：用另一种数学路径（如 Bezier 的闭式 Bernstein 公式）算同一个值，互相对照。
- **`main()`**：跑若干 Test，逐项打印 `[PASS]/[FAIL]` 和数值误差，并生成 SVG。

所以验证是"客观双算对照"——**只改 PASTE ZONE，不要改 `main()` 和参考实现**。

> 📌 仓库的初始状态下，PASTE ZONE 里是**直接返回 0 的占位桩**，所以三个测试一跑全是
> `[FAIL]`——这是预期行为，不是工程坏了。粘入正确实现后即转 `[PASS]`。

---

# 一、Windows 环境

## 环境要求

- **Visual Studio 2022 (17.x)** 或 **Visual Studio 2026 (18.x)**
- 安装时勾选 **"使用 C++ 的桌面开发"** 工作负载（自带 MSVC 编译器 + CMake + Ninja）
- 不需要任何第三方库（无需 Eigen / Boost / Qt）

## 方法 A：Visual Studio 直接打开（最省事）

VS 原生支持 CMake：

1. 启动 Visual Studio → **"打开本地文件夹"** → 选 `week01_A`
2. VS 读到本工程的 `CMakePresets.json`，顶部"配置"下拉出现 **`windows-vs2026`**，选它即自动配置（首次约 10 秒）
3. 顶部"启动项"下拉框选 **`test_bezier.exe`**
4. **F5**（调试运行，可断点）或 **Ctrl+F5**（直接运行）
5. 控制台显示 PASS/FAIL；SVG 生成在 `build\bin\<Config>\` 下（带了预设，VS 用 VS 生成器配置到 `build\`，不再是 `out/build/`）
6. 切换 `test_bspline` / `test_nurbs`：改顶部启动项下拉框即可

VS 的 F5 默认就是 Debug 配置，断点开箱即用，无需额外设置。

> 这一步"点配置/构建按钮"底层跑的，就是方法 B 那两句 `cmake` 命令——原理见
> [CMake构建与编译笔记（Win·VS2026篇）.md](CMake构建与编译笔记（Win·VS2026篇）.md) 第 1.5 节。

## 方法 B：命令行构建（适合脚本化）

打开 **"x64 Native Tools Command Prompt for VS 2022"**（开始菜单里搜），cd 到 `week01_A`：

```cmd
cmake -S . -B build
cmake --build build --config Release
```

产物：
```
build\bin\Release\test_bezier.exe
build\bin\Release\test_bspline.exe
build\bin\Release\test_nurbs.exe
```

运行：
```cmd
cd build\bin\Release
test_bezier.exe
```

SVG 输出在当前目录（即 `build\bin\Release\`）。

显式指定生成器：
```cmd
cmake -S . -B build -G "Visual Studio 17 2022" -A x64    :: VS2022
cmake -S . -B build -G "Visual Studio 18 2026" -A x64    :: VS2026
cmake -S . -B build -G "Ninja"                           :: Ninja（更快，且产 compile_commands.json）
```

## 方法 C：VSCode（日常写代码 + 调试）

### 装扩展

| 扩展 | id | 作用 |
|---|---|---|
| clangd | `llvm-vs-code-extensions.vscode-clangd` | C++ 智能提示（补全、跳转、查引用、refactor） |
| CodeLLDB | `vadimcn.vscode-lldb` | 断点调试（跨平台，比 cpptools 的 lldb-mi 稳） |
| CMake Tools（可选） | `ms-vscode.cmake-tools` | 图形化配置 / 构建 / 选 target |

装 clangd 时若提示"是否禁用 Microsoft C/C++ 的 IntelliSense"——**点禁用**（两者会打架）。
已装过 cpptools 的，把设置里 `C_Cpp.intelliSenseEngine` 设为 `disabled`。

### 让 clangd 拿到 compile_commands.json（Windows 关键）

`compile_commands.json` 只在 **Ninja / Makefile** 生成器下产出，**Visual Studio 生成器不产出**。
本工程带了 `CMakePresets.json`，CMake Tools 会直接读它——**在状态栏选配置预设
`windows-ninja`** 即可（Ninja 在装 VS"使用 C++ 的桌面开发"工作负载时已经一起装好）。

> ⚠️ 如果 `build/` 之前是用 VS 生成器配置的，切换预设前先删掉 `build/`，
> 否则 CMake 报"生成器与已有缓存不一致"——同 Win 篇笔记 1.5 节末尾那条提醒。

### 流程

1. `File → Open Folder` 选 `week01_A`（VSCode 会按 `.vscode/extensions.json` 提示装推荐扩展；clangd 需手动装）
2. 状态栏选配置预设 **`windows-ninja`**（检测到 `CMakePresets.json` 后，CMake Tools 用预设代替 Kit 选择）
3. **F7** 构建 → 生成 `build/compile_commands.json`，clangd 自动加载
4. 在 `test_*.cpp` 打断点 → **F5** 调试

---

# 二、macOS 环境

## 环境要求

- **Xcode Command Line Tools**（提供 `clang++`）：终端跑 `xcode-select --install`
- **CMake**：`brew install cmake`
- 验证：`clang++ --version` 和 `cmake --version` 都能输出版本号即可
- 同样不需要任何第三方库

## 方法 A：命令行构建运行

cd 到 `week01_A`：

```bash
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

> 本工程在非 MSVC 环境下**默认就是 Debug**（带 `-g -O0`），所以 `-DCMAKE_BUILD_TYPE=Debug` 可省略；
> 想要优化版加 `-DCMAKE_BUILD_TYPE=Release`（`-O3 -DNDEBUG`）。

产物（Makefile 是单配置生成器，没有 `Release/` 子目录）：
```
build/bin/test_bezier
build/bin/test_bspline
build/bin/test_nurbs
```

运行：
```bash
cd build/bin
./test_bezier
```

SVG 输出在当前目录（即 `build/bin/`）。

## 方法 B：VSCode + 断点调试（推荐）

本工程已附带完整 `.vscode/` 配置，开箱即用：

- `tasks.json` / `launch.json` —— F5 自动"配置 + 构建 Debug"再启动 CodeLLDB
- `settings.json` / `launch.json` 里的调试显示增强（Point3D 一行显示、行尾内联值、
  LLDB 控制台命令模式）—— 逐项解释见 [VSCode调试技巧笔记.md](VSCode调试技巧笔记.md)
- `extensions.json` —— 打开工作区时自动提示装推荐扩展（CodeLLDB、Debug Visualizer、
  Error Lens、CMake Tools）

### 装扩展

| 扩展 | id | 作用 |
|---|---|---|
| clangd | `llvm-vs-code-extensions.vscode-clangd` | C++ 智能提示（需手动装，不在推荐列表里） |
| **CodeLLDB** | `vadimcn.vscode-lldb` | **断点调试（Mac 必装）** |

同样：装 clangd 时让它禁用 Microsoft C/C++ 的 IntelliSense。

### 一次性确认 clangd 工作

1. `File → Open Folder` **选 `week01_A` 根目录**（必须是根目录，`${workspaceFolder}` 才对得上配置）
2. 终端跑一次 `cmake -S . -B build -G "Unix Makefiles"` 生成 `build/compile_commands.json`
3. 打开任一 `.cpp`，几秒后状态栏 clangd 显示 "indexed"
4. 鼠标悬停 `Point3D` 能弹出定义；`p.w = 1.0;` 会报红波浪线（`Point3D` 没有 `w` 字段）= clangd 正常

### 断点调试流程 🔴

1. 打开 `test_bezier.cpp`，在 PASTE ZONE 的函数体里某行**点左侧设断点** → 应是**实心红点**（不是空心灰圈）
2. 按 **F5** → 顶部弹窗选 `test_bezier` / `test_bspline` / `test_nurbs`
3. F5 会自动先跑 `build-debug` 任务重新编译，再启动 CodeLLDB
4. 断点命中后：左侧看局部变量、**F10** 单步跳过、**F11** 步入、调用栈一目了然

> **断点跟不进（空心灰圈）？** 99% 是产物没带调试符号。确认：
> - 用的是 Debug 构建（本工程默认即是），编译命令里有 `-g`、没有 `-O2/-O3`
> - 检查 `build/compile_commands.json` 里出现 `-g`
> 这与"配置隔离"无关——C++ 断点的唯一前提就是 `-g` 调试符号 + 一个调试器（CodeLLDB）。

---

## 把 AI 代码接入（工作流 · 通用）

以 Bezier 为例：

1. 在 AI 上跑你的 prompt，拿到 `bezier_cubic` 的 C++ 实现
2. 打开 `test_bezier.cpp`，找到这两行之间：
   ```cpp
   // >>>>>>>>>>  PASTE AI-GENERATED bezier_cubic BELOW  <<<<<<<<<<
   ...占位实现...
   // >>>>>>>>>>             END OF PASTE ZONE              <<<<<<<<<<
   ```
3. 用 AI 实现**整体替换**占位实现（保持函数签名一致）
4. 保存 → **Ctrl+F5 运行**（或 F5 调试）
5. 看控制台 PASS/FAIL；浏览器拖入 SVG 看曲线形态

### 三段 prompt 对照实验

周一-周二建议建个子目录存各版本输出：
```
week01_A/ai_versions/
├── bezier_A.cpp.txt     ← prompt A 的 AI 输出（仅函数体）
├── bezier_B.cpp.txt
├── bezier_C.cpp.txt
└── notes.md             ← 三版数值误差 + SVG 截图对比
```
逐版本粘进 `test_bezier.cpp` 跑一遍记录结果。

---

## SVG 查看

生成的 `.svg` 直接拖进浏览器（Chrome / Edge）。

- **Mac**：`open build/bin/bezier_output.svg`
- **Windows**：双击或拖进浏览器
- 也可在 VSCode 装 **SVG Preview** 插件，编辑器内直接预览

---

## 常见问题

| 现象 | 平台 | 原因 / 解决 |
|---|---|---|
| 断点是空心灰圈，跟不进 | Mac/VSCode | 缺调试符号。用 Debug 构建（默认即是），确认编译命令有 `-g`、无 `-O2` |
| F5 没反应 | Mac/VSCode | 没装 CodeLLDB，或没用根目录打开文件夹 |
| 控制台中文乱码 | Windows | 控制台默认 GBK；`enable_utf8_console()` 已自动调用，cmd 仍乱码先 `chcp 65001` |
| `clangd 找不到 compile_commands.json` | 两者 | 用了 VS 生成器（不产此文件）。改 Ninja/Makefile 生成器后重新构建 |
| 编译报 `C4819` / `M_PI` 未定义 | Windows | CMakeLists 已设 `/utf-8`、`_USE_MATH_DEFINES`，正常构建不会出现 |
| `min/max` 报错 | Windows | CMakeLists 已设 `NOMINMAX` |
| SVG 不知去哪了 | 两者 | 工作目录 = 可执行文件所在目录（`build/bin/` 或 `build\bin\<Config>\`） |
| 想只编一个 target | 两者 | `cmake --build build --target test_bezier`（Win 加 `--config Release`） |
| 想清理重来 | 两者 | 删掉 `build/` 文件夹即可 |

---

## 仓库说明（哪些文件入库）

本目录是学习仓库 `code/` 下的第 1 周工程（git 根在仓库顶层）。
`.gitignore` 的取舍原则：**笔记里说"本工程已附带"的必须入库，
构建时能重新生成的一律不入库**。

| 入库 ✅ | 不入库 ❌（.gitignore） |
|---|---|
| 源码（`geo.h`、三个 `test_*.cpp`） | `build/`、`out/`（构建产物） |
| `CMakeLists.txt`、`CMakePresets.json` | `*.svg`（每次运行重新生成） |
| `.vscode/` 四件套 | `.cache/`（clangd 索引）、`.DS_Store` |
| README + 三篇笔记、`.markdownlint.jsonc` | `CMakeUserPresets.json`（用户本地覆盖） |

> 其中 `CMakePresets.json` 和 `.vscode/` **必须随仓库分发**：
> Win 篇笔记 1.5/1.6 节的"VS 打开即用"靠前者，VSCode 调试笔记的"已配"项靠后者。
> 把它们加进 ignore 会让两篇笔记失真。

---

## 编辑约定

- **不要修改 `geo.h`**：整个第 1 周保持不变
- **不要修改 `int main()` 和参考实现**：它们是客观判定的金标准
- **只在 PASTE ZONE 内替换函数实现**
- AI 输出里若重复定义了 `Point3D`，**删掉它的**，统一用 `geo.h` 的

---

## 验证标准速查

| 测试 | 通过线 | 失败常见原因 |
|---|---|---|
| Bezier Test 1 | 误差 < 1e-9 | 用 Bernstein 展开且 t 接近 0/1 时精度差 |
| Bezier Test 2 | 严格相等 | 端点处理错（应直接返回控制点，不该插值） |
| Bezier Test 3 | 严格相等 | 退化（重合控制点）处理错 |
| B-spline Test 1 | 误差 < 1e-9 | 递推公式实现错 |
| B-spline Test 2 | 0 violation | 基函数出现负值 |
| NURBS Test 1 | 误差 < 1e-6 | 权重或归一化错（点不在单位圆上） |
| NURBS Test 2 | 误差 < 1e-6 | 端点插值错 |
