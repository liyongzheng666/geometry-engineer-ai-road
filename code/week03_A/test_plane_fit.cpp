// test_plane_fit.cpp — 最小二乘平面拟合（Eigen）的验证闭环
// 用法：把 AI 生成的 plane_fit 函数粘到 PASTE ZONE 之间，重新编译即可
//
// 本周唯一允许 Eigen 的文件。给 AI 的 prompt 约束（大纲原文）：
//   1. 输入是 const double*（row-major，N×3），必须用 Eigen::Map 零拷贝接入
//   2. 对 3×3 协方差矩阵做 JacobiSVD（不是对 N×3！），
//      取最小奇异值对应的右奇异向量作为法向
//   3. N < 3 → 返回 false；共线 → 检测 σ_min/σ_mid 比值 → 返回 false
//   4. 避免 N×3 的显式临时矩阵（Test 7 的计时会暴露反复拷贝）

#include "geo.h"

#include <Eigen/Dense>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <vector>

// =================================================================
// >>>>>>>>>>  PASTE AI-GENERATED plane_fit BELOW  <<<<<<<<<<
// =================================================================

// 输出：centroid（平面上一点）+ normal（单位法向）
// 返回：true = 成功；false = 退化（N<3、共线、数值失败），此时不得写入 NaN
bool plane_fit(const double* points, int n,
               Eigen::Vector3d& centroid, Eigen::Vector3d& normal) {
    // TODO: 在这里粘贴 / 实现最小二乘平面拟合
    // 必含要素（建议连同文件头的 4 条约束一起写进你的 prompt）：
    //   1. Eigen::Map<const Matrix<double,Dynamic,3,RowMajor>> 零拷贝接入
    //   2. centroid = 列均值；协方差 = Σ (p_i-c)(p_i-c)^T（3×3，逐行累加）
    //   3. JacobiSVD / SelfAdjointEigenSolver 取最小奇异值（特征值）方向作法向
    //   4. N<3、共线（σ_min/σ_mid 比值）→ return false，不得写入 NaN
    (void)points; (void)n; (void)centroid; (void)normal;
    return false;  // 占位：未实现
}

// =================================================================
// >>>>>>>>>>             END OF PASTE ZONE              <<<<<<<<<<
// =================================================================

// ---------- 基础设施：真值平面与数据生成（使用者不动） ----------

// 真值平面：过 Q，单位法向 n0；e1/e2 是平面内单位正交基（设计时已验证正交归一）
static const double QX = 1, QY = 2, QZ = 3;
static const double N0X = 1.0 / 3, N0Y = 2.0 / 3, N0Z = 2.0 / 3;
static const double E1X = 0,                  E1Y = 1 / std::sqrt(2.0),  E1Z = -1 / std::sqrt(2.0);
static const double E2X = -4 / (3 * std::sqrt(2.0)), E2Y = 1 / (3 * std::sqrt(2.0)), E2Z = 1 / (3 * std::sqrt(2.0));

// p = Q + s·e1 + t·e2 + h·n0（h 是沿法向的噪声）
static void append_plane_point(std::vector<double>& out, double s, double t, double h) {
    out.push_back(QX + s * E1X + t * E2X + h * N0X);
    out.push_back(QY + s * E1Y + t * E2Y + h * N0Y);
    out.push_back(QZ + s * E1Z + t * E2Z + h * N0Z);
}

// 独立验证路径：手写 Cramer 法解 z = ax + by + c 的正规方程（完全不用 Eigen）。
// 注意：z-残差最小二乘 ≠ 正交最小二乘，仅在【精确共面】数据上两者法向严格一致，
// 所以只用于 Test 1 的对照。
static bool zfit_ref(const double* p, int n, double& nx, double& ny, double& nz) {
    double sxx = 0, sxy = 0, sx = 0, syy = 0, sy = 0, sxz = 0, syz = 0, sz = 0;
    for (int i = 0; i < n; ++i) {
        const double x = p[3 * i], y = p[3 * i + 1], z = p[3 * i + 2];
        sxx += x * x; sxy += x * y; sx += x;
        syy += y * y; sy += y;
        sxz += x * z; syz += y * z; sz += z;
    }
    const double N = n;
    auto det3 = [](double a11, double a12, double a13,
                   double a21, double a22, double a23,
                   double a31, double a32, double a33) {
        return a11 * (a22 * a33 - a23 * a32)
             - a12 * (a21 * a33 - a23 * a31)
             + a13 * (a21 * a32 - a22 * a31);
    };
    const double D = det3(sxx, sxy, sx, sxy, syy, sy, sx, sy, N);
    if (std::abs(D) < 1e-12) return false;
    const double a = det3(sxz, sxy, sx, syz, syy, sy, sz, sy, N) / D;
    const double b = det3(sxx, sxz, sx, sxy, syz, sy, sx, sz, N) / D;
    const double len = std::sqrt(a * a + b * b + 1.0);
    nx = a / len; ny = b / len; nz = -1.0 / len;
    return true;
}

