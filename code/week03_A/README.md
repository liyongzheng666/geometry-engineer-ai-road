# week03_A — 上下文注入（CMake 工程）

第 3 周的"粘贴即验证"工程，配套 [docs/week03_实操_上下文注入.md](../../docs/week03_实操_上下文注入.md)。
本周考点不是算法难度，而是**让 AI 服从你注入的项目风格**。

> 🔧 **构建方法与 week01_A 相同**：见 [week01_A/README.md](../week01_A/README.md)。
> ⚠️ 唯一区别：`test_plane_fit` 依赖 **Eigen**。CMake 会优先用本机安装的 Eigen
> （mac: `brew install eigen`；Windows: vcpkg/手动均可）；找不到则自动 FetchContent
> 在线拉取 Eigen 3.4.0——**首次 configure 需联网约 30 秒**，之后离线可用。
> MSVC `/W4` 对 Eigen 头文件可能刷少量警告，可忽略。

## 本周内容

| 文件 | 角色 |
|---|---|
| `mini_types.h` | **练习材料**（不要改）：模拟"公司项目核心头文件"的 `gx::` 类型集——成员函数风格、错误码、统一容差 `kEpsGeom`，风格刻意与 `geo.h` 相反 |
| `test_circumcircle.cpp` | 被测接口 `circumcircle`（三点定圆）——PASTE ZONE 限定**只用 gx:: 类型**，考察风格注入是否生效 |
| `test_plane_fit.cpp` | 被测接口 `plane_fit`（最小二乘平面拟合）——按大纲约束用 **Eigen**（Map 零拷贝、3×3 协方差、SVD/特征分解） |

## 风格判分三条（test_circumcircle 跑完会再提醒）

1. PASTE ZONE 里有没有出现 STL 容器？
2. 是不是用 `.add()/.sub()` 而非运算符？
3. 错误路径返回的 `GxStatus` 语义对不对？容差用的是 `gx::kEpsGeom` 吗？

> 📌 初始状态下 PASTE ZONE 是占位桩，测试全 FAIL 属预期。
> `test_plane_fit` 的 Test 7 是性能观察题（N=200000 计时）：
> 正确实现 < 1ms 量级，反复拷贝的实现会到数百毫秒。

## 快速开始（命令行）

```bash
cmake -S . -B build
cmake --build build
./build/bin/test_circumcircle
./build/bin/test_plane_fit
```
