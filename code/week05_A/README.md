# week05_A — SymPy 梯度 × C++ 有限差分验证（CMake 工程）

第 5 周的"粘贴即验证"工程，配套 [docs/week05_实操_SymPy推导梯度.md](../../docs/week05_实操_SymPy推导梯度.md)。
本周把 AI 当**符号推导的搭档**，但**坚持第 1 周的 C++ 数值金标准**：
SymPy 只当"参考生成器"，真正粘进 PASTE ZONE 的是它 `ccode()` 吐出的 C++，
再用手写**中心差分**双算对照。

> 🔧 **构建方法与 week01_A 完全相同**（零依赖，无需 Eigen / 也不需要在本机装 Python）：
> 见 [week01_A/README.md](../week01_A/README.md)。
> 快速开始：`cmake -S . -B build && cmake --build build`。

## 为什么"让 AI 写脚本"而不是"让 AI 手推公式"

AI 自己手推偏导 70% 会在符号上漏个负号或漏个 1/d。所以本周的铁律（总纲原话）：
**永远让 AI 写 SymPy 脚本，永远跑一遍验证**。脚本跑出来的结果才信。
而"跑一遍验证"落到工程上，就是这个 harness：把生成的 C++ 和有限差分对撞。

## 能量模型（写在 `energy.h`，不要改）

一个自由点 p=(x,y) 被 3 根弹簧连到 3 个固定锚点：

```text
E(p) = Σ_i ½·k_i·(‖p − a_i‖ − L_i)²
```

变量只有 2 个 → 有限差分廉价、参考可信；但含 sqrt + 链式求导 → 最能暴露符号错误。
（第 6 周的 CSE / 稀疏组装会复用这同一个能量。）

## 本周内容

| 文件 | 被测接口（PASTE ZONE） | 验证金标准 |
|---|---|---|
| `energy.h` | 公共头（不要改）：`energy` + `fd_gradient` / `fd_hessian_from_grad` + 能量场 SVG | — |
| `test_gradient.cpp` | `compute_gradient`（SymPy 生成，粘入） | 中心差分梯度，max 相对误差 < 1e-6；专抓漏负号 |
| `test_hessian.cpp` | `compute_hessian`（SymPy 生成，粘入） | 对已验证解析梯度再差分 + 对称性 Hxy==Hyx |

> 📌 初始 PASTE ZONE 是占位桩（返回全 0），测试全 FAIL 属预期。

## SymPy 脚本骨架（实操文档里有完整版）

```python
import sympy as sp
x, y = sp.symbols('x y')
# 写出 E(x,y)（锚点/L/k 为常数）→ 求 grad = [E.diff(x), E.diff(y)]
# → cse(grad) 提公共子表达式 → sp.ccode(...) 生成 compute_gradient
# → 末尾用 sp.lambdify 对几个点跑一遍，与手动有限差分自检
```

**要的是脚本，不是手推结果**——脚本可复现、可回归，手推不可信。

## 快速开始（命令行）

```bash
cmake -S . -B build
cmake --build build
./build/bin/test_gradient      # 生成 gradient_output.svg（能量热力图 + 负梯度箭头）
./build/bin/test_hessian
```

## 编辑约定

- **不要修改 `energy.h`** 和各 `int main()`：它们是有限差分金标准。
- **只在 PASTE ZONE 内**替换 `compute_gradient` / `compute_hessian`。
- `test_hessian.cpp` 里的 `grad_ref` 是给定基础设施（已验证的解析梯度），供参考 Hessian 用，别动。
