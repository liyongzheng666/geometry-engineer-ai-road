# week04_A — 保特征网格平滑（CMake 工程）

第 4 周的"粘贴即验证"工程，配套 [docs/week04_实操_保特征网格平滑.md](../../docs/week04_实操_保特征网格平滑.md)。
本周是**阶段一的合体项目**：把前三周的技能（公式化 prompt / 退化清单 / 上下文注入）
用在一个完整算子——**基于法向的双边网格平滑**上。

> 🔧 **构建方法与 week01_A 完全相同**（零依赖，无需 Eigen）：见 [week01_A/README.md](../week01_A/README.md)。
> 快速开始：`cmake -S . -B build && cmake --build build`，然后运行 `build/bin/` 下三个 test。

## 关键设计：为什么用"高度场三角网"

为保持第 1 周的"零依赖 + SVG 可视化 + 可构造金标准"，`mesh.h` 用**规则栅格高度场三角网**
（顶点 (i,j) 在 (i·dx, j·dy, z_ij)，每个栅格剖成两个三角形）而非通用半边结构：

- 它是**真实三角网**：有 one-ring 邻域、面法向、开放边界——保特征平滑该考虑的都在；
- 能**俯视渲染成 SVG 热力图**（按 z 染色），一眼看出"噪声抹平但折痕仍锐"；
- 能**由答案构造输入**：已知折痕角度 → 加固定种子噪声 → 平滑后回测，金标准无可辩驳。

在你的真实项目里，这一步对应把半边结构头文件注入上下文（第 3 周技巧），接口语义一致。

## 本周内容

| 文件 | 角色 |
|---|---|
| `mesh.h` | **公共头（不要改）**：V3 向量代数 + 栅格高度场 Mesh（邻域/面法向/边界）+ 俯视热力图 SVG |
| `test_vertex_normal.cpp` | 被测接口 `vertex_normal`——面积加权顶点法向（含零面积三角形处理） |
| `test_bilateral_weight.cpp` | 被测接口 `bilateral_weight`——空间核 × range 核 |
| `test_mesh_smooth.cpp` | 被测接口 `smooth_step`——一次保特征更新；`vertex_normal`/`bilateral_weight` 在此为**给定基础设施** |

> 📌 初始状态下三个 PASTE ZONE 都是占位桩，测试全 FAIL 属预期。粘入正确实现后转 PASS。

## 三个金标准一句话

1. **vertex_normal**：倾斜平面上顶点法向严格等于解析法向 `normalize(-a,-b,1)`（tol 1e-12，无离散化误差）。
2. **bilateral_weight**：与"两个独立一维高斯之积"双算对照 + 非负/单调衰减/偶对称。
3. **smooth_step**：屋脊网格加噪后平滑——① 平坦区 RMS 明显下降（去噪，~40%+）② 折痕不下沉（保特征，区分双边 vs 拉普拉斯）③ 边界不漂移。

## 快速开始（命令行）

```bash
cmake -S . -B build
cmake --build build
./build/bin/test_vertex_normal
./build/bin/test_bilateral_weight
./build/bin/test_mesh_smooth      # 生成 mesh_smooth_output.svg（左噪声 / 右平滑）
```

## 编辑约定

- **不要修改 `mesh.h`**：整个第 4 周保持不变。
- **不要修改 `int main()` 和参考实现**：它们是客观判定的金标准。
- **只在 PASTE ZONE 内替换函数实现**；`test_mesh_smooth.cpp` 里 `mesh_vertex_normal` /
  `bilateral_weight` 是给定基础设施，把签名贴进 prompt 让 AI 只写 `smooth_step` 外壳。
