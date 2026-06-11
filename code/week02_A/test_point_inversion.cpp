// test_point_inversion.cpp — 点到 B 样条曲线最近点（point inversion）的验证闭环
// 用法：把 AI 生成的 point_inversion 函数粘到 PASTE ZONE 之间，重新编译即可
//
// 基础设施（使用者不要改，但 PASTE ZONE 内允许调用）：
//   curve_eval(u)   —— 曲线求值 C(u)
//   curve_deriv(u)  —— 一阶导 C'(u)
//   curve_deriv2(u) —— 二阶导 C''(u)
//   dot3(a, b)      —— 三维点积
// 实操建议：把这四个函数的【声明】原样贴进你的 prompt，作为 AI 可用的接口，
// 让 AI 聚焦于 Newton-Raphson 迭代本身。

#include "geo.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

// =================================================================
// 基础设施：B 样条基函数 / 导数 / 曲线求值（使用者不动）
// =================================================================

static double bspline_basis(int i, int p, double u, const std::vector<double>& knots) {
    if (p == 0) {
        // 标准半开区间 [u_i, u_{i+1})
        if (u >= knots[i] && u < knots[i + 1]) return 1.0;
        // corner case：u = u_max 时闭合最后一个非空 span（与 week01_A 一致）
        if (u == knots.back() && knots[i] < knots[i + 1] && knots[i + 1] == u)
            return 1.0;
        return 0.0;
    }
    const double d1 = knots[i + p]     - knots[i];
    const double d2 = knots[i + p + 1] - knots[i + 1];
    const double a = (d1 > 0.0)
        ? (u - knots[i]) / d1 * bspline_basis(i,     p - 1, u, knots) : 0.0;
    const double b = (d2 > 0.0)
        ? (knots[i + p + 1] - u) / d2 * bspline_basis(i + 1, p - 1, u, knots) : 0.0;
    return a + b;
}

// k 阶导数（order = 0 时退化为基函数本身）
//   N'_{i,p} = p/(u_{i+p}-u_i) · N_{i,p-1} − p/(u_{i+p+1}-u_{i+1}) · N_{i+1,p-1}
// 高阶导对低一阶导数重复套用同型公式；零长 span 的项按惯例记 0。
static double bspline_basis_deriv(int i, int p, double u,
                                  const std::vector<double>& knots, int order) {
    if (order <= 0) return bspline_basis(i, p, u, knots);
    if (p == 0) return 0.0;
    const double d1 = knots[i + p]     - knots[i];
    const double d2 = knots[i + p + 1] - knots[i + 1];
    const double a = (d1 > 0.0)
        ? p / d1 * bspline_basis_deriv(i,     p - 1, u, knots, order - 1) : 0.0;
    const double b = (d2 > 0.0)
        ? p / d2 * bspline_basis_deriv(i + 1, p - 1, u, knots, order - 1) : 0.0;
    return a - b;
}

static Point3D curve_point_impl(double u, const std::vector<Point3D>& ctrl,
                                const std::vector<double>& knots, int degree, int order) {
    Point3D s{0, 0, 0};
    for (size_t i = 0; i < ctrl.size(); ++i)
        s = s + ctrl[i] * bspline_basis_deriv(static_cast<int>(i), degree, u, knots, order);
    return s;
}

// C(u)
static Point3D curve_eval(double u, const std::vector<Point3D>& ctrl,
                          const std::vector<double>& knots, int degree) {
    return curve_point_impl(u, ctrl, knots, degree, 0);
}
// C'(u)
static Point3D curve_deriv(double u, const std::vector<Point3D>& ctrl,
                           const std::vector<double>& knots, int degree) {
    return curve_point_impl(u, ctrl, knots, degree, 1);
}
// C''(u)
static Point3D curve_deriv2(double u, const std::vector<Point3D>& ctrl,
                            const std::vector<double>& knots, int degree) {
    return curve_point_impl(u, ctrl, knots, degree, 2);
}

