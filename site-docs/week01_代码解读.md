# 第 1 周代码解读：验证闭环是怎么搭起来的

> 对应代码：[`code/week01_A/`](https://github.com/liyongzheng666/geometry-engineer-ai-road/tree/main/code/week01_A)
> 这篇讲的不是"Bezier 怎么算"，而是**这个工作台为什么长这样**——读懂它，
> 后面每一周的 harness 你都能秒懂，甚至能给自己的算法照着搭一个。

## 一、geo.h：60 行撑起整个实验室

`geo.h` 是全部三周唯一的公共依赖，只有两样东西：

### Point3D：刻意保持"裸"

```cpp
struct Point3D {
    double x = 0, y = 0, z = 0;
    Point3D operator+(const Point3D& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Point3D operator-(const Point3D& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Point3D operator*(double s) const { return {x*s, y*s, z*s}; }
};
```

注意它**没有** dot/cross/norm。这是故意的：

- 第 1 周的三个算法（Bezier / B 样条 / NURBS）本质都是**仿射组合**——只需要加法和数乘。
  类型本身就在提示你这件事。
- 缺什么运算，由各周 harness 在自己文件里以 `static` 函数补（第 2 周的
  `dot3/cross3` 就是这么来的），不污染公共头。

### SVG writer：30 行的可视化方案

```cpp
SVG svg("bezier_output.svg", -1, -1, 6, 5);   // viewBox 范围
svg.polyline(curve, "#2266ff", 0.04);          // 折线
svg.dots(ctrl, "red", 0.08);                   // 点
```

构造时写 SVG 头，析构时写尾——**RAII 管文件生命周期**，所以 main() 里
SVG 代码都包在一对花括号里：作用域一结束文件就完整落盘。
里面有个小细节：`transform='matrix(1 0 0 -1 0 0)'` 把 y 轴翻上去，
让数学坐标系（y 向上）和 SVG 坐标系（y 向下）对齐。

## 二、harness 的五要素解剖

打开任何一个 `test_*.cpp`，结构都是这五块：

```text
① 文件头注释          —— 用途 + 用法两行
② PASTE ZONE          —— 被测函数（占位桩），使用者唯一要动的地方
③ 独立参考实现        —— static 函数，用【另一条数学路径】算同一个值
④ main()              —— enable_utf8_console + check lambda + 逐项 Test
⑤ SVG 作用域块        —— 可视化 + filesystem::absolute 打印绝对路径
```

关键设计是 ②③ 的关系：**被测实现和参考实现绝不共享代码**。
AI 写 De Casteljau，参考就用 Bernstein 闭式；AI 写递推，参考就用解析性质。
两条路殊途同归才算 PASS——这叫"客观双算对照"，AI 自夸无效。

## 三、三个金标准为什么这么选

### test_bezier：Bernstein 闭式 vs De Casteljau

被测函数推荐用 De Casteljau（数值稳定），参考实现用 Bernstein 展开：

```cpp
static Point3D bernstein_ref(..., double t) {
    const double u = 1.0 - t;
    return P0*(u*u*u) + P1*(3*u*u*t) + P2*(3*u*t*t) + P3*(t*t*t);
}
```

两者是同一条曲线的两种算法，在 [0,1] 内部相互印证。另外三组测试各打一个点：
端点插值（P(0)=P0 必须**严格**成立，不是近似）、四点重合（退化）、
t 越界（观察题：AI 是夹紧、外推还是抛异常——你的 prompt 说了吗？）。

### test_bspline：用数学性质当裁判

B 样条基函数没有简单的闭式参考，但它有**与实现无关的数学性质**：

- **Partition of unity**：任意 u 处所有基函数之和恒等于 1
- **非负性**：N ≥ 0 处处成立
- **u = 1.0 边界**：标准 Cox-de Boor 的半开区间 [u_i, u_{i+1}) 会让
  u=u_max 处所有基函数归零——sum=0、NURBS 除零得 NaN。
  正确实现要把"最后一个非空 span"右端闭合。**这是本周最阴的 corner case**，
  AI 第一版几乎必踩。

### test_nurbs：圆性测试（circularity test）

NURBS 行业里最经典的金标准：取 3 个控制点 + 权重 `(1, √2/2, 1)` +
节点 `{0,0,0,1,1,1}`，这是**精确的 1/4 单位圆**。于是验证只需一行数学：

```text
对任意 u：√(x² + y²) 必须 = 1（误差 < 1e-6）
```

权重、节点、De Boor span、归一化——任何一处错，点就掉下圆。
对照组（权重全 1）退化为普通 B 样条，|P(0.5)| ≈ 0.85，明显离圆，
SVG 里红（圆弧）灰（真圆）蓝（退化）三线并排，一眼定胜负。

## 四、占位桩的设计

```cpp
Point3D bezier_cubic(...) {
    // TODO: 在这里粘贴 / 实现三次 Bezier 求值（如 De Casteljau 算法）
    (void)P0; (void)P1; (void)P2; (void)P3; (void)t;
    return Point3D{0, 0, 0};  // 占位：未实现
}
```

三个细节：

1. `(void)参数` 压掉未使用参数警告——保证仓库初始状态**编译零警告**；
2. 返回零值而不是抛异常——所有测试能跑完、打出完整 FAIL 清单，而不是中途崩掉；
3. **全 FAIL 是预期状态**：它逼你先看一遍"测试都测什么"，再去写 prompt。

## 五、迁移到你自己的算法

照这个模板给任何几何算子搭闭环，自查清单：

- [ ] 参考路径和被测路径**数学上独立**（不同公式 / 不同性质）
- [ ] 至少一个**由答案构造输入**的测试（先定答案再造输入，比对照参考更硬）
- [ ] 至少一个退化输入 + 一个观察题（AI 声明过它的边界约定吗？）
- [ ] 数值打印 + 图形可视化双通道
- [ ] 占位桩可编译、全 FAIL、零警告
