// test_bezier.cpp — Bezier 求值的验证闭环
// 用法：把 AI 生成的 bezier_cubic 函数粘到 PASTE ZONE 之间，重新编译即可

#include "geo.h"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

// =================================================================
// >>>>>>>>>>  PASTE AI-GENERATED bezier_cubic BELOW  <<<<<<<<<<
// =================================================================

Point3D bezier_cubic(const Point3D& P0, const Point3D& P1,
                     const Point3D& P2, const Point3D& P3, double t) {
    // TODO: 在这里粘贴 / 实现三次 Bezier 求值（如 De Casteljau 算法）
    (void)P0; (void)P1; (void)P2; (void)P3; (void)t;
    return Point3D{0, 0, 0};  // 占位：未实现
}

// =================================================================
// >>>>>>>>>>             END OF PASTE ZONE              <<<<<<<<<<
// =================================================================

// 独立验证路径：闭式 Bernstein 公式，不复用上面被测函数
static Point3D bernstein_ref(const Point3D& P0, const Point3D& P1,
                             const Point3D& P2, const Point3D& P3, double t) {
    const double u = 1.0 - t;
    return P0 * (u * u * u)
         + P1 * (3.0 * u * u * t)
         + P2 * (3.0 * u * t * t)
         + P3 * (t * t * t);
}

int main() {
    enable_utf8_console();

    bool all_pass = true;
    auto check = [&](const std::string& name, const Point3D& a, const Point3D& b,
                     double tol = 1e-9) {
        const double dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
        const double err = std::sqrt(dx*dx + dy*dy + dz*dz);
        const bool ok = err < tol;
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << name
                  << "  got=(" << a.x << "," << a.y << "," << a.z << ")"
                  << "  expect=(" << b.x << "," << b.y << "," << b.z << ")"
                  << "  err=" << err << "\n";
        if (!ok) all_pass = false;
    };

    const Point3D P0{0, 0, 0};
    const Point3D P1{1, 4, 0};
    const Point3D P2{4, 4, 0};
    const Point3D P3{5, 0, 0};

    std::cout << "=== Test 1: Normal curve (vs Bernstein reference) ===\n";
    for (double t : {0.0, 0.25, 0.5, 0.75, 1.0}) {
        const Point3D got = bezier_cubic(P0, P1, P2, P3, t);
        const Point3D ref = bernstein_ref(P0, P1, P2, P3, t);
        check("t=" + std::to_string(t), got, ref);
    }

    std::cout << "\n=== Test 2: Endpoint interpolation ===\n";
    check("P(0)=P0", bezier_cubic(P0, P1, P2, P3, 0.0), P0);
    check("P(1)=P3", bezier_cubic(P0, P1, P2, P3, 1.0), P3);

    std::cout << "\n=== Test 3: Coincident control points ===\n";
    const Point3D Q{1, 1, 1};
    check("4-point-coincident", bezier_cubic(Q, Q, Q, Q, 0.5), Q);

    std::cout << "\n=== Test 4: Out-of-range t (degenerate) ===\n";
    const Point3D low  = bezier_cubic(P0, P1, P2, P3, -0.5);
    const Point3D high = bezier_cubic(P0, P1, P2, P3,  1.5);
    std::cout << "  P(-0.5) = (" << low.x  << "," << low.y  << "," << low.z  << ")\n";
    std::cout << "  P( 1.5) = (" << high.x << "," << high.y << "," << high.z << ")\n";
    std::cout << "  问题：AI 的代码是夹紧 / 外推 / 抛异常？是否符合你的 prompt 约束？\n";

    // -------- SVG 可视化 --------
    const std::string svg_name = "bezier_output.svg";
    {
        SVG svg(svg_name, -1, -1, 6, 5);
        svg.polyline({P0, P1, P2, P3}, "#cccccc", 0.03);   // 控制多边形
        svg.dots({P0, P1, P2, P3}, "red", 0.08);           // 控制点
        std::vector<Point3D> curve;
        curve.reserve(101);
        for (int i = 0; i <= 100; ++i) {
            curve.push_back(bezier_cubic(P0, P1, P2, P3, i / 100.0));
        }
        svg.polyline(curve, "#2266ff", 0.04);              // Bezier 曲线
    }

    const auto abs_path = std::filesystem::absolute(svg_name);
    std::cout << "\n=== SVG ===\n";
    std::cout << "生成: " << abs_path.string() << "\n";
    std::cout << "把这个文件拖进浏览器（Chrome / Edge）查看曲线形态。\n";
    std::cout << (all_pass ? "\nAll numeric checks passed.\n"
                           : "\nSome checks failed. 检查 AI 代码。\n");
    return all_pass ? 0 : 1;
}