static double dot3(const Point3D& a, const Point3D& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static double norm3(const Point3D& a) { return std::sqrt(dot3(a, a)); }

// =================================================================
// >>>>>>>>>>  PASTE AI-GENERATED point_inversion BELOW  <<<<<<<<<<
// =================================================================

// 返回值约定（写进你的 prompt）：
//   converged  = 是否收敛（fallback 成功也算）
//   u          = 最近点参数（必须落在 [knots.front(), knots.back()] 内）
//   distance   = |C(u) - P|
//   iterations = 实际迭代次数
struct InversionResult {
    bool   converged  = false;
    double u          = 0.0;
    double distance   = 0.0;
    int    iterations = 0;
};

InversionResult point_inversion(const Point3D& P,
                                const std::vector<Point3D>& ctrl,
                                const std::vector<double>& knots,
                                int degree,
                                double tol_distance  = 1e-9,
                                double tol_parameter = 1e-6,
                                int    max_iter      = 50) {
    // TODO: 在这里粘贴 / 实现 Newton-Raphson point inversion
    // 必含要素（建议原样写进你的 prompt）：
    //   1. 粗采样选初值：先在参数域均匀采样取最近，再 Newton 精化
    //   2. f(u) = (C(u)-P)·C'(u)，f'(u) = |C'(u)|² + (C(u)-P)·C''(u)
    //   3. |Δu| < tol_parameter 时早停
    //   4. 发散（|f| 连续 3 次不下降）时的 fallback：局部细分 / 黄金分割
    //   5. u 越界时 clamp 到 [knots.front(), knots.back()]（开曲线）
    (void)P; (void)ctrl; (void)knots; (void)degree;
    (void)tol_distance; (void)tol_parameter; (void)max_iter;
    (void)&curve_deriv2;  // 实现时删掉这行：仅用于压制未使用函数的警告
    return InversionResult{};  // 占位：未实现
}

// =================================================================
// >>>>>>>>>>             END OF PASTE ZONE              <<<<<<<<<<
// =================================================================

// 独立验证路径：密集采样 + 黄金分割细化（与 Newton 完全不同的数学路径，不复用被测函数）
static InversionResult inversion_ref(const Point3D& P,
                                     const std::vector<Point3D>& ctrl,
                                     const std::vector<double>& knots, int degree) {
    const double u0 = knots.front(), u1 = knots.back();
    const int N = 4000;
    int best = 0;
    double best_d2 = std::numeric_limits<double>::max();
    for (int k = 0; k <= N; ++k) {
        const double u = u0 + (u1 - u0) * k / N;
        const Point3D r = curve_eval(u, ctrl, knots, degree) - P;
        const double d2 = dot3(r, r);
        if (d2 < best_d2) { best_d2 = d2; best = k; }
    }
    double lo = u0 + (u1 - u0) * std::max(0, best - 1) / N;
    double hi = u0 + (u1 - u0) * std::min(N, best + 1) / N;
    const double phi = (std::sqrt(5.0) - 1.0) / 2.0;
    for (int g = 0; g < 200 && hi - lo > 1e-13; ++g) {
        const double x1 = hi - phi * (hi - lo);
        const double x2 = lo + phi * (hi - lo);
        const Point3D r1 = curve_eval(x1, ctrl, knots, degree) - P;
        const Point3D r2 = curve_eval(x2, ctrl, knots, degree) - P;
        if (dot3(r1, r1) < dot3(r2, r2)) hi = x2; else lo = x1;
    }
    InversionResult res;
    res.converged = true;
    res.u         = (lo + hi) / 2.0;
    res.distance  = norm3(curve_eval(res.u, ctrl, knots, degree) - P);
    return res;
}

int main() {
    enable_utf8_console();

    // 测试曲线：cubic clamped B 样条（关于 x=2.5 左右对称，Test 5 依赖这一点）
    const std::vector<Point3D> ctrl = {
        {0, 0, 0}, {1, 2.5, 0}, {2.5, 3.2, 0}, {4, 2.5, 0}, {5, 0, 0}};
    const std::vector<double> knots = {0, 0, 0, 0, 0.5, 1, 1, 1, 1};
    const int degree = 3;

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

    std::cout << "=== Test 1: 法向偏移构造已知解（投影必须回到构造参数 u*） ===\n";
    std::vector<Point3D> t1_points;   // 留给 SVG
    for (double u_star : {0.2, 0.5, 0.8}) {
        const Point3D C  = curve_eval (u_star, ctrl, knots, degree);
        const Point3D Cp = curve_deriv(u_star, ctrl, knots, degree);
        // 平面内单位法向（曲线在 z=0 平面内）
        const double len = std::sqrt(Cp.x * Cp.x + Cp.y * Cp.y);
        const Point3D n{-Cp.y / len, Cp.x / len, 0.0};
        const Point3D P = C + n * 0.25;     // 偏移 0.25（远小于局部曲率半径）
        t1_points.push_back(P);
        const InversionResult r = point_inversion(P, ctrl, knots, degree);
        check_true("u*=" + std::to_string(u_star) + " converged", r.converged);
        check("u*=" + std::to_string(u_star) + " u", r.u, u_star, 1e-5);
        check("u*=" + std::to_string(u_star) + " distance", r.distance, 0.25, 1e-6);
        std::cout << "         (iterations=" << r.iterations
                  << "，Newton 二次收敛通常 3~6 次)\n";
    }

    std::cout << "\n=== Test 2: 任意点 vs 密集采样参照（独立路径双算对照） ===\n";
    const std::vector<Point3D> t2_points = {
        {1.0, 3.5, 0}, {3.8, 0.5, 0}, {2.5, 5.0, 0}};
    for (const Point3D& P : t2_points) {
        const InversionResult got = point_inversion(P, ctrl, knots, degree);
        const InversionResult ref = inversion_ref(P, ctrl, knots, degree);
        const std::string tag = "P=(" + std::to_string(P.x) + "," + std::to_string(P.y) + ")";
        check(tag + " u",        got.u,        ref.u,        1e-5);
        check(tag + " distance", got.distance, ref.distance, 1e-7);
    }

    std::cout << "\n=== Test 3: 越界 clamp（最近点在端点，解析值 √5） ===\n";
    {
        const double sqrt5 = std::sqrt(5.0);
        const InversionResult rl = point_inversion({-2, -1, 0}, ctrl, knots, degree);
        const InversionResult rr = point_inversion({ 7, -1, 0}, ctrl, knots, degree);
        check("left  u",        rl.u,        0.0,   1e-9);
        check("left  distance", rl.distance, sqrt5, 1e-6);
        check("right u",        rr.u,        1.0,   1e-9);
        check("right distance", rr.distance, sqrt5, 1e-6);
        check_true("u 落在参数域内", rl.u >= 0.0 && rl.u <= 1.0
                                  && rr.u >= 0.0 && rr.u <= 1.0);
    }

    std::cout << "\n=== Test 4: 点恰好在曲线上（distance 应为 0） ===\n";
    {
        const Point3D P = curve_eval(0.37, ctrl, knots, degree);
        const InversionResult r = point_inversion(P, ctrl, knots, degree);
        check("on-curve u",        r.u,        0.37, 1e-5);
        check("on-curve distance", r.distance, 0.0,  1e-7);
    }

    std::cout << "\n=== Test 5: 等距双解（观察题，无标准答案） ===\n";
    {
        // 曲线左右对称，P 在正下方 → u=0 与 u=1 严格等距
        const InversionResult r = point_inversion({2.5, -2, 0}, ctrl, knots, degree);
        std::cout << "  P=(2.5,-2) 两个全局最近点等距：u=0 与 u=1，dist=√10.25≈3.2016\n";
        std::cout << "  AI 返回：u=" << r.u << "  distance=" << r.distance
                  << "  iterations=" << r.iterations << "\n";
        std::cout << "  问题：两侧等距时 AI 收敛到哪侧？粗采样初值策略是否让结果可复现？\n";
    }

    std::cout << "\n=== Test 6: 迭代预算不足（观察题） ===\n";
    {
        const InversionResult r = point_inversion({1.0, 3.5, 0}, ctrl, knots, degree,
                                                  1e-9, 1e-12, /*max_iter=*/2);
        std::cout << "  max_iter=2, tol_parameter=1e-12 时：converged="
                  << (r.converged ? "true" : "false")
                  << "  iterations=" << r.iterations
                  << "  distance=" << r.distance << "\n";
        std::cout << "  问题：迭代不足时 AI 返回什么？是 fallback 到细分还是直接报未收敛？\n";
        std::cout << "        distance 字段此时还可信吗？\n";
    }

    // -------- SVG 可视化 --------
    const std::string svg_name = "point_inversion_output.svg";
    {
        SVG svg(svg_name, -2.5, -3, 7.5, 6);
        // 控制多边形 + 控制点
        svg.polyline(ctrl, "#cccccc", 0.03);
        svg.dots(ctrl, "red", 0.08);
        // 曲线
        std::vector<Point3D> curve;
        curve.reserve(201);
        for (int k = 0; k <= 200; ++k)
            curve.push_back(curve_eval(k / 200.0, ctrl, knots, degree));
        svg.polyline(curve, "#2266ff", 0.04);
        // 所有测试点 + 投影脚点 + 连线
        std::vector<Point3D> queries = t1_points;
        queries.insert(queries.end(), t2_points.begin(), t2_points.end());
        queries.push_back({-2, -1, 0});
        queries.push_back({ 7, -1, 0});
        queries.push_back({2.5, -2, 0});
        for (const Point3D& P : queries) {
            const InversionResult r = point_inversion(P, ctrl, knots, degree);
            const Point3D foot = curve_eval(r.u, ctrl, knots, degree);
            svg.line(P.x, P.y, foot.x, foot.y, "#ff7f00", 0.02);   // 连线（应⊥曲线）
            svg.dots({foot}, "#4daf4a", 0.07);                     // 脚点 - 绿
            svg.dots({P},    "black",   0.09);                     // 测试点 - 黑
        }
    }

    const auto abs_path = std::filesystem::absolute(svg_name);
    std::cout << "\n生成: " << abs_path.string() << "\n";
    std::cout << "可视判断：橙色连线应垂直于曲线切向（端点 clamp 的两条除外）。\n";
    std::cout << (all_pass ? "\nAll numeric checks passed.\n"
                           : "\nSome checks failed. 检查 AI 代码。\n");
    return all_pass ? 0 : 1;
}
