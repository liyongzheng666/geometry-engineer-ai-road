# 几何 × AI 工程师 12 周双轨进阶大纲（进阶课程）

> 📌 **定位说明**：本文是**进阶 / 全景版**，在 A 轨（把 AI 当生产力工具）之外
> 增加 B 轨——SDK 编程、MCP、RAG、几何深度学习等 **AI 工程**内容。
> 主线课程请从 [A轨_12周总纲.md](A轨_12周总纲.md) 开始（含第 1-3 周配套实操与代码）；
> 完成 A 轨后想更进一步——**会造 AI 工具的几何工程师**——再回到本文。

---

## 设计思想：双轨并行

原大纲的根本问题是**只有一条腿**——所有任务都在聊天框里完成 prompt 工程。本版引入**第二条腿**：让你同时把 AI 当作可编程的基础设施去构建系统。

| 轨道 | 关注点 | 周时间投入 | 产物形态 |
|---|---|---|---|
| **A 轨：AI as Tool** | 让 AI 帮你写几何代码 | ~60% | 高质量 C++/Python 代码、Prompt 模板 |
| **B 轨：AI as Craft** | 让你能用 AI 造系统 | ~40% | SDK 调用、RAG、MCP server、ML 模型 |

每周都同时推进两条轨道。这是这份大纲与原版最大的差异。

---

## 阶段一：地基（第 1-4 周）

**目标**：A 轨打牢公式化 Prompt 与防御性代码生成；B 轨完成 SDK 入门 + 第一个可调用的 AI 程序。

### 第 1 周：公式化 Prompt + Anthropic SDK 入门

**A 轨（保留原版思路，强化）**
- 任务：Bezier / B 样条 / NURBS 求值器
- 强化点：在生成前要求 AI 列出**所有可能的退化分支**作为 Checklist，再生成代码。Prompt 收尾固定带：「列出此函数在哪些输入下会进入未定义行为，并给出对应的单元测试」

**B 轨（新增）**
- 任务：装好 `anthropic` Python SDK，跑通第一个调用
- 学习点：
  - Message API 的 `system` / `user` / `assistant` 三种 role
  - `max_tokens`、`temperature`、`stop_sequences` 的实际含义
  - Token 计费：用 `client.messages.count_tokens()` 估算你贴 OCCT 头文件的成本
- **本周产出**：一个 `geo_assistant.py`，命令行接收数学公式，调用 Claude Opus 4.7 返回带退化处理的 C++ 代码。

### 第 2 周：边界鲁棒性 + 结构化输出

**A 轨**
- 任务：Newton-Raphson 点投影、射线法点在多边形内、Moeller-Trumbore 三角形相交
- 强化点：每个函数交付时必须配一份 `corner_cases.md`，由 AI 帮你穷举

**B 轨**
- 学习点：
  - **结构化输出**（Anthropic 的 tool use 强制 JSON / OpenAI 的 `response_format`）
  - 让 AI 输出**结构化的退化用例表**（JSON），由 Python 自动生成 Google Test 桩代码
- **本周产出**：`degeneracy_finder.py`——输入函数签名+数学描述，输出 JSON 形式的退化用例集 + 自动生成 GTest 代码。

### 第 3 周：Prompt 上下文管理 + 第一个 RAG

**A 轨**
- 任务：把 `Point3D.h` / `Vector3D.h` / 半边数据结构注入 Prompt，让 AI 用你的类型而不是自造

**B 轨（重要新增）**
- 学习点：
  - Embedding 模型选择（OpenAI `text-embedding-3-small` / BGE-M3 中文优先）
  - 向量库：用 **ChromaDB** 本地起步（零运维）
  - 切块策略：对头文件**按类/函数切**而非按行切
- **本周产出**：`geo_rag.py`——把你项目所有 `.h` 头文件 + 注释灌进 ChromaDB，提供一个 `retrieve_context(query)` 接口。下周开始所有 A 轨任务都用它替代手贴头文件。

### 第 4 周：阶段一综合实战

**考核项目**：**Feature-preserving Mesh Smoothing**（保留原版）

**双轨整合**：
1. 用 RAG（B 轨产物）自动检索半边结构相关头文件并注入上下文
2. 用结构化输出（B 轨产物）生成退化用例
3. A 轨完成双边滤波的数学推导和 C++ 实现
4. **新交付物**：把整个流水线封装成一个 CLI：`./geo-ai smooth --mesh input.obj --feature-preserve`，内部全程调用 Claude

---

## 阶段二：数学引擎 + Agent 化（第 5-8 周）

