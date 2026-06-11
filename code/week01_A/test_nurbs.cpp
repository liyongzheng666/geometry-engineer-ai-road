// test_nurbs.cpp — NURBS 曲线求值的验证闭环
// 金标准：用标准权重的 1/4 圆 NURBS，求值后必须落在单位圆上

#include "geo.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

// MSVC 的 <cmath> 在 _USE_MATH_DEFINES（CMake 已设）下提供 M_PI
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 本测试需要 B 样条基函数；独立实现一份，避免与 test_bspline.cpp 链接冲突
static double bspline_basis(int i, int p, double u, const std::vector<double>& knots) {
    if (p == 0) {
        // 标准半开区间 [u_i, u_{i+1})
        if (u >= knots[i] && u < knots[i + 1]) return 1.0;
        // corner case：u = u_max 时闭合最后一个非空 span，避免右端点处
        // 所有基函数归零（否则 nurbs_eval 分母为 0 → NaN）。与 test_bspline.cpp 一致。
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

// =================================================================
// >>>>>>>>>>  PASTE AI-GENERATED nurbs_eval BELOW  <<<<<<<<<<
// =================================================================

Point3D nurbs_eval(double u,
                   const std::vector<Point3D>& ctrl,
                   const std::vector<double>& weights,
                   const std::vector<double>& knots,
                   int degree) {
    // TODO: 在这里粘贴 / 实现 NURBS 求值（可调用上方的 bspline_basis）
    (void)u; (void)ctrl; (void)weights; (void)knots; (void)degree;
    (void)&bspline_basis;  // 实现时删掉这行：仅用于压制未使用函数的警告
    return Point3D{0, 0, 0};  // 占位：未实现
}

// =================================================================
// >>>>>>>>>>          END OF PASTE ZONE                <<<<<<<<<<
// =================================================================

int main() {
    enable_utf8_console();

    // 经典 1/4 圆 NURBS：3 个控制点
    // 权重 (1, sqrt(2)/2, 1) 让有理 B 样条精确表示一段圆弧
    const std::vector<Point3D> ctrl       = {{1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
    const std::vector<double>  w_circle   = {1.0, std::sqrt(2.0) / 2.0, 1.0};
    const std::vector<double>  w_uniform  = {1.0, 1.0, 1.0};
    const std::vector<double>  knots      = {0, 0, 0, 1, 1, 1};
    const int                  degree     = 2;

    bool all_pass = true;
    auto check = [&](const std::string& name, double got, double expect,
                     double tol = 1e-6) {
        const bool ok = std::abs(got - expect) < tol;
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << name
                  << "  got=" << got << "  expect=" << expect << "\n";
        if (!ok) all_pass = false;
    };

    std::cout << "=== Test 1: Quarter-circle NURBS — 求值点必须落在单位圆上 ===\n";
    for (double u : {0.0, 0.25, 0.5, 0.75, 1.0}) {
        const Point3D p = nurbs_eval(u, ctrl, w_circle, knots, degree);
        const double r = std::sqrt(p.x * p.x + p.y * p.y);
        check("|P(u=" + std::to_string(u) + ")|=1", r, 1.0);
    }

    std::cout << "\n=== Test 2: Endpoint interpolation ===\n";
    const Point3D p0 = nurbs_eval(0.0,         ctrl, w_circle, knots, degree);
    const Point3D p1 = nurbs_eval(1.0 - 1e-9,  ctrl, w_circle, knots, degree);
    check("P(0).x", p0.x, 1.0);
    check("P(0).y", p0.y, 0.0);
    check("P(1).x", p1.x, 0.0);
    check("P(1).y", p1.y, 1.0);

    std::cout << "\n=== Test 3: 均匀权重 → 退化为 B 样条（不再是圆） ===\n";
    const Point3D mid_uniform = nurbs_eval(0.5, ctrl, w_uniform, knots, degree);
    const double r_uniform = std::sqrt(mid_uniform.x * mid_uniform.x
                                     + mid_uniform.y * mid_uniform.y);
    std::cout << "  |P(0.5)| 均匀权重 = " << r_uniform
              << "  （应明显小于 1，约 0.85）\n";

    // -------- SVG: 圆弧（红） + 均匀权重（蓝） + 参考单位圆（灰） --------
    const std::string svg_name = "nurbs_output.svg";
    {
        SVG svg(svg_name, -0.2, -0.2, 1.4, 1.4);
        // 参考单位圆
        std::vector<Point3D> ref_circle;
        ref_circle.reserve(91);
        for (int k = 0; k <= 90; ++k) {
            const double a = k * (M_PI / 2.0) / 90.0;
            ref_circle.push_back({std::cos(a), std::sin(a), 0.0});
        }
        svg.polyline(ref_circle, "#cccccc", 0.01);

        // NURBS 求值结果
        std::vector<Point3D> arc, arc_uniform;
        arc.reserve(201);
        arc_uniform.reserve(201);
        for (int k = 0; k <= 200; ++k) {
            const double u = k / 200.0;
            arc.push_back(nurbs_eval(u, ctrl, w_circle,  knots, degree));
            arc_uniform.push_back(nurbs_eval(u, ctrl, w_uniform, knots, degree));
        }
        svg.polyline(arc,         "#e41a1c", 0.015);  // 真圆弧 - 红
        svg.polyline(arc_uniform, "#377eb8", 0.015);  // 均匀权重 - 蓝
        svg.polyline(ctrl,        "#999999", 0.008);  // 控制多边形
        svg.dots(ctrl,            "black",   0.025);
    }

    const auto abs_path = std::filesystem::absolute(svg_name);
    std::cout << "\n生成: " << abs_path.string() << "\n";
    std::cout << "可视判断：\n";
    std::cout << "  灰色 = 真正的单位圆\n";
    std::cout << "  红色 = NURBS（权重对的，应严丝合缝贴住灰色）\n";
    std::cout << "  蓝色 = 退化 B 样条（权重全 1，应明显偏离灰色）\n";
    std::cout << "  如果红色不贴合灰色，AI 的 NURBS 求值实现有问题。\n";
    std::cout << (all_pass ? "\nAll numeric checks passed.\n"
                           : "\nSome checks failed. 检查 AI 代码。\n");
    return all_pass ? 0 : 1;
}
