// test_bspline.cpp — Cox-de Boor B 样条基函数的验证闭环

#include "geo.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

// =================================================================
// >>>>>>>>>>  PASTE AI-GENERATED basis FUNCTION BELOW  <<<<<<<<<<
// =================================================================

double basis(int i, int p, double u, const std::vector<double>& knots) {
    // TODO: 在这里粘贴 / 实现 Cox-de Boor 递推 N_{i,p}(u)
    (void)i; (void)p; (void)u; (void)knots;
    return 0.0;  // 占位：未实现
}

// =================================================================
// >>>>>>>>>>             END OF PASTE ZONE              <<<<<<<<<<
// =================================================================

int main() {
    enable_utf8_console();

    // 经典 cubic clamped knot vector
    const std::vector<double> knots = {0, 0, 0, 0, 0.25, 0.5, 0.75, 1, 1, 1, 1};
    const int p = 3;
    const int n_ctrl = static_cast<int>(knots.size()) - p - 1;  // = 7

    bool all_pass = true;

    std::cout << "=== Test 1: Partition of unity (sum should = 1) ===\n";
    for (double u : {0.0, 0.1, 0.3, 0.5, 0.7, 0.99}) {
        double sum = 0.0;
        for (int i = 0; i < n_ctrl; ++i) sum += basis(i, p, u, knots);
        const bool ok = std::abs(sum - 1.0) < 1e-9;
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ")
                  << "u=" << u << "  sum=" << sum
                  << "  err=" << std::abs(sum - 1.0) << "\n";
        if (!ok) all_pass = false;
    }

    std::cout << "\n=== Test 2: Non-negativity ===\n";
    int violations = 0;
    for (double u = 0.0; u < 1.0; u += 0.01) {
        for (int i = 0; i < n_ctrl; ++i) {
            if (basis(i, p, u, knots) < -1e-12) ++violations;
        }
    }
    std::cout << (violations == 0 ? "  [PASS] " : "  [FAIL] ")
              << "negative-value violations: " << violations << "\n";
    if (violations > 0) all_pass = false;

    std::cout << "\n=== Test 3: Boundary behavior at u=1.0 ===\n";
    double sum_at_one = 0.0;
    for (int i = 0; i < n_ctrl; ++i) sum_at_one += basis(i, p, 1.0, knots);
    std::cout << "  sum at u=1.0 = " << sum_at_one << "\n";
    std::cout << "  注意：标准 Cox-de Boor 在 u=1.0 会返回 sum=0（半开区间问题）。\n";
    std::cout << "  AI 是否处理了这个 corner case？\n";

    // -------- SVG 可视化：每个 N_{i,3}(u) 一条彩色曲线 --------
    const std::string svg_name = "bspline_output.svg";
    {
        SVG svg(svg_name, -0.05, -0.1, 1.05, 1.2);
        // 坐标轴
        svg.line(0, 0, 1, 0, "#888888", 0.005);
        svg.line(0, 0, 0, 1, "#888888", 0.005);
        // knot 位置（短刻度）
        for (double k : knots) svg.line(k, 0, k, 0.05, "#aaaaaa", 0.005);

        const char* colors[] = {
            "#e41a1c", "#377eb8", "#4daf4a", "#984ea3",
            "#ff7f00", "#a65628", "#f781bf"
        };
        for (int i = 0; i < n_ctrl; ++i) {
            std::vector<Point3D> curve;
            curve.reserve(501);
            for (int k = 0; k <= 500; ++k) {
                const double u = k / 500.0;
                curve.push_back({u, basis(i, p, u, knots), 0.0});
            }
            svg.polyline(curve, colors[i % 7], 0.008);
        }
    }

    const auto abs_path = std::filesystem::absolute(svg_name);
    std::cout << "\n生成: " << abs_path.string() << "\n";
    std::cout << "可视判断：曲线是否光滑？是否非负？每个 knot span 内只有 4 条非零？\n";
    std::cout << (all_pass ? "\nAll numeric checks passed.\n"
                           : "\nSome checks failed. 检查 AI 代码。\n");
    return all_pass ? 0 : 1;
}
