# 第 3 周补充：Eigen 集成与 CMake 依赖管理

> 本周 `test_plane_fit` 是整个课程第一个（也是前三周唯一一个）第三方依赖。
> 这篇讲两件事：CMake 怎么优雅地引入 Eigen，以及 Eigen 用在几何代码里的
> 几个关键决策点。

## 一、find_package + FetchContent 兜底：依赖管理的双保险

week03_A 的 CMakeLists 核心段：

```cmake
find_package(Eigen3 QUIET CONFIG)
if(NOT TARGET Eigen3::Eigen)
    include(FetchContent)
    set(BUILD_TESTING         OFF CACHE BOOL "" FORCE)
    set(EIGEN_BUILD_DOC       OFF CACHE BOOL "" FORCE)
    set(EIGEN_BUILD_TESTING   OFF CACHE BOOL "" FORCE)
    set(EIGEN_BUILD_PKGCONFIG OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(eigen
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG        3.4.0
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(eigen)
endif()
...
target_link_libraries(test_plane_fit PRIVATE Eigen3::Eigen)
```

逐个决策点拆开：

| 写法 | 为什么 |
|---|---|
| `find_package(... QUIET CONFIG)` | 优先用本机安装（brew/vcpkg/apt），秒级配置；`CONFIG` 限定走包自带的配置文件，避免老式 Find 模块行为不一 |
| `if(NOT TARGET Eigen3::Eigen)` | 判断**目标存在与否**比判断 `Eigen3_FOUND` 更稳——target 才是后面真正要用的东西 |
| 不写最低版本号 | 写 `find_package(Eigen3 3.4)` 会把 brew 的 Eigen 5.x 拒之门外（版本兼容声明按主版本匹配），明明能用却去联网下载 |
| `BUILD_TESTING OFF` 等四连关 | Eigen 仓库自带几百个测试 target，不关的话 `cmake --build` 会编到天荒地老——我们只要头文件 |
| `GIT_SHALLOW TRUE` | 浅克隆，下载量从几百 MB 降到几十 MB |
| `target_link_libraries(test_plane_fit ...)` | 只给需要的 target；test_circumcircle 保持零依赖，互不拖累 |

效果：有本机 Eigen 零等待，没有则首次 configure 联网 ~30 秒，之后离线可用。
**这个模式（find_package → FetchContent 兜底）适用于一切 header-only 库**，
值得进你的 prompt_library。

## 二、Eigen::Map：零拷贝接入裸内存

工业点云永远是别人给的 `const double*`。错误姿势是先复制：

```cpp
std::vector<Eigen::Vector3d> pts;          // ❌ N 次构造 + 双倍内存
for (int i = 0; i < n; ++i) pts.emplace_back(p[3*i], p[3*i+1], p[3*i+2]);
```

正确姿势是把已有内存"贴上 Eigen 的壳"：

```cpp
using MatX3 = Eigen::Matrix<double, Eigen::Dynamic, 3, Eigen::RowMajor>;
Eigen::Map<const MatX3> P(points, n, 3);   // ✅ 零拷贝，只是个视图
```

两个易错点：

- **RowMajor 必须显式写**。Eigen 默认列主序，而 C 数组按行排——
  漏写的话数据全错位，且能"正常"跑完（这正是第 1 周说的
  "看起来对、跑起来 90% 对"型 bug）；
- `Map<const ...>` 的 const 写在矩阵类型上，不是 Map 外面。

## 三、JacobiSVD vs SelfAdjointEigenSolver

平面拟合要的是"协方差矩阵最小特征方向"，两条路都通：

| | JacobiSVD | SelfAdjointEigenSolver |
|---|---|---|
| 适用 | 任意矩阵 | 对称（自伴）矩阵 |
| 协方差（对称半正定） | 可用，σ = 特征值 | **天然匹配**，更快 |
| 排序 | 奇异值降序 | 特征值**升序**（最小方向 = `.col(0)`） |
| API 稳定性 | 3.4 → 5.x 间构造选项写法有变动（deprecation） | 几个大版本未变 |

大纲原文写 JacobiSVD（教学上 SVD 概念更通用），实现上对 3×3 对称矩阵
`SelfAdjointEigenSolver` 是更对口的工具。**两者都接受**——harness 只验数值结果，
不审实现路径；但你应该能向 AI 解释清楚为什么二选一。

## 四、性能三定律（本周 Test 7 的理论依据）

1. **先降维再分解**：协方差是 3×3，无论 N 多大。对 N×3 做 SVD 是
   O(N) 内存 + 大常数，对 3×3 是 O(1)；
2. **警惕显式临时**：`(P.rowwise() - c.transpose())` 写进表达式里是
   惰性求值，赋给 `MatrixXd` 变量就物化成 N×3 临时——一词之差，内存翻倍；
3. **逐行累加最稳**：`for 每行 { d = row - c; cov += d * d.transpose(); }`
   ——可读、零临时、编译器好优化。20 万点 ≈ 0.3ms。

## 五、版本兼容备注

- 本仓库 FetchContent 锁定 **Eigen 3.4.0**（最后一个稳定大版本）；
- 本机若装了 Eigen 5.x（如 brew 最新），find_package 也能用——
  但 `JacobiSVD<M> svd(m, ComputeFullV)` 这种**运行时选项构造**在新版会出
  deprecation 警告（新写法是模板参数）。看到这种警告不用慌，
  也正好是给 AI 的一道真实考题：让它解释 deprecation 并给出两个版本都能编的写法。
- MSVC `/W4` 对 Eigen 头文件会刷少量警告，属已知现象，可忽略。