**目标**：A 轨用 SymPy 解决符号推导；B 轨学会把几何库暴露给 AI 作为 tool。

### 第 5 周：SymPy + JAX 自动微分对照

**A 轨（保留原版）**
- 任务：B 样条曲面能量的梯度推导

**B 轨（关键修正——原版漏了 autodiff）**
- 学习点：
  - **SymPy 不是唯一答案**。符号微分对复杂能量泛函会爆炸式膨胀
  - **JAX 的 `jax.grad` / `jax.jacobian`**——数值精确、不会膨胀、可 JIT
  - 何时用哪个：解析式短且需要导出 C++ → SymPy；优化迭代中实时求导 → JAX
- **本周产出**：同一个能量函数用两种方式实现，对比代码量、计算时间、数值误差

### 第 6 周：自动代码生成（修正原版的"手动 CSE"错误）

**A 轨修正**
- 原版让你手动做 CSE，但 **SymPy 自带 `sympy.cse()`**，几行搞定
- 任务：用 `sympy.cse()` + `sympy.codegen.ast.CCodePrinter` 自动生成带公共子表达式提取的 C++ 函数
- 进阶：用 `sympy.utilities.codegen.codegen()` 直接产出可编译的 `.cpp` + `.h`

**B 轨**
- 学习点：用 `pybind11` 把生成的 C++ 自动编译并暴露给 Python，形成「SymPy 推导 → C++ 编译 → Python 验证」全自动闭环
- **本周产出**：`symbolic_to_cpp.py`——输入 SymPy 表达式，输出已编译可调用的 Python 模块

### 第 7 周：MCP Server——把 OCCT 暴露给 Claude

**这是与原版最关键的分歧点**。原版让你"问 Claude 怎么用 OCCT"；本版让你**让 Claude 直接操作 OCCT**。

**A 轨**
- 保留：OCCT 容差/Handle/布尔运算的 Prompt 实践

**B 轨（核心新增）**
- 学习点：
  - **Model Context Protocol (MCP)** 协议规范
  - 用 Python `mcp` SDK 写一个 server
  - 把这些 OCCT 操作暴露为 tool：
    - `load_step(path)` / `save_step(shape, path)`
    - `boolean_cut(a, b, tolerance)`
    - `extract_faces(shape)` / `extract_edges(shape)`
    - `measure_distance(s1, s2)`
- **本周产出**：`occt-mcp-server` 可以被 Claude Code 调用，你可以在聊天里说"打开这个 STEP 文件，把所有圆柱面找出来"，Claude 真的去执行而不是只生成代码

### 第 8 周：阶段二综合实战

**考核项目**：**ARAP 曲面变形**（保留原版）

**双轨整合**：
1. SymPy 推导局部能量 + `cse()` 自动生成 C++（B 轨第 6 周产物）
2. JAX 实现一个对照版本验证数值正确性（B 轨第 5 周产物）
3. 通过 MCP server 让 Claude 加载测试模型并执行 ARAP（B 轨第 7 周产物）
4. **新交付物**：一个 demo 视频，自然语言指挥 Claude 完成「加载-变形-导出」全流程

---

## 阶段三：几何深度学习 + 终极项目（第 9-12 周）

**目标**：补齐原版完全缺失的几何 ML 内容，这部分是「AI × 几何工程师」真正的护城河。

### 第 9 周：点云与 PointNet

**为什么先学 PointNet**：架构简单，但概念完整，能让你建立"几何数据如何进神经网络"的直觉。

- 学习内容：
  - PyTorch 基础（Tensor、autograd、Module、DataLoader）—— 假设你完全没碰过，4 天速成
  - PointNet 论文精读 + PyTorch 复现（**用 Claude 辅助理解每一行**，这就是 A 轨自然延续）
  - 数据集：ModelNet40 / ShapeNet
- 工具：`PyTorch3D` 处理点云 IO 与变换
- **本周产出**：训出一个能区分 10 类零件的 PointNet 分类器

### 第 10 周：神经隐式表示（SDF / NeRF / 3DGS）

**为什么对你重要**：这是几何表示的范式革命，CAD 工业界已经在用 SDF 做 retopology 和 mesh repair。

- 学习内容：
  - **Signed Distance Function** 神经化（DeepSDF 思路）
  - 训一个小 MLP 拟合单个零件的 SDF
  - 用 Marching Cubes 把 SDF 还原为 mesh，对比原 mesh 的几何误差
  - **延伸**：3D Gaussian Splatting 的工程意义（仅阅读，不实现）
