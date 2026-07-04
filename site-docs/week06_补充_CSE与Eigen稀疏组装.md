# 第 6 周补充：CSE 与 Eigen 稀疏组装的最佳实践

> 配套代码：[`code/week06_A/`](https://github.com/liyongzheng666/geometry-engineer-ai-road/tree/main/code/week06_A)
> 这篇讲两件工程细节：`sympy.cse()` 到底做了什么、生成的 C++ 怎么组织；
> 以及 Eigen 稀疏矩阵为什么必须 `Triplet + setFromTriplets`、`insert()` 到底慢在哪。

## 一、cse() 做了什么

公共子表达式消除（Common Subexpression Elimination）就是：找出表达式里重复出现的子式，
提成临时变量算一次，后面复用。SymPy 的 `cse` 返回两部分：

```python
common, reduced = cse([expr1, expr2, ...])
# common  = [(t0, 子式0), (t1, 子式1), ...]   按依赖顺序，t1 可以用 t0
# reduced = [用 t0,t1... 改写后的 expr1, expr2, ...]
```

生成 C++ 时**必须按 `common` 的顺序声明**——它已经拓扑排序好了，`t1` 的定义可能用到 `t0`。
如果 AI 打乱顺序，就会出现"用了还没声明的变量"，轻则编译错、重则读到垃圾值
（这正是 `test_cse_equiv` 的 Test 2 要抓的）。

```cpp
// cse 输出的标准形态：先一串 const double 临时量，再写结果
const double t0 = x - AX;
const double t1 = std::sqrt(t0*t0 + ...);   // 用到 t0
const double t2 = 1.0 / t1;
H[0] = STIF * (1.0 - REST*t2 + ...);        // 用到 t2
```

**为什么 `cse` 比"让 AI 手动提"强**：AI 手动提子式全凭眼力，漏一个少一个、提错一个引 bug；
`cse` 是确定性算法，且和 `simplify()` 不同——它**不改写数学形式**，只做机械的子式复用，
所以数值必然一致（到浮点重排精度）。第 5 周用 `simplify()`，第 6 周改用 `cse()`，
就是因为 simplify 对大表达式又慢又可能改变形式，cse 又快又保结果。

## 二、CSE 的加速来自哪里

不是所有子式都值得提。提一个 `x-AX`（一次减法）省不了多少；提一个 `sqrt(...)`
或 `pow(...,1.5)` 才是真赚——**超越函数比四则运算贵一到两个量级**。

本周 harness 里 naive 版把 Hxx/Hxy/Hyy 写成三段，各自重算每个锚点的 `sqrt`+`pow`；
cse 版每个锚点的距离只算一次，三个输出共享。5 个锚点、3 个输出 → naive 算 ~15 次 sqrt/pow，
cse 只算 ~5 次 sqrt。实测 ~3x 加速，基本就是这个比例。

> 经验：**看 cse 收益，先数超越函数调用减了多少**，别看总代码行数。

## 三、Eigen 稀疏矩阵：为什么是 Triplet

Eigen 的 `SparseMatrix`（默认 CSC 压缩列存储）一旦压缩，**在已压缩结构里插入一个新非零，
要移动后面所有元素**——O(nnz) 的搬移。所以两种构建方式天差地别：

| 方式 | 做法 | 复杂度 | 场景 |
|---|---|---|---|
| ❌ `insert(i,j)` 逐个塞 | 每次都在压缩结构里找位置、可能触发重排 | 最坏 O(nnz) / 次 | 只适合"事先知道每列非零数并 reserve"的极少数情况 |
| ✅ `Triplet + setFromTriplets` | 先把所有 (行,列,值) 收进数组，最后一次性排序建结构 | O(nnz log nnz) 总 | **组装的默认选择** |

`setFromTriplets` 还有个关键语义：**重复的 (i,j) 自动求和**。有限元/网格组装里，
一个 dof 被多个单元/边共享，每个都往同一格贡献一份——你只管把每份都 `emplace_back`
成一个三元组，求和是免费的。这就是本周 `test_sparse_assembly` Test 1 的核心考点。

```cpp
std::vector<Eigen::Triplet<double>> trips;
trips.reserve(edges.size() * 16);                 // 预估，省 vector 扩容
for (auto& e : edges)
    for (a,b in 4x4 local block)
        trips.emplace_back(global_row, global_col, value);  // 共享 dof 自动累加
Eigen::SparseMatrix<double> H(2*nv, 2*nv);
H.setFromTriplets(trips.begin(), trips.end());
```

## 四、对称矩阵还能再省一半

弹簧/有限元 Hessian 通常对称。Eigen 允许**只填上三角**，用时以
`H.selfadjointView<Eigen::Upper>()` 引用下三角——存储和组装都省一半。本周 harness 为了
让稠密对照直观，填了完整矩阵；真实大规模项目里，只填上三角是标准做法。这也是可以写进
prompt 的一条工业约束："只填上三角，最后用 selfadjointView 引用"。

## 五、把这些写进 prompt

和 Eigen 配合时，AI 默认给的不一定最优。真正把工业经验注入的，是 prompt 里那些**否决句**：

- "用 Triplet + setFromTriplets，**不要用 insert()**"
- "**不要**构造显式的 N×3 中心化临时矩阵"（第 3 周平面拟合的教训）
- "只填上三角，用 selfadjointView 引用"
- "预先 reserve 三元组数量"

> 一句话：**AI 知道 Eigen 的 API，但不知道你的规模和权衡。
> `cse()` 的加速要靠你去数超越函数，`insert` 的坑要靠你在 prompt 里提前否决——
> 而 harness 的计时和稠密参考，会告诉你这些经验有没有真的生效。**
