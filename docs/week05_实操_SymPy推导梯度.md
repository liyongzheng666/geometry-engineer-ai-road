# 第 5 周实操：SymPy + AI 推导梯度（C++ 有限差分验证）

> 配套大纲：[A轨_12周总纲.md](A轨_12周总纲.md) 第 5 周
> 配套代码：[`code/week05_A/`](../code/week05_A/)（clone 即用，零依赖，构建方法同 [week01_A](../code/week01_A/README.md)）
>
> 周目标：告别手算偏导数，且**能验证 AI 推导没出错**。
> 本周把 AI 当符号推导搭档，但**坚持第 1 周的 C++ 数值金标准**——SymPy 只当"参考生成器"，
> 真正粘进 PASTE ZONE 的是它 `ccode()` 吐的 C++，再用手写中心差分对撞。

---

## 设计哲学：为什么坚持"C++ 数值验证"

总纲第 5 周的原范式是 Python/SymPy。但我们把验证那一环**焊死在 C++ 里**，理由：

| 决策 | 理由 |
|---|---|
| SymPy 只当"生成器"，不当"最终答案" | AI 手推偏导 70% 会漏负号 / 漏 1/d；符号推导必须机器跑 |
| 生成的 C++ 粘进 harness，对撞有限差分 | 有限差分与解析式走**完全不同的数值路径**，是无可辩驳的金标准 |
| 能量只用 2 个变量 | 有限差分廉价、参考可信；却含 sqrt + 链式，最能暴露符号错 |
| 和第 1-4 周共用"harness + PASTE ZONE + 数值双算 + SVG" | 六周一套心智模型，不为第 5 周单独学一套 Python 验证栈 |

**一句话**：SymPy 负责"推得快"，C++ 有限差分负责"信得过"。

## 工作台里有什么

`code/week05_A/`：`energy.h` 是公共头（能量 + 中心差分工具 + 能量场 SVG，**不要改**）。
能量模型是一个自由点被 3 根弹簧连到 3 个锚点：`E(p) = Σ_i ½·k_i·(‖p−a_i‖−L_i)²`。

| 文件 | 被测接口（你只动 PASTE ZONE） | 验证金标准 |
|---|---|---|
| `test_gradient.cpp` | `compute_gradient`（SymPy 生成） | 中心差分梯度，max 相对误差 < 1e-6；**专抓漏负号** |
| `test_hessian.cpp` | `compute_hessian`（SymPy 生成） | 对已验证解析梯度再差分 + 对称性 Hxy==Hyx |

初始 PASTE ZONE 是占位桩（返回全 0），测试全 FAIL——预期。

---

## 完整工作流（融合进第 5 周日程）

### 周一-周三：让 AI 写 SymPy 脚本求梯度

**错误做法**：让 AI 直接手推 ∂E/∂x 贴给你。**正确做法**：让它写脚本，你跑脚本。

prompt（总纲第 5 周模板的具象化）：

```text
我要推导弹簧能量 E(x,y)=Σ_i ½·k_i·(√((x-ax_i)²+(y-ay_i)²) - L_i)² 关于 (x,y) 的梯度。
请生成一段 SymPy 脚本：
1. 声明符号 x,y；锚点/L/k 作为已知常数数组
2. 写出 E 的 SymPy 表达式
3. 求 grad = [E.diff(x), E.diff(y)]，simplify()
4. cse(grad) 提公共子表达式
5. sp.ccode() 输出 C++，函数签名：void compute_gradient(const double* p, double* g)
6. 脚本末尾用 sp.lambdify 对几个点求值，与手动有限差分对比自检
不要直接给我手推公式——我要脚本，脚本跑出来的结果我才信。
```

脚本骨架（AI 应产出类似物，你本机 `pip install sympy` 跑一遍）：

```python
import sympy as sp

x, y = sp.symbols('x y', real=True)
AX = [0.0, 4.0, 2.0]; AY = [0.0, 0.0, 3.0]
REST = [2.0, 2.0, 2.0]; STIF = [1.0, 1.5, 0.8]

E = 0
for ax, ay, L, k in zip(AX, AY, REST, STIF):
    d = sp.sqrt((x - ax)**2 + (y - ay)**2)
    E += sp.Rational(1, 2) * k * (d - L)**2

grad = [sp.simplify(sp.diff(E, v)) for v in (x, y)]
common, reduced = sp.cse(grad)
print("void compute_gradient(const double* p, double* g) {")
print("    const double x = p[0], y = p[1];")
for name, expr in common:
    print(f"    const double {name} = {sp.ccode(expr)};")
for i, expr in enumerate(reduced):
    print(f"    g[{i}] = {sp.ccode(expr)};")
print("}")

# —— 自检：与有限差分对比（脚本里先跑一遍，harness 再跑一遍，双保险）
f = sp.lambdify((x, y), E, 'math')
import math
for px, py in [(1.5, 1.0), (3.0, 2.0)]:
    h = 1e-6
    gx = (f(px + h, py) - f(px - h, py)) / (2 * h)
    print("fd check gx:", gx)
```

