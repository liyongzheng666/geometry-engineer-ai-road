# 几何工程师的 AI 提效实验室

> 📖 **在线阅读（课程主页）**：<https://liyongzheng666.github.io/geometry-engineer-ai-road/>
> 展望 · 12 周路线 · 每周计划拆解 / 代码解读 / 补充知识点

面向**几何造型算法工程师**（B 样条 / NURBS / 网格 / OCCT 不需要从头讲、C++ 老兵、
AI 几乎零基础）的 12 周提效课程：把 AI 当生产力工具用，每天少花 2-3 小时在重复劳动上。

核心方法论一句话：

> **先有验证闭环，再让 AI 写代码。**
> 没有验证闭环的 AI 输出只是文本；有了验证闭环的 AI 输出才是可信代码。

---

## 12 周总览

| 阶段 | 周 | 主题 | 实操 | 代码 |
|---|---|---|---|---|
| **一·表达** | 1 | 公式化 Prompt vs 大白话 | [week01 实操](docs/week01_实操_AI代码验证闭环.md) | [`code/week01_A/`](code/week01_A/) |
| | 2 | 退化输入的防御性提示词 | [week02 实操](docs/week02_实操_退化输入防御.md) | [`code/week02_A/`](code/week02_A/) |
| | 3 | 上下文注入：让 AI 用你的代码风格 | [week03 实操](docs/week03_实操_上下文注入.md) | [`code/week03_A/`](code/week03_A/) |
| | 4 | 综合实战：保特征网格平滑 | 规划中 | 规划中 |
| **二·数学** | 5-8 | SymPy 推导 / 符号→C++ / OCCT·CGAL / ARAP | 规划中 | 规划中 |
| **三·调优** | 9-12 | 浮点 Review / 测试自动化 / 工作流 / SOP | 规划中 | 规划中 |

完整大纲（每周逐日任务 + prompt 模板原文）见 **[docs/A轨_12周总纲.md](docs/A轨_12周总纲.md)**。
完成 A 轨后想造 AI 工具（SDK / MCP / RAG / 几何深度学习），见进阶课程
**[docs/进阶_双轨AB版.md](docs/进阶_双轨AB版.md)**。

---

## 仓库结构

```text
├── README.md                ← 本文档
├── docs/                    ← 课程文档：总纲 + 各周实操 + 进阶
├── code/
│   ├── week01_A/            ← 第 1 周：Bezier / B样条 / NURBS 验证闭环
│   ├── week02_A/            ← 第 2 周：点投影 / 点在多边形 / 射线-三角形（退化输入）
│   └── week03_A/            ← 第 3 周：三点定圆 / 平面拟合（风格注入，含 Eigen）
└── prompt_library/          ← 你的个人 prompt 模板库（逐周填充）
```

## 每周代码工程的共同约定

每个 `code/weekNN_A/` 都是独立的 CMake 工程，同一套用法：

1. **PASTE ZONE**：每个 `test_*.cpp` 里有一段标记区域，放被测函数。
   初始是占位桩，**测试全 FAIL 是预期状态**；把 AI 生成的实现整体替换进去。
2. **客观双算对照**：harness 用与被测实现**不同数学路径**的金标准验证
   （Bernstein 闭式、圆性测试、winding number、重构校验……），AI 自夸无效。
3. **观察题**：部分测试无标准答案（等距双解、watertight、边界约定），
   打印 AI 代码的实际行为，考察它**有没有声明自己的约定**。
4. **SVG 可视化**：跑完生成 `*_output.svg`，浏览器拖开即看。数值供 assertion，图形供直觉。
5. **只改 PASTE ZONE**，不要动 `main()`、参考实现和 `geo.h`。

## 快速开始

```bash
git clone <本仓库>
cd code/week01_A
cmake -S . -B build && cmake --build build
./build/bin/test_bezier        # 全 FAIL：正常，等你粘 AI 的实现
```

环境要求与三种用法（Visual Studio 直开 / 命令行 / VSCode+断点调试）见
[week01_A/README.md](code/week01_A/README.md)；想搞懂 CMake 在每一步做什么，
week01_A 里还有三篇逐行解读的笔记（Mac·Clang 篇 / Win·VS2026 篇 / VSCode 调试技巧）。

> 唯一的第三方依赖是第 3 周的 Eigen（本机没装则 CMake 自动联网拉取，仅首次）。

## 学习产出（12 周后你该有的东西）

- `prompt_library/`：15+ 个按场景分类的可重用模板
- `context_snippets/`：你项目的核心类型与约定片段库
- `failure_log.md`：AI 翻车样本集 → 提炼成"什么任务不该让 AI 碰"的决策树

---

> 这 12 周不是学 AI，**是用几何工程师的判断力去校准 AI**。
> AI 永远会写出"看起来对、跑起来 90% 对、工业场景下 10% 翻车"的代码——
> **那 10% 就是你的护城河**。
