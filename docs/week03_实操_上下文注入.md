# 第 3 周实操：上下文注入——让 AI 用你的代码风格

> 配套大纲：[A轨_12周总纲.md](A轨_12周总纲.md) 第 3 周
> 配套代码：[`code/week03_A/`](../code/week03_A/)（构建方法同 week01_A；`test_plane_fit` 需要 Eigen，见其 [README](../code/week03_A/README.md)）
>
> 周目标：解决"AI 总用 std::vector 不用我的 ArrayN"这种**风格污染**问题。
> 本周算法都是基本功（三点定圆、平面拟合），考点在别处：
> **把"公司项目头文件 + 禁令清单"注入上下文后，AI 是否真的服从。**

---

## 练习材料：mini_types.h

`code/week03_A/mini_types.h` 模拟一份"公司项目核心几何头文件"——
`gx::Vec3` / `gx::GxStatus` / `gx::kEpsGeom`，风格**刻意与 geo.h 相反**：

| 风格维度 | geo.h Point3D | gx::（mini_types.h） |
|---|---|---|
| 运算 | 运算符重载 `+,-,*` | **成员函数** `.add()/.sub()/.scaled()` |
| 数据访问 | public 成员 `x,y,z` | private 数组 + 访问器 `x()/y()/z()` |
| 错误处理 | 无 | **错误码 GxStatus + 输出参数**，禁异常 |
| 容差 | 魔法数 | 项目常量 `gx::kEpsGeom` |

差异越大，"AI 有没有服从注入"越容易判分。在你的真实项目里，
这一步对应的是把 `Point3D.h` / `Vector3D.h` / `Matrix3x3.h` 全文贴进去。

---

## 周一-周二：三点定圆 × 类型注入（4h）

工作台：`test_circumcircle.cpp`。prompt 结构（总纲第 3 周模板的具象化）：

```text
以下是项目的核心几何类型头文件，请严格使用其中定义的类和方法：
**禁止引入任何 STL 容器**，**禁止异常 / std::optional**，
**禁止运算符重载式写法（用 .add()/.sub()）**，容差一律用 gx::kEpsGeom。

[mini_types.h 全文粘贴在这里]

任务：实现三维空间中过三点的外接圆。
函数签名：gx::GxStatus circumcircle(const gx::Vec3& a, const gx::Vec3& b,
                                    const gx::Vec3& c, gx::Circle3& out);
退化输入（共线 / 重合）返回相应的 GxStatus，并先于任何除法检测。
```

**实务技巧**（总纲原文，这周亲自验证）：
- 把头文件**全文**贴进去，比简化描述效果好 10 倍；
- 一次会话做完相关功能，避免重复贴大段头文件；
- 单次会话超过 30k tokens 后效果下降，及时开新会话。

跑 harness 后看两层：
- **数值层**：Test 1-4（含三维倾斜圆——很多 AI 只会写 2D 版）+ Test 5/6 退化必须失败；
- **风格层**（本周真正的考点，main 末尾会提醒）：
  ① PASTE ZONE 里有没有 std:: 容器？② 有没有偷偷用运算符？③ GxStatus 语义对不对？

**对照实验**：把禁令清单删掉再发一次同样的任务。多数 AI 会立刻回到
`std::optional<Circle>` + 运算符重载的"默认风格"——这就是上下文注入的价值的直观证据。

---

## 周三-周四：Eigen 平面拟合（4h）

工作台：`test_plane_fit.cpp`。用总纲第 3 周的 Eigen 模板（要点全部写进了
PASTE ZONE 上方注释）：

```text
用 Eigen 实现 N 个三维点的最小二乘平面拟合。
约束：
1. 输入是 const double*（row-major，N×3），通过 Eigen::Map 处理，避免内存拷贝
2. 求法向：对 3×3 协方差矩阵做 SVD / 特征分解，取最小奇异值方向
   ——不要对 N×3 矩阵做 SVD
3. 输出：单位法向 normal + 平面上一点 centroid
4. 退化处理：N < 3 → false；共线（σ_min/σ_mid 比值）→ false
5. 性能要求：避免 N×3 的显式中心化临时矩阵
给出代码并解释每个性能关键点的原因。
```

**学到的**（总纲原话）：和 Eigen 配合时，AI 经常给出"能跑但低效"的代码（多次临时拷贝）。
**你的价值是认得出这种低效**——Test 7 用 N=200000 计时给你客观证据：
正确实现 < 1ms，反复拷贝的实现会到数百毫秒。

验证金标准：
- Test 1 精确共面恢复（法向允许反号，比 |normal·n0| 与 1 的差）+ 手写 Cramer 法 z-拟合双算对照；
- Test 3 固定种子加噪（±0.01），角偏差必须 < 0.01 rad；
- Test 6 观察题："几乎是线"的输入接受还是拒绝——**拒绝阈值该由谁定，AI 还是你的 prompt？**

---

## 周五：Code Review（2h）

没有 harness——把**你自己写过的**一段复杂几何代码（网格曲率、半边遍历类）发给 AI，
用总纲第 3 周的 review 模板（堆分配 / 缓存命中 / 重复计算 / aliasing / 复杂度，
每个 issue 给行号 + 严重程度 + 改进方案，不准风格批评）。

**接受率应该在 30-50%**——太低说明 prompt 不准，太高说明你之前代码太糙。

---

## 周末：建立上下文片段库（1h）

不是 RAG，就是一个目录的纯文本片段（总纲第 3 周原文）：

```text
context_snippets/
├── core_types.txt          # 你项目的 Point3D / Vector3D / Matrix 头文件聚合
├── halfedge_mesh.txt       # 你的半边数据结构
├── nurbs_types.txt         # 你的 NURBS 表示
└── project_conventions.md  # 命名、错误处理、容差约定
```

本周用 mini_types.h 练的手感，原样迁移：每次开 AI 会话，**先粘片段再说任务**。
两周后你会感受到效率倍增。同时把本周模板抽进 [`prompt_library/`](../prompt_library/)
（建议 `01_algorithm_design/context_injection.md`），更新 `failure_log.md`。

---

## 常见踩坑

| 现象 | 原因 / 应对 |
|---|---|
| circumcircle 三维倾斜圆（Test 4）FAIL | AI 写的是 2D 公式（只用 x/y）。prompt 里强调"三维空间中" |
| AI 用了 `std::sqrt` 以外的 STL | 禁令写得不够具体。"禁止 STL 容器"和"禁止一切 std::"是两回事，按需收紧 |
| plane_fit Test 7 耗时数百 ms | 显式构造了 N×3 中心化矩阵或对 N×3 做 SVD——这正是观察题要你抓的 |
| Eigen 编译期报错巨长 | 贴**完整报错 + 源码**给 AI（第 7 周 CGAL 模板报错的预演） |
| cmake 配置时联网下载 Eigen | 本机没装 Eigen，FetchContent 兜底（仅首次）。mac：`brew install eigen` 可跳过 |
| MSVC /W4 下 Eigen 头刷警告 | 已知现象，可忽略；不想看就给 test_plane_fit 单独降警告级 |

---

## 最后一句

> 第 2 周教会你"把隐性的工业实务说出来"；
> 这一周教会你"把隐性的**项目惯例**贴进去"。
> 两者合起来，AI 写出的代码才开始像"你团队的代码"，
> 而不是"Stack Overflow 的代码"。