把 `print` 出来的 `compute_gradient` 粘进 `test_gradient.cpp` 的 PASTE ZONE，编译运行。
**Test 1** 与中心差分双算：正确实现相对误差 ~1e-10；漏负号则爆到 O(1)。
**Test 2** 沿 −grad 走能量必须下降（抓全局符号翻转）。
**Test 3** 观察题：把点几乎压到锚点上，1/d 发散——SymPy 式子里的奇异，AI 声明了吗？

### 周四-周日：Hessian 生成

同样让 AI 写脚本，只是把 `diff` 换成 `sp.hessian`：

```python
H = sp.hessian(E, (x, y))            # 2×2 对称
common, reduced = sp.cse([H[0,0], H[0,1], H[1,1]])   # 只需上三角
# ccode 输出 compute_hessian，行主序 H_out[0..3]，末尾 H_out[2]=H_out[1]
```

粘进 `test_hessian.cpp`。金标准是**对已验证的解析梯度再做一次中心差分**
（`fd_hessian_from_grad`，比二阶差分能量稳得多）+ **对称性 Hxy==Hyx**（解析 Hessian 必须严格对称）。
`compute_gradient` 在这里是"给定基础设施"（周一三已验证过），把它签名贴进 prompt 让脚本只管 Hessian。

**Test 3 观察题**给你一组点的 det/trace：会看到有的点 SPD（局部凸）、有的点非正定。
牛顿法在非正定处怎么办（加阻尼 / 特征值截断）？**这是 AI 给不了的工程决策。**

### 周末：沉淀模板 + 失败日志

把这周的套路抽进 [`prompt_library/`](../prompt_library/)（建议 `06_math/sympy_gradient.md`）：
"声明符号 → 写能量 → diff/hessian → cse → ccode → lambdify 自检"。
更新 `failure_log.md`：记 AI 在符号推导上翻车的样本（十有八九是漏负号或漏 1/d）。

---

## 这套流程的"杠杆"在哪里

1. **推导时间断崖式下降**：手推 Hessian 半小时 + 反复查负号，变成"写脚本 30 秒 + 机器跑"。
2. **AI 出错也拦得住**：有限差分是外部裁判，SymPy 漏项当场现形，你不需要"相信"AI。
3. **奇异/正定性是你的地盘**：脚本给公式，但 1/d 奇异要不要保护、非正定 Hessian 怎么修——AI 交卷，你判分。

---

## 常见踩坑

| 现象 | 原因 / 应对 |
|---|---|
| gradient Test 1 相对误差 ~O(1) | 漏负号 / 漏 1/d / 链式求导错。让 AI **重跑脚本**，别手改 C++ |
| gradient Test 2 FAIL 但 Test 1 也 FAIL | 梯度整体符号反了（可能把 −grad 当成 grad 输出） |
| hessian Test 1 过、Test 2 不过 | 只填了上三角没做 `H_out[2]=H_out[1]`，或 Hxy/Hyx 算了两个不同式 |
| 靠近锚点结果爆炸 / NaN | 1/d 奇异没保护。真实项目里对 d 加 epsilon 或在 prompt 里先声明奇异清单 |
| SymPy 跑不出来 / 巨慢 | `simplify()` 对大表达式会卡；本周规模无碍，第 6 周会改用 `cse()` 直出，不 simplify |
| 生成的 C++ 编译报错 | ccode 可能用了 `pow(x,2)`——无碍；或漏了 `#include <cmath>`（harness 已 include） |

---

## 最后一句

> 第 1-4 周你学会"让 AI 写对代码，再用测试打脸"。
> 这一周把同样的纪律用到**数学推导**上：
> **让 AI 写 SymPy 脚本（推得快），让中心差分当裁判（信得过），
> 而奇异保护和正定性修复这些容差决策，永远留给你。**
> SymPy 会算，有限差分会验，但"这个 1/d 要不要保护"——机器不会替你拍板。
