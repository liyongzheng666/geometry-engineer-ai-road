# 第 1 周实操：AI 代码验证闭环

> 配套大纲：[A轨_12周总纲.md](A轨_12周总纲.md) 第 1 周
> 配套代码：[`code/week01_A/`](../code/week01_A/)（clone 即用，构建方法见其 [README](../code/week01_A/README.md)）
>
> 目的：解决"AI 生成的 C++ 代码到底对不对"——通过一个**零依赖的最小工作台**，
> 把 AI 输出粘进 PASTE ZONE 就能编译、跑测试、看 SVG 可视化。

---

## 设计哲学

| 决策 | 理由 |
|---|---|
| 不用 Python plotting / matplotlib | 多一个依赖少一份动力 |
| 不用 OpenGL / Qt | 杀鸡用牛刀 |
| 不用第三方库（Eigen 都不用） | 第 1 周专注 prompt 训练，不混入工具学习 |
| 输出 SVG（不是 PNG） | 浏览器直接打开，矢量缩放清晰，**纯文本可 diff** |
| 单文件单测试 | 每个算法独立编译，不互相干扰 |
| 在终端打印数值 + 同时画图 | 数值供 assertion，图形供直觉判断 |

## 工作台里有什么

`code/week01_A/` 是一个标准 CMake 工程（Windows VS / macOS clang / VSCode 三种用法都开箱即用）：

| 文件 | 被测接口（你只动 PASTE ZONE） | 验证金标准 |
|---|---|---|
| `test_bezier.cpp` | `bezier_cubic` — 三次 Bezier 求值 | 闭式 Bernstein 公式独立对照 |
| `test_bspline.cpp` | `basis` — Cox-de Boor 基函数 | partition of unity / 非负性 / u=1 边界 |
| `test_nurbs.cpp` | `nurbs_eval` — NURBS 曲线求值 | **圆性测试**：标准权重 1/4 圆必须落在单位圆上 |
| `geo.h` | （公共头，不要改）Point3D + SVG writer | — |

初始状态下三个 PASTE ZONE 都是占位桩，测试全 FAIL——这是预期，等你把 AI 的实现粘进去。

---

## 完整工作流（融合进第 1 周日程）

### 周一-周二（对照实验）

1. 在 AI 上对同一任务发**三种 prompt**（大白话 / 加公式 / 加算法+约束，见总纲第 1 周表格）→ 拿到 3 段 Bezier 代码
2. 各自粘进 `test_bezier.cpp` 的 PASTE ZONE（建议保留三份副本：`test_bezier_A/B/C.cpp` 放本地，不用提交）
3. 各自编译运行，对比观察：
   - 数值测试：哪几版能 PASS 所有 5 个 t 值？
   - 端点测试：哪几版 P(0)、P(1) 严格精确？
   - 退化测试：哪几版能扛住四点重合？
   - SVG：曲线形态是否合理？有无尖角、突变、跑飞的点？

**这一步把"主观判断 prompt 谁好"变成客观数据**——A 版可能在 t=0.999 处偏离 ref 1e-3，
B 版 1e-6，C 版 1e-15。亲眼看到这个差距，prompt 工程的价值就立起来了。

### 周三（Cox-de Boor）

1. 用总纲第 1 周的"思维链 prompt"（先分析除零位置和半开区间，再写码）拿到 `basis`
2. 粘进 `test_bspline.cpp`，编译运行
3. 三个核心验证：**Partition of unity**（任意 u 处和=1）、**Non-negativity**、**u=1.0 边界行为**
4. SVG 里看 7 条彩色曲线是否光滑、形态对称

如果前两个数学性质 FAIL，说明递推公式实现错了——这是无可辩驳的客观证据，
比让 AI 自己"声称我的代码是对的"强 100 倍。

### 周四（NURBS）

1. 用总纲第 1 周的"奇点声明 prompt"（先列权重/重节点/归一化的奇异清单）拿到 `nurbs_eval`
2. 粘进 `test_nurbs.cpp`，编译运行
3. **NURBS 最严苛的客观测试**：1/4 圆标准权重下，求值点的 √(x²+y²) 必须 = 1
4. SVG 里红色弧线必须**严丝合缝**贴在灰色参考圆上

任何隐性 bug（权重归一化、节点处理、De Boor span）都逃不过圆性测试——
这是几何代码验证最经典的金标准（circularity test）。

### 周末（综合）

1. 把三份测试都跑 PASS 的最佳版本固化在 PASTE ZONE 里，作为你的"AI 代码验证基线"
2. 按总纲第 1 周周四的要求，把三个 prompt 套路抽进 [`prompt_library/`](../prompt_library/)
3. 开始记 `failure_log.md`（格式见总纲）

下周开始新算法时，沿用这个范式：**先有测试 harness，再让 AI 写代码，跑测试直到 PASS**。

---

## 这套工具的"杠杆"在哪里

1. **从"主观判断"到"客观度量"**
   "prompt A 不太好" ≠ "prompt A 在 t=0.999 处误差 3.2e-4，prompt C 同位置 1.1e-15"。
   后者会让你深刻记住 prompt 工程的价值。

2. **从"AI 自夸"到"测试打脸"**
   AI 经常说"我的代码处理了所有边界情况"。圆性测试 / partition-of-unity 一跑，
   牛皮就破了。这是你建立**对 AI 的判断力**的基础。

3. **从"看代码"到"看图"**
   人脑读 100 行 C++ 比看一张曲线图慢 100 倍。SVG 让你 1 秒发现
   "这条曲线为什么在中间凸了一下"。

4. **后续 11 周的复用**
   第 2 周（退化输入）、第 3 周（风格注入）的工程就是同一范式的延续：
   **harness + paste-zone + numeric checks + SVG**。

---

## 常见踩坑

| 现象 | 原因 | 应对 |
|---|---|---|
| 编译报错 `Point3D` 重定义 | AI 在它的代码块里也定义了 Point3D | 删掉 AI 那份，用 `geo.h` 里的 |
| 编译报错 `std::vector` 未声明 | AI 没 include `<vector>` | harness 已 include，删 AI 重复的 |
| SVG 打开是空白 | 坐标范围不对，曲线在 viewBox 外 | 检查 SVG 的 viewBox 是否包含数据范围 |
| 测试一个都没 PASS | AI 误解了输入约定 | 拿 1-2 个具体输入手算，对照哪一步偏离 |
| 圆弧测试明显偏离 | NURBS 权重 / 端点 / De Boor span 错 | 看 SVG，从哪个 u 开始偏离就是 bug 起点 |
| 构建 / 断点问题 | — | 见 [week01_A/README.md](../code/week01_A/README.md) 的"常见问题"表和三篇配套笔记 |

---

## 进阶（可选，周末再做）：给 prompt 加测试约束

下周可以在 prompt 模板里加上：

```text
代码必须能通过以下测试：
1. P(0) 严格等于 P0（精度 1e-15）
2. P(1) 严格等于 P3（精度 1e-15）
3. 任意 t ∈ [0,1] 时，与 Bernstein 闭式公式的差 < 1e-10
4. 4 个控制点全部相等时，对任意 t 返回该点
请确保你的实现通过以上每一条。
```

这让 AI 的目标从"写一个看起来对的函数"变成"**写一个能通过这些 assertion 的函数**"——
后者质量陡升一个台阶。

---

## 最后一句

> 没有验证闭环的 AI 输出，**只是文本**；
> 有了验证闭环的 AI 输出，才是**可信代码**。
> 第 1 周这套 harness，会让你之后 11 周对 AI 产出的判断力，
> 从"看起来对"升级到"测试过了"。
