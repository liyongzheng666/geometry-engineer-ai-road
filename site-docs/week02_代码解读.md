# 第 2 周代码解读：三个退化重灾区的 harness 设计

> 对应代码：[`code/week02_A/`](https://github.com/liyongzheng666/geometry-engineer-ai-road/tree/main/code/week02_A)
> 本周三个 harness 比第 1 周多了一类东西——**观察题**：没有 PASS/FAIL，
> 只打印 AI 代码在退化输入下的实际行为，然后问你一句"它声明过这个约定吗？"

## 一、test_point_inversion：基础设施分层

点到 B 样条曲线投影需要曲线求值和导数，但**这些不是本周的练习目标**。
所以文件刻意分了两层：

```text
PASTE ZONE 之前（基础设施，使用者不动，但允许调用）：
    bspline_basis / bspline_basis_deriv     ← 基函数及 k 阶导
    curve_eval / curve_deriv / curve_deriv2 ← C(u), C'(u), C''(u)
    dot3                                    ← 点积

PASTE ZONE（你唯一要动的）：
    point_inversion(...)                    ← Newton 迭代外壳
```

实操建议直接写在文件头：**把基础设施函数的声明原样贴进 prompt**，
让 AI 在给定接口上只写 Newton 部分。这正是"接口聚焦"——
你控制问题边界，AI 填空。

### 返回结构体而不是裸 double

```cpp
struct InversionResult {
    bool   converged;   // 是否收敛（fallback 成功也算）
    double u;           // 最近点参数
    double distance;    // |C(u) - P|
    int    iterations;  // 实际迭代次数
};
```

`iterations` 字段是个小探针：Newton 二次收敛正常 3~6 次。
如果 AI 的实现回报 30 次还"converged"，多半初值策略有问题。

### 金标准 1：由答案构造输入

```cpp
const Point3D C  = curve_eval (u_star, ...);
const Point3D Cp = curve_deriv(u_star, ...);
const Point3D n  = 单位法向(Cp);
const Point3D P  = C + n * 0.25;   // 沿法向偏移 0.25
// 投影 P 必须返回 u_star，distance 必须 = 0.25
```

先定答案（u*），再造输入（P）——只要偏移距离小于局部曲率半径，
最近点**数学上必然**回到 u*。这比"和参考实现对一下"硬得多：
参考实现可能跟被测实现错得一样，构造出来的答案不会。

### 金标准 2：完全不同的算法路径

参考实现 `inversion_ref` 是**密集采样 4001 点 + 黄金分割细化**——
没有导数、没有迭代、不会发散，跟 Newton 没有一行共同逻辑。
慢但稳，正适合当裁判。

### 两道观察题

- **等距双解**（Test 5）：测试曲线刻意做成左右对称，P 放正下方——u=0 和 u=1
  严格等距。AI 收敛到哪侧？粗采样初值策略能保证可复现吗？
- **预算不足**（Test 6）：max_iter=2 强行掐断。AI 返回什么？
  `converged=false` 时 `distance` 字段还可信吗？——这是 API 契约设计问题，
  不是数学问题，AI 通常不会主动想。

## 二、test_point_in_polygon：让 tie-breaking 无处可藏

四个测试多边形各打一个痛点：

| 多边形 | 打什么 |
|---|---|
| 正方形 | 基线（这都不过就别往下看了） |
| 凹"飞镖" | 凹口处射线要数 2 次交叉；左角在 y=3.5 处只有 0.4 宽——内外只差 0.1 |
| 菱形 | 左右顶点 y=2，query 的 y 也取 2：**射线恰好穿顶点** |
| 台阶形 | 含一条水平边，射线与它**整条共线** |

朴素实现"碰到顶点算一次交叉"会把共享顶点的两条边各数一遍——内外判反。
经典解法是半开区间规则：

```cpp
if ((yi > P.y) != (yj > P.y)) {        // 每条边只在"跨越"时计数
    x_cross = ...;
    if (x_cross > P.x) inside = !inside;
}
```

顶点恰在射线上、水平边共线，全部被这一个判断自然吞掉——
**好的 tie-breaking 不是补丁堆出来的，是规则本身选对了**。

参考实现用 winding number（atan2 有向角度和），数学上和射线法毫无血缘。
SVG 是张"审判图"：按 0.15 步长网格采样染色（内绿外灰），
凡是两种算法不一致的点画红色大点——AI 的规则错在哪个区域，一眼可见。
（近边界 0.05 带跳过双算：winding 在边界上数值无定义。）

## 三、test_ray_triangle：一个真实翻车案例

Möller-Trumbore 的参考实现是传统两段法（平面求交 + 重心坐标 2×2 方程），
另加一条**与任何实现无关**的重构校验：

```cpp
|  (orig + t·dir)  −  ((1-u-v)·A + u·B + v·C)  |  <  1e-9
```

t 和 (u,v) 是同一个交点的两种表达——对不上就是内部不一致。

### 这个坑我们自己先踩了

开发这个 harness 时，第一版"正确实现"写的是：

```cpp
const double eps = 1e-12 * norm(e1) * norm(e2) * norm(dir);
if (std::abs(det) < eps) return miss;     // ← 三点全重合时翻车
```

三点全重合 ⇒ e1 = e2 = 0 ⇒ **eps 也是 0** ⇒ `0 < 0` 为假 ⇒
继续除以 det=0 ⇒ NaN 一路传染 ⇒ NaN 和谁比较都是 false ⇒
所有拒绝分支全部跳过 ⇒ **返回 hit=true，t/u/v 全是 NaN**。

修复是一个字符：`<` 改 `<=`。Test 6 现在就埋着这个用例，
专等 AI 的实现来踩。**相对容差在退化输入下会和被检测量一起归零**——
这是几何代码里最隐蔽的坑型之一。

### 两道观察题

- **watertight**（Test 7）：边/顶点命中时 u≥0 还是 u>-eps？
  两个共享一条边的三角形，光线打在公共边上是双命中还是漏命中？
  射线求交写进网格求交器之前，这个约定必须想清楚。
- **背面**（Test 8）：det<0 的背面入射，AI 默认单面（culling）还是双面？
  你的 prompt 里写了吗？

## 四、本周的元规律

三个 harness 的退化用例摆出来后你会发现：**AI 不是不会处理退化，
是不知道你要哪种处理**。夹紧还是报错、true 还是 -1、单面还是双面——
这些都是合法选项，AI 随机挑一个。你的 prompt 把约定写死，它就稳定；
不写，它每次重新掷骰子。这就是"退化检查清单"模板存在的意义。