- **本周产出**：输入一个 STEP 零件 → 训出 SDF → 输出重建的 mesh，全流程脚本

### 第 11 周：可微分几何与参数反演

**为什么是杀手锏**：把你阶段二学的优化能力和阶段三的 ML 工具结合。

- 学习内容：
  - **可微分渲染**：`PyTorch3D` 或 `nvdiffrast`
  - **参数反演问题**：给定目标 mesh，反推参数化建模的参数（比如螺栓孔位置、圆角半径）
  - 这是 CAD 自动化的圣杯之一
- **本周产出**：给定一个法兰盘的图片/点云，反演出 OCCT 建模脚本的参数

### 第 12 周：终极项目——自然语言驱动的参数化 CAD Agent

**这一周把前 11 周全部串起来**，产出一个真正可用的 AI × 几何系统。

**项目**：`geo-agent`——自然语言到 STEP 文件的端到端 Agent

**架构**：
```
用户自然语言
    ↓
Claude Opus 4.7（主控）
    ↓ 调用 MCP tools（第 7 周产物）
    ├─→ OCCT 建模操作
    ├─→ RAG 检索（第 3 周产物）
    ├─→ SymPy 公式推导（第 6 周产物）
    ├─→ PointNet 分类辨别零件类型（第 9 周产物）
    └─→ SDF/可微分渲染做参数反演（第 10-11 周产物）
    ↓
.step 文件 + 验证报告
```

**测试用例**：
1. "做一个 M8 螺栓的 3D 模型" → 输出标准件 STEP
2. "看一下这个点云，反推出最接近的法兰盘参数" → 调用第 11 周的可微分渲染
3. "把这两个零件做布尔差，保留容差信息" → 调用 OCCT MCP

**交付物**：
- 可运行的代码仓库
- 一份**真正的复盘文档**（替代原版"Prompt 词典"）：哪些 AI 能搞定、哪些必须人来、成本/延迟/失败模式

---

## 与原版的关键差异速查表

| 维度 | 原版 | 本版 |
|---|---|---|
| AI 形态 | 仅聊天框 | 聊天 + SDK + Agent + ML 模型 |
| 上下文管理 | 手工贴头文件 | RAG 系统化（第 3 周建好用 9 周） |
| 数学推导 | 仅 SymPy | SymPy + JAX autodiff 对照 |
| CSE | 手动让 AI 提取 | `sympy.cse()` 一行 |
| OCCT 集成 | 问 Claude 怎么写 | 让 Claude 直接调用（MCP） |
| 几何深度学习 | 完全缺失 | 3 周专注（PointNet / SDF / DiffRender） |
| 第 12 周产出 | Prompt 词典 | 端到端可运行的 Agent |
| 工程产物 | 代码片段 | CLI + Server + Model + Agent |

---

## 学习节奏建议

- **每周 15-20 小时**：A 轨 ~10 小时，B 轨 ~6 小时，复盘 ~2 小时
- **B 轨遇到瓶颈优先**：B 轨是你完全陌生的领域，A 轨可以拖到周末补
- **每周五写一份「失败日志」**：记录这周 AI 在哪些任务上失败、什么 Prompt 不灵、什么模型表现差——这比成功案例值钱
- **不要追新论文**：第 11 周的 3DGS 只读不实现，把时间留给基本功

---

## 推荐资源（最小化清单）

- **必读**：Anthropic 官方文档的「Tool Use」和「Prompt Caching」章节
- **必跑**：`anthropic-cookbook` 仓库的 RAG 示例
- **必看**：`PyTorch3D` 官方 tutorial 的前 5 个
- **MCP**：`modelcontextprotocol/python-sdk` README + 一个官方示例 server
- **几何 ML 入门**：Stanford CS468「Machine Learning for 3D Data」的课件（不用看视频，PDF 够了）

---

## 最后的提醒

原版作者写得很认真，但他写这份大纲时心目中的"AI"是 2023 年的 ChatGPT 形态。**2026 年的 AI 工程是「Agent + Tool + Context」三件套**，不是聊天框里的复制粘贴。

如果你只有 12 周，**B 轨比 A 轨更值得投入**。A 轨的技能未来 2 年会逐渐被通用模型自动消化（更强的模型 + 更好的默认 prompt），但 B 轨的技能——**把你的领域知识封装成 AI 可调用的基础设施**——是会持续增值的。

> "几何算法工程师 + AI 使用者" 在 2027 年遍地都是；
> "几何算法工程师 + AI 工具构建者" 永远稀缺。
