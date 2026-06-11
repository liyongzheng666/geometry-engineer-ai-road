// test_point_in_polygon.cpp — 点在多边形内（射线法）的验证闭环
// 用法：把 AI 生成的 point_in_polygon 函数粘到 PASTE ZONE 之间，重新编译即可
//
// 这是几何代码经典的"退化重灾区"：射线穿顶点、射线与边共线、点在边上、
// 顶点数不足 …… 每一条都是工业代码里真实炸过的坑。
// 独立验证路径用 winding number（atan2 角度和），与射线法完全不同的数学。

#include "geo.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =================================================================
// >>>>>>>>>>  PASTE AI-GENERATED point_in_polygon BELOW  <<<<<<<<<<
// =================================================================

// 输入约定（写进你的 prompt）：
//   poly 为顶点序列（首尾不重复闭合）；所有点都在 z=0 平面，z 分量忽略。
// 返回值约定：
//   1 = 严格内部，0 = 严格外部，-1 = 恰在边界上
//   （允许 AI 简化为 0/1 二值，但必须在注释里写明它的边界约定是什么）
int point_in_polygon(const Point3D& P, const std::vector<Point3D>& poly) {
    // TODO: 在这里粘贴 / 实现射线法点在多边形内判定
    // 必须明确处理（建议原样写进你的 prompt）：
    //   1. 射线穿过多边形顶点 → tie-breaking 规则（如半开区间 (yi>y)!=(yj>y)）
    //   2. 射线与水平边共线 → 该边如何跳过
    //   3. 点恰好在边上 → 返回什么（并在注释里写明约定）
    //   4. 顶点数 < 3 → 提前返回 0
    //   5. 非简单/自相交多边形 → 行为约定（even-odd？winding？）写进注释
    (void)P; (void)poly;
    return 0;  // 占位：未实现
}

// =================================================================
// >>>>>>>>>>             END OF PASTE ZONE              <<<<<<<<<<
// =================================================================

// 独立验证路径：winding number（atan2 有向角度和）。
// 注意：仅对【不在边界上】的点有效——边界点的角度和无定义。
static bool winding_ref(const Point3D& P, const std::vector<Point3D>& poly) {
    const int n = static_cast<int>(poly.size());
    double sum = 0.0;
    for (int i = 0, j = n - 1; i < n; j = i++) {
        const double ax = poly[j].x - P.x, ay = poly[j].y - P.y;
        const double bx = poly[i].x - P.x, by = poly[i].y - P.y;
        sum += std::atan2(ax * by - ay * bx, ax * bx + ay * by);
    }
    return std::abs(sum) > M_PI;   // 内部 ≈ ±2π，外部 ≈ 0
}

// 点到多边形边界的最小距离（SVG 双算对照时跳过近边界带用）
static double dist_to_boundary(const Point3D& P, const std::vector<Point3D>& poly) {
    const int n = static_cast<int>(poly.size());
    double best = 1e300;
    for (int i = 0, j = n - 1; i < n; j = i++) {
        const double abx = poly[i].x - poly[j].x, aby = poly[i].y - poly[j].y;
        const double apx = P.x - poly[j].x,       apy = P.y - poly[j].y;
        const double len2 = abx * abx + aby * aby;
        double s = (len2 > 0.0) ? (apx * abx + apy * aby) / len2 : 0.0;
        s = std::max(0.0, std::min(1.0, s));
        const double dx = apx - s * abx, dy = apy - s * aby;
        best = std::min(best, std::sqrt(dx * dx + dy * dy));
    }
    return best;
}

