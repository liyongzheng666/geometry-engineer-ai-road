# week02_A — 退化输入防御（CMake 工程）

第 2 周的"粘贴即验证"工程，配套 [docs/week02_实操_退化输入防御.md](../../docs/week02_实操_退化输入防御.md)。
把 AI 生成的函数粘到各 `test_*.cpp` 的 **PASTE ZONE**，重新编译运行，
立刻得到 PASS/FAIL 数值判定 + SVG 可视化。

> 🔧 **构建方法与 week01_A 完全相同**（同一份 CMakePresets、同一套 .vscode 配置）：
> Windows / macOS 的三种用法见 [week01_A/README.md](../week01_A/README.md)，
> 原理详解见其中的两篇 CMake 笔记。

## 本周三个 harness

| 文件 | 被测接口（你只动这个） | 验证金标准 |
|---|---|---|
| `test_point_inversion.cpp` | `point_inversion` — Newton-Raphson 点到 B 样条曲线投影 | 法向偏移构造已知解 + 密集采样/黄金分割独立对照 |
| `test_point_in_polygon.cpp` | `point_in_polygon` — 射线法点在多边形内 | winding number（atan2 角度和）双算对照 |
| `test_ray_triangle.cpp` | `ray_triangle` — Möller-Trumbore 射线-三角形相交 | 平面求交+重心坐标两段法对照 + 重构校验 |

## 接口聚焦说明

每个文件里，**PASTE ZONE 之前的代码都是给定基础设施**（曲线求值/导数、向量运算、
参考实现），使用者不要动；其中标注"PASTE ZONE 内允许调用"的函数（如
`curve_eval` / `dot3`），建议把声明原样贴进你的 prompt，作为 AI 可用的接口。

> 📌 初始状态下 PASTE ZONE 是占位桩，三个测试一跑全是 `[FAIL]`——预期行为。
> 每个 harness 除数值判定外还有若干**观察题**（如等距双解、watertight、
> backface culling）：没有标准答案，考察 AI 有没有声明它的行为约定。

## 快速开始（命令行）

```bash
cmake -S . -B build
cmake --build build
./build/bin/test_point_inversion     # Windows: build\bin\<Config>\test_point_inversion.exe
```