static double max_abs_residual(const double* p, int n,
                               const Eigen::Vector3d& centroid,
                               const Eigen::Vector3d& normal) {
    double m = 0;
    for (int i = 0; i < n; ++i) {
        const double r = (p[3 * i]     - centroid.x()) * normal.x()
                       + (p[3 * i + 1] - centroid.y()) * normal.y()
                       + (p[3 * i + 2] - centroid.z()) * normal.z();
        m = std::max(m, std::abs(r));
    }
    return m;
}

int main() {
    enable_utf8_console();

    bool all_pass = true;
    auto check = [&](const std::string& name, double got, double expect,
                     double tol) {
        const bool ok = std::abs(got - expect) < tol;
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << name
                  << "  got=" << got << "  expect=" << expect
                  << "  err=" << std::abs(got - expect) << "\n";
        if (!ok) all_pass = false;
    };
    auto check_true = [&](const std::string& name, bool ok) {
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << name << "\n";
        if (!ok) all_pass = false;
    };

    const Eigen::Vector3d n0(N0X, N0Y, N0Z);
    const Eigen::Vector3d Q(QX, QY, QZ);

    // 留给 SVG
    std::vector<double> exact_pts, noisy_pts;
    Eigen::Vector3d exact_centroid = Eigen::Vector3d::Zero();
    Eigen::Vector3d exact_normal  = Eigen::Vector3d::Zero();
    Eigen::Vector3d noisy_centroid = Eigen::Vector3d::Zero();
    Eigen::Vector3d noisy_normal  = Eigen::Vector3d::Zero();
    bool noisy_ok = false;

    std::cout << "=== Test 1: 精确共面恢复（4×3 网格，零噪声，N=12） ===\n";
    {
        for (double s : {-1.5, -0.5, 0.5, 1.5})
            for (double t : {-1.0, 0.0, 1.0})
                append_plane_point(exact_pts, s, t, 0.0);
        const bool ok = plane_fit(exact_pts.data(), 12, exact_centroid, exact_normal);
        check_true("返回 true", ok);
        // 法向允许反号：|normal·n0| 应为 1
        check("1-|normal·n0|", 1.0 - std::abs(exact_normal.dot(n0)), 0.0, 1e-9);
        // centroid 必须落在真值平面上
        check("|(centroid-Q)·n0|", std::abs((exact_centroid - Q).dot(n0)), 0.0, 1e-9);
        check("max 残差", max_abs_residual(exact_pts.data(), 12, exact_centroid, exact_normal),
              0.0, 1e-9);
        // 双算对照：精确共面数据上 z-拟合参考的法向应与被测一致
        double rx, ry, rz;
        if (zfit_ref(exact_pts.data(), 12, rx, ry, rz)) {
            const double agree = std::abs(exact_normal.x() * rx + exact_normal.y() * ry
                                        + exact_normal.z() * rz);
            check("vs zfit_ref |dot|", agree, 1.0, 1e-9);
        }
    }

    std::cout << "\n=== Test 2: 法向单位长度 ===\n";
    check("|normal|", exact_normal.norm(), 1.0, 1e-12);

    std::cout << "\n=== Test 3: 加噪容差（N=60，法向噪声 ±0.01，种子 42） ===\n";
    {
        std::mt19937 rng(42);
        std::uniform_real_distribution<double> st(-2.0, 2.0), noise(-0.01, 0.01);
        for (int i = 0; i < 60; ++i)
            append_plane_point(noisy_pts, st(rng), st(rng), noise(rng));
        noisy_ok = plane_fit(noisy_pts.data(), 60, noisy_centroid, noisy_normal);
        check_true("返回 true", noisy_ok);
        const double angle = std::acos(std::min(1.0, std::abs(noisy_normal.dot(n0))));
        check("法向角偏差 (rad)", angle, 0.0, 0.01);
        check("|(centroid-Q)·n0|", std::abs((noisy_centroid - Q).dot(n0)), 0.0, 0.005);
    }

    std::cout << "\n=== Test 4: N < 3 拒绝 ===\n";
    {
        const double dummy[6] = {0, 0, 0, 1, 1, 1};
        Eigen::Vector3d c = Eigen::Vector3d::Zero(), nv = Eigen::Vector3d::Zero();
        check_true("N=0 → false", !plane_fit(dummy, 0, c, nv));
        check_true("N=1 → false", !plane_fit(dummy, 1, c, nv));
        check_true("N=2 → false", !plane_fit(dummy, 2, c, nv));
        check_true("失败时输出无 NaN", c.allFinite() && nv.allFinite());
    }

    std::cout << "\n=== Test 5: 共线拒绝（10 点严格落在一条直线上） ===\n";
    {
        std::vector<double> line;
        for (int t = 0; t < 10; ++t) {
            line.push_back(t); line.push_back(2.0 * t); line.push_back(-1.0 * t);
        }
        Eigen::Vector3d c = Eigen::Vector3d::Zero(), nv = Eigen::Vector3d::Zero();
        check_true("共线 → false", !plane_fit(line.data(), 10, c, nv));
    }

    std::cout << "\n=== Test 6: 近退化（观察题：阈值哲学） ===\n";
    {
        // 共线点 + 垂直方向 1e-7 的微小摆动："几乎是线"的输入
        std::vector<double> wiggle;
        const double wx = 2.0 / std::sqrt(5.0), wy = -1.0 / std::sqrt(5.0);  // ⊥ 线方向
        for (int t = 0; t < 10; ++t) {
            const double w = ((t % 2 == 0) ? 1.0 : -1.0) * 1e-7;
            wiggle.push_back(t + w * wx);
            wiggle.push_back(2.0 * t + w * wy);
            wiggle.push_back(-1.0 * t);
        }
        Eigen::Vector3d c = Eigen::Vector3d::Zero(), nv = Eigen::Vector3d::Zero();
        const bool ok = plane_fit(wiggle.data(), 10, c, nv);
        std::cout << "  共线 + 1e-7 横向摆动：返回 " << (ok ? "true" : "false");
        if (ok) std::cout << "  normal=(" << nv.x() << "," << nv.y() << "," << nv.z() << ")";
        std::cout << "\n  问题：AI 的 σ_min/σ_mid 阈值取了多少？这种输入它接受还是拒绝？\n";
        std::cout << "        拒绝阈值该由谁定——AI 还是你的 prompt？\n";
    }

    std::cout << "\n=== Test 7: 性能与代码审查（观察题，N=200000） ===\n";
    {
        std::vector<double> big;
        big.reserve(600000);
        for (int i = 0; i < 200000; ++i)
            append_plane_point(big, (i % 500) * 0.01, (i / 500) * 0.01, 0.0);
        Eigen::Vector3d c = Eigen::Vector3d::Zero(), nv = Eigen::Vector3d::Zero();
        const auto t0 = std::chrono::steady_clock::now();
        const bool ok = plane_fit(big.data(), 200000, c, nv);
        const auto t1 = std::chrono::steady_clock::now();
        const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        std::cout << "  N=200000：返回 " << (ok ? "true" : "false")
                  << "，耗时 " << ms << " ms\n";
        std::cout << "  肉眼 review PASTE ZONE：有没有 N×3 的显式临时矩阵\n";
        std::cout << "  （如 MatrixXd centered = ...）？是不是对 3×3 而非 N×3 做分解？\n";
        std::cout << "  耗时若达数百毫秒，多半在反复拷贝。\n";
    }

    // -------- SVG：侧视残差图（有向残差 ×20 放大） --------
    const std::string svg_name = "plane_fit_output.svg";
    {
        SVG svg(svg_name, -4.5, -0.6, 4.5, 0.6);
        svg.line(-4, 0, 4, 0, "black", 0.01);   // 拟合平面 = 零轴
        const Eigen::Vector3d e1(E1X, E1Y, E1Z);
        auto draw_residuals = [&](const std::vector<double>& pts,
                                  const Eigen::Vector3d& c, const Eigen::Vector3d& nv,
                                  const char* color, double r) {
            for (size_t i = 0; i + 2 < pts.size(); i += 3) {
                const Eigen::Vector3d p(pts[i], pts[i + 1], pts[i + 2]);
                const double s = (p - c).dot(e1);
                const double h = (p - c).dot(nv) * 20.0;   // 残差放大 20 倍
                svg.dots({{s, h, 0}}, color, r);
            }
        };
        draw_residuals(exact_pts, exact_centroid, exact_normal, "#4daf4a", 0.05);
        if (noisy_ok) draw_residuals(noisy_pts, noisy_centroid, noisy_normal, "#377eb8", 0.04);
    }

    const auto abs_path = std::filesystem::absolute(svg_name);
    std::cout << "\n生成: " << abs_path.string() << "\n";
    std::cout << "可视判断（残差已 ×20 放大）：\n";
    std::cout << "  绿色（精确数据）应全部压在零轴上——不压线 = 拟合有系统偏差；\n";
    std::cout << "  蓝色（加噪数据）应均匀骑跨零轴 ±0.2 以内——整体偏一侧 = 质心或法向错。\n";
    std::cout << (all_pass ? "\nAll numeric checks passed.\n"
                           : "\nSome checks failed. 检查 AI 代码。\n");
    return all_pass ? 0 : 1;
}
