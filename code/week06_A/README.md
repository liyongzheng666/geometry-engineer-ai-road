# week06_A — 符号 → 工业 C++：CSE 等价 + Eigen 稀疏组装（CMake 工程）

第 6 周的"粘贴即验证"工程，配套 [docs/week06_实操_符号转CPP.md](../../docs/week06_实操_符号转CPP.md)。
把第 5 周 SymPy 推出的表达式，落成**项目里能编译、又快又对**的 C++：
一半功夫在 `cse()` 自动公共子表达式，一半在 Eigen 稀疏矩阵组装。

> 🔧 **构建方法同 week01_A**；`test_sparse_assembly` 依赖 **Eigen**，处理方式同 week03_A
> （本机有 Eigen 直接用，没有则 CMake 自动 FetchContent 拉 3.4.0，首次约 30s 联网）。
> 快速开始：`cmake -S . -B build && cmake --build build`。

## 本周两条主线

| 文件 | 被测接口（PASTE ZONE） | 验证金标准 | 依赖 |
|---|---|---|---|
| `codegen_ref.h` | 公共头（不要改）：`hessian_ref` + 计时器 + 扫描曲线 SVG | — | 无 |
| `test_cse_equiv.cpp` | `hessian_naive` / `hessian_cse`（SymPy ccode 两版） | ① 两版结果一致 <1e-12 ② 都对参考 <1e-9 ③ cse 更快 | 无 |
| `test_sparse_assembly.cpp` | `assemble_sparse`（Triplet+setFromTriplets） | 与稠密参考逐元一致 <1e-12 + 全局对称 | **Eigen** |

> 📌 初始 PASTE ZONE 是占位桩，测试全 FAIL 属预期。

## 主线一：CSE 是"结果不变、只减重复计算"

第 5 周的重要修正：**别让 AI"手动提公共子表达式"，SymPy 自带 `cse()`**。

```python
from sympy import cse, ccode
common, reduced = cse([H[0,0], H[0,1], H[1,1]])
for name, expr in common:  print(f"const double {name} = {ccode(expr)};")
for expr in reduced:       print(ccode(expr))
```

`test_cse_equiv` 把 "ccode(H)（不 cse）" 和 "ccode(cse(H))" 两版都粘进来对撞：
**Test 2 要求两版逐点一致到 1e-12**——CSE 只是把 `sqrt(...)` 这类子式提出来算一次，
绝不能改变数值。**Test 3 计时**：cse 版每个 `d_i` 只算一次，naive 版每个输出项重算，
加速主要来自省掉的超越函数。SVG 把两版 Hxx(x) 画成蓝粗+橙细两条线，重合则叠成一条。

## 主线二：稀疏组装的"只此一次 setFromTriplets"

`test_sparse_assembly` 把逐边的局部 4×4 Hessian 组装成全局稀疏矩阵。局部块
`local_hessian` 是给定基础设施，你只写 scatter 外壳，大纲要点全在 PASTE ZONE 注释里：

- 用 `Eigen::Triplet` 收集所有非零，**最后一次 `setFromTriplets`**（自动对共享 dof 求和）；
- **不要用 `insert()`** 逐个塞——多次 insert 触发多次列重排，大规模下慢一到两个量级（Test 3 点破）。

金标准是与"稠密直接组装"逐元对照（走不同代码路径）+ 全局对称。`sparse_pattern_output.svg`
画出稀疏图，应呈块三对角。

## 快速开始（命令行）

```bash
cmake -S . -B build
cmake --build build
./build/bin/test_cse_equiv         # 生成 cse_equiv_output.svg
./build/bin/test_sparse_assembly   # 生成 sparse_pattern_output.svg
```

## 编辑约定

- **不要改 `codegen_ref.h`** 和各 `int main()`：它们是金标准。
- **只在 PASTE ZONE 内**替换函数；`test_sparse_assembly` 里 `sp_::local_hessian` /
  `dense_assemble` 是给定基础设施，把签名贴进 prompt 让 AI 只写 `assemble_sparse`。