int main() {
    enable_utf8_console();

    // 四个测试多边形（首尾不重复闭合）
    const std::vector<Point3D> S = {{0, 0, 0}, {4, 0, 0}, {4, 4, 0}, {0, 4, 0}};          // 正方形
    const std::vector<Point3D> D = {{0, 0, 0}, {4, 0, 0}, {4, 4, 0}, {2, 1.5, 0}, {0, 4, 0}};  // 凹"飞镖"
    const std::vector<Point3D> V = {{2, 0, 0}, {4, 2, 0}, {2, 4, 0}, {0, 2, 0}};          // 菱形（顶点 y=2）
    const std::vector<Point3D> E = {{0, 0, 0}, {4, 0, 0}, {4, 2, 0}, {6, 2, 0}, {6, 4, 0}, {0, 4, 0}};  // 台阶（含水平边）

    bool all_pass = true;
    // expect = 1/0；同时与 winding_ref 双算对照（仅非边界点可用）
    auto check = [&](const std::string& name, const Point3D& P,
                     const std::vector<Point3D>& poly, int expect) {
        const int got = point_in_polygon(P, poly);
        const bool wind = winding_ref(P, poly);
        const bool ok = (got == expect) && ((got == 1) == wind);
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << name
                  << "  got=" << got << "  expect=" << expect
                  << "  winding=" << (wind ? 1 : 0) << "\n";
        if (!ok) all_pass = false;
    };

    std::cout << "=== Test 1: 凸多边形基线（正方形） ===\n";
    check("S 内 (2,2)",     {2, 2, 0},     S, 1);
    check("S 内 (0.5,3.5)", {0.5, 3.5, 0}, S, 1);
    check("S 内 (3.9,0.1)", {3.9, 0.1, 0}, S, 1);
    check("S 外 (5,2)",     {5, 2, 0},     S, 0);
    check("S 外 (-1,-1)",   {-1, -1, 0},   S, 0);
    check("S 外 (2,5)",     {2, 5, 0},     S, 0);

    std::cout << "\n=== Test 2: 凹多边形（飞镖，凹口处射线要数 2 次交叉） ===\n";
    check("D 内 (1,1)",       {1, 1, 0},       D, 1);
    check("D 内 (3,1)",       {3, 1, 0},       D, 1);
    check("D 内·左角 (0.2,3.5)",  {0.2, 3.5, 0},   D, 1);   // 左角在 y=3.5 处只有 x∈[0,0.4] 宽
    check("D 内·右角 (3.95,3.9)", {3.95, 3.9, 0},  D, 1);
    check("D 外·角旁 (0.5,3.5)",  {0.5, 3.5, 0},   D, 0);   // 左角边界 x=0.4 再往外 0.1
    check("D 外·凹口 (2,3)",  {2, 3, 0},       D, 0);
    check("D 外·凹口 (2,1.6)",{2, 1.6, 0},     D, 0);
    check("D 外 (5,1)",       {5, 1, 0},       D, 0);
    check("D 外 (-0.5,2)",    {-0.5, 2, 0},    D, 0);

    std::cout << "\n=== Test 3: 射线穿顶点（菱形左右顶点 y=2，query 的 y 也取 2） ===\n";
    std::cout << "  朴素实现\"碰到顶点算一次交叉\"会把共享顶点的两条边各数一遍 → 内外判反\n";
    check("V 内 (1,2)",  {1, 2, 0},  V, 1);
    check("V 内 (3,2)",  {3, 2, 0},  V, 1);
    check("V 外 (-1,2)", {-1, 2, 0}, V, 0);
    check("V 外 (5,2)",  {5, 2, 0},  V, 0);

    std::cout << "\n=== Test 4: 射线与水平边共线（台阶形的边 (4,2)-(6,2)） ===\n";
    check("E 内 (1,2)（射线与边共线）", {1, 2, 0}, E, 1);
    check("E 外 (7,2)",                {7, 2, 0}, E, 0);
    check("E 外 (5,1)",                {5, 1, 0}, E, 0);
    check("E 内 (5,3)",                {5, 3, 0}, E, 1);

    std::cout << "\n=== Test 5: 点恰在边/顶点上（观察题，无标准答案） ===\n";
    {
        const int r1 = point_in_polygon({2, 0, 0}, S);   // 边中点
        const int r2 = point_in_polygon({0, 0, 0}, S);   // 顶点
        const int r3 = point_in_polygon({5, 2, 0}, E);   // 水平边上
        std::cout << "  S 边中点 (2,0) → " << r1
                  << "   S 顶点 (0,0) → " << r2
                  << "   E 水平边上 (5,2) → " << r3 << "\n";
        std::cout << "  问题：AI 的边界约定是内 / 外 / -1？三处返回一致吗？\n";
        std::cout << "        与它代码注释里【声称】的约定一致吗？\n";
    }

    std::cout << "\n=== Test 6: 配置退化（顶点数 < 3、共线多边形） ===\n";
    {
        auto check0 = [&](const std::string& name, const Point3D& P,
                          const std::vector<Point3D>& poly) {
            const int got = point_in_polygon(P, poly);
            const bool ok = (got == 0);
            std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << name
                      << "  got=" << got << "  expect=0\n";
            if (!ok) all_pass = false;
        };
        check0("空多边形",   {1, 1, 0}, {});
        check0("单点多边形", {1, 1, 0}, {{0, 0, 0}});
        check0("两点多边形", {1, 1, 0}, {{0, 0, 0}, {4, 0, 0}});
        const std::vector<Point3D> L = {{0, 0, 0}, {2, 0, 0}, {4, 0, 0}};  // 三点共线，零面积
        check0("共线多边形 (1,1)", {1, 1, 0}, L);
        std::cout << "  观察：共线多边形上的点 (1,0) → "
                  << point_in_polygon({1, 0, 0}, L)
                  << "（零面积多边形的\"边上\"算什么？AI 有没有想过这种输入？）\n";
    }

    // -------- SVG：网格采样染色 + 双算不一致点 --------
    const std::string svg_name = "point_in_polygon_output.svg";
    {
        // 飞镖 D 画在原位，台阶 E 整体右移 +7
        std::vector<Point3D> E_shift = E;
        for (auto& p : E_shift) p.x += 7.0;

        SVG svg(svg_name, -1.5, -1.5, 14.5, 5.5);
        auto draw_poly = [&](const std::vector<Point3D>& poly) {
            std::vector<Point3D> closed = poly;
            closed.push_back(poly.front());
            svg.polyline(closed, "black", 0.04);
        };
        auto fill_region = [&](const std::vector<Point3D>& poly,
                               double x0, double y0, double x1, double y1) {
            for (double x = x0; x <= x1; x += 0.15) {
                for (double y = y0; y <= y1; y += 0.15) {
                    const Point3D P{x, y, 0};
                    const int r = point_in_polygon(P, poly);
                    if      (r == 1)  svg.dots({P}, "#4daf4a", 0.05);   // 内 - 绿
                    else if (r == -1) svg.dots({P}, "#ff7f00", 0.05);   // 边界 - 橙
                    else              svg.dots({P}, "#dddddd", 0.03);   // 外 - 浅灰
                    // 双算对照（跳过近边界 0.05 带，那里 winding 数值不稳）
                    if (dist_to_boundary(P, poly) > 0.05 &&
                        (r == 1) != winding_ref(P, poly))
                        svg.dots({P}, "#e41a1c", 0.09);                 // 不一致 - 红大点
                }
            }
        };
        fill_region(D,       -0.5, -0.5,  4.5, 4.5);
        fill_region(E_shift,  6.5, -0.5, 13.5, 4.5);
        draw_poly(D);
        draw_poly(E_shift);
    }

    const auto abs_path = std::filesystem::absolute(svg_name);
    std::cout << "\n生成: " << abs_path.string() << "\n";
    std::cout << "可视判断：绿色应严丝合缝填满多边形内部；\n";
    std::cout << "          任何红色大点 = 射线法与 winding number 双算不一致处。\n";
    std::cout << (all_pass ? "\nAll numeric checks passed.\n"
                           : "\nSome checks failed. 检查 AI 代码。\n");
    return all_pass ? 0 : 1;
}
