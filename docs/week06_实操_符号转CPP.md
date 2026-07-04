# 第 6 周实操：符号 → 工业 C++（自动 CSE + Eigen 稀疏组装）

> 配套大纲：[A轨_12周总纲.md](A轨_12周总纲.md) 第 6 周
> 配套代码：[`code/week06_A/`](../code/week06_A/)（clone 即用；`test_sparse_assembly` 需 Eigen，同 [week03_A](../code/week03_A/README.md)）
>
> 周目标：把第 5 周 SymPy 推出的表达式，落成**项目里能编译、又快又对**的 C++。
> 两条主线：`cse()` 自动公共子表达式（**结果不变、只减重复计算**）、Eigen 稀疏矩阵组装
> （**Triplet 一次 setFromTriplets，别用 insert**）。验证仍坚持 C++ 数值金标准。

---

## 设计哲学

第 5 周解决了"推得对"，这周解决"落地得又对又快"。两个常被 AI 做糙的地方：

| 决策 | 理由 |
|---|---|
| 用 SymPy 自带 `cse()`，不让 AI"手动提公共子表达式" | 手动提易错易漏；`cse()` 是经过验证的算法，一行搞定 |
| CSE 前后必须**逐点数值一致** | CSE 只是把重复子式提出来算一次，绝不能改变结果——这是可判定的硬约束 |
| 稀疏组装用 `Triplet + setFromTriplets` | 一次性构建，自动对共享 dof 求和；`insert()` 逐个塞会触发多次重排 |
| 验证走"稠密参考"独立路径 | 稠密直接按下标累加，与稀疏构建完全不同的代码路径，互为金标准 |

## 工作台里有什么

`code/week06_A/`：`codegen_ref.h` 是公共头（弹簧能量 Hessian 的干净参考 + 计时器 + 扫描 SVG，**不要改**）。
能量复用第 5 周（自由点连 5 个锚点），锚点更多是为了让 CSE 的收益更明显。

| 文件 | 被测接口（你只动 PASTE ZONE） | 验证金标准 | 依赖 |
|---|---|---|---|
| `test_cse_equiv.cpp` | `hessian_naive` / `hessian_cse`（ccode 两版） | 两版逐点一致 <1e-12 + 都对参考 <1e-9 + cse 更快 | 无 |
| `test_sparse_assembly.cpp` | `assemble_sparse`（Triplet+setFromTriplets） | 与稠密参考逐元一致 <1e-12 + 全局对称 | Eigen |

初始 PASTE ZONE 是占位桩，测试全 FAIL——预期。

---

## 完整工作流（融合进第 6 周日程）

### 周一-周三：自动 CSE 与代码组织

**总纲的重要修正**：原版让你"手动让 AI 提取公共子表达式"——这是错的。SymPy 自带 `cse()`：

```python
from sympy import cse, ccode
common, reduced = cse([H[0,0], H[0,1], H[1,1]])   # 只需上三角
print("void hessian_cse(const double* p, double* H) {")
print("    const double x = p[0], y = p[1];")
for name, expr in common:
    print(f"    const double {name} = {ccode(expr)};")   # 公共子表达式按依赖顺序声明
print(f"    H[0] = {ccode(reduced[0])};  // Hxx")
print(f"    H[1] = {ccode(reduced[1])};  // Hxy")
print(f"    H[3] = {ccode(reduced[2])};  // Hyy")
print("    H[2] = H[1];\\n}")
```

同时生成一版**不 cse**的 `ccode(H)`（每个输出项各自重算 sqrt/pow）作对照。两版都粘进
`test_cse_equiv.cpp`：

- **Test 2**：cse 版与 naive 版逐点一致到 1e-12——CSE 不改变数值的硬证明；
- **Test 3**：计时。naive 每个输出项重算 sqrt/pow，cse 每个 `d_i` 只算一次，
  实测 **cse 快约 3 倍**（加速主要来自省掉的超越函数）；
