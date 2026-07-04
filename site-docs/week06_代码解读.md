# 第 6 周代码解读：把"优化没改坏结果"变成可判定的测试

> 对应代码：[`code/week06_A/`](https://github.com/liyongzheng666/geometry-engineer-ai-road/tree/main/code/week06_A)
> 本周两条主线——CSE 自动优化、Eigen 稀疏组装——harness 的共同母题是：
> **每一步优化都要能被一个独立参考当场验对错。** 优化不该靠"应该没问题"。

## 一、codegen_ref.h：复用第 5 周能量，但改用 5 个锚点

CSE 要有"料"可提。3 个锚点的 Hessian 重复子表达式还不够多，加到 5 个后，
每个输出项都要重算 5 个 `sqrt`/`pow`，naive 和 cse 的差距就拉开了。`hessian_ref`
是**干净的循环版**（每个 `d_i` 只算一次），当正确性锚点——注意它既不是"未 cse"
也不是"cse 输出"，而是第三条独立路径，用来抓"naive 和 cse 同错"。

计时器 `time_ms` 用 `std::chrono::high_resolution_clock` 包一个 reps 循环，
故意做得极简——本周计时是"数量级对比"，不追求 benchmark 级严谨。

## 二、test_cse_equiv：三条测试是一条验证链

```text
Test 1  hessian_naive vs hessian_ref  < 1e-9    → 确认 ccode 本身对（naive 是基准）
Test 2  hessian_cse   vs hessian_naive < 1e-12  → CSE 的核心承诺：结果一字不改
Test 3  计时 naive vs cse                        → CSE 的收益：实测快约 3x
```

顺序有讲究：先用独立参考钉死 naive（Test 1），再用 naive 钉 cse（Test 2）。
这样 Test 2 的 1e-12 才有意义——它比的是"两版符号代码"，差异只该来自浮点重排
（`pow(d²,1.5)` vs `d²·√d²`），到机器精度即可。**如果 CSE 把某个临时变量的依赖顺序
排错了（用了还没赋值的量），Test 2 立刻爆到 O(1)。**

Test 3 的加速来自哪里？naive 版（模拟未 cse 的 ccode）把 Hxx/Hxy/Hyy 写成三段独立循环，
每段都重算 `sqrt` 和 `pow`；cse 版每个锚点的 `d`、`inv_d`、`inv_d3` 只算一次，三个输出项
共享。省掉的主要是**超越函数调用**（sqrt/pow 比乘法贵得多），所以实测 ~3x。
SVG 把两版 Hxx(x) 画成蓝粗+橙细，重合时橙线像给蓝线镶了道边——**数值一致的可视化冗余**。

## 三、test_sparse_assembly：稠密参考是最诚实的裁判

组装稀疏矩阵最容易错在两处：**共享 dof 求和**和**下标映射**。harness 用一个
`dense_assemble` 参考——它就是最朴素的"开一个 2N×2N 稠密矩阵，按 dof 下标 `+=`"。
朴素到不可能写错，正好当金标准：

```text
Test 1  (dense − MatrixXd(sparse)).cwiseAbs().maxCoeff() < 1e-12
```

内部顶点被相邻两条边共享，两条边都往同一个对角块贡献。`setFromTriplets` 的语义是
**对重复三元组求和**——如果 AI 误以为是"覆盖"，或自己写了去重逻辑，Test 1 立刻现形。
这就是为什么大纲反复强调"用 Triplet 收集 + 一次 setFromTriplets"：**求和语义是免费的正确性**。

Test 2 查全局对称（局部块对称 + scatter 对称 → 全局必对称，任一处行列写反就破）。
Test 3 用两万顶点计时，并在输出里点破 insert() 的代价——它不判分，是把"为什么否决 insert"
从一句教条变成一个你能亲手改大 N 去验证的实验。

`write_spy` 把稀疏矩阵每个非零画成一格，12×12 的例子呈**块三对角**：相邻顶点耦合成
2×2 块，非相邻顶点全零。一眼确认"组装出的稀疏结构 = 网格连接结构"。

## 四、本周的元规律

第 5 周说"让有限差分给推导当裁判"，第 6 周把同一信条推到优化上：
**CSE 有 Test 2 当裁判、稀疏组装有稠密参考当裁判**。于是"优化"不再是提心吊胆的手艺——
你可以放心让 AI 做激进优化，因为任何"优化改坏了结果"都会被独立参考当场抓住。
**有了自动裁判，你就敢让 AI 跑得更快。**