- **SVG**：沿 y=1 扫 Hxx(x)，蓝(naive)粗线 + 橙(cse)细线严丝合缝叠成一条。

这周的 prompt 不再是"让 AI 写 CSE"，而是让 AI 写**后处理脚本**：把 `cse()` 的临时变量
重命名成可读名、按依赖顺序排序声明、包成一个 C++ 函数。

### 周四-周日：Eigen 稀疏组装

任务：把每条边的局部 4×4 Hessian 组装成全局稀疏矩阵 H ∈ R^{2N×2N}。
`local_hessian`（弹簧边的二阶导块）是给定基础设施，把它签名贴进 prompt，让 AI 只写 scatter：

```text
用 Eigen 把逐边局部 4×4 Hessian 组装成全局稀疏矩阵，要求：
1. 用 Eigen::Triplet 收集所有非零项，最后一次 setFromTriplets（会自动对共享 dof 求和）
2. dof 映射：顶点 v 的两个自由度是全局下标 2v, 2v+1
3. 预先 reserve 三元组数量（按边数×16 估）
不要用 insert() —— 告诉我为什么不能用（多次 insert 触发多次列重排）。
```

跑 `test_sparse_assembly`：**Test 1** 与稠密参考逐元对照（内部顶点被两边共享，
setFromTriplets 必须把两份对角块求和，错则失败）；**Test 2** 全局对称；
**Test 3** 两万顶点计时，并点破"若用 insert() 会慢一到两个量级"。
`sparse_pattern_output.svg` 画出块三对角稀疏图。

### 周末：沉淀模板 + 失败日志

抽进 [`prompt_library/`](../prompt_library/)（建议 `06_math/sympy_to_cpp.md`）：
"cse 后处理脚本 + Eigen Triplet 组装（主动否决 insert）"。更新 `failure_log.md`：
记 AI 在"能跑但低效"上翻车的样本（多半是显式临时矩阵、或误用 insert）。

---

## 这套流程的"杠杆"在哪里

1. **CSE 白捡的加速**：一行 `cse()`，Hessian 求值快 3 倍，且有 Test 2 保证结果没变——零风险优化。
2. **"能跑但低效"当场现形**：稀疏组装用 insert 还是 Triplet，肉眼看不出，计时（Test 3）看得出。你的价值就是认得出这种低效。
3. **主动否决错误方案**：和 Eigen 配合时 AI 默认给的不一定最优，prompt 里"不要用 insert / 不要显式临时矩阵"这类否决句，是你把工业经验注入的关键。

---

## 常见踩坑

| 现象 | 原因 / 应对 |
|---|---|
| cse 版 Test 2 不过（与 naive 有差） | CSE 临时变量声明顺序错（用了还没赋值的），或漏了某个输出项。让 AI 重跑 `cse()` |
| cse 版没比 naive 快 | 生成的 cse 版其实没提出子式，或 naive 版本身已很省。检查 cse 输出有没有 `const double` 临时量 |
| 稀疏 Test 1 不过 | 共享顶点的对角块没求和（用了覆盖而非累加），或 dof 下标映射错（2v/2v+1） |
| 稀疏 Test 2 不对称 | 局部块 scatter 时行列下标写反了一处 |
| Eigen 编译期报错巨长 | 贴完整报错 + 源码给 AI（第 7 周 CGAL 模板报错的预演） |
| cmake 配置时联网下载 Eigen | 本机没装 Eigen，FetchContent 兜底（仅首次）。mac：`brew install eigen` 可跳过 |

---

## 最后一句

> 第 5 周你让 AI 把数学推对了；这一周你让它把数学**落成又快又对的工业代码**。
> CSE 是 `cse()` 一行的白捡加速，稀疏组装是 Triplet 一次成型的纪律——
> 但"这段代码到底快不快、组装方式对不对"，
> **AI 给你候选，计时器和稠密参考替你判分，而"否决 insert、否决临时矩阵"这些经验，只有你能写进 prompt。**
