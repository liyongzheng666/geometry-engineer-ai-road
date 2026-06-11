// test_circumcircle.cpp — 三点定圆（限定 gx:: 类型）的验证闭环
// 用法：把 AI 生成的 circumcircle 函数粘到 PASTE ZONE 之间，重新编译即可
//
// 本周考点不是算法本身（三点定圆是基本功），而是【上下文注入】：
// 把 mini_types.h 全文贴进 AI 上下文 + 禁令清单，看 AI 是否服从注入的项目风格。
// 风格判分三条（main 末尾会再提醒你逐条肉眼 review）：
//   ① 没出现 STL 容器  ② 用 .add()/.sub() 而非运算符  ③ 错误路径返回正确的 GxStatus

#include "geo.h"
#include "mini_types.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

// =================================================================
// >>>>>>>>>>  PASTE AI-GENERATED circumcircle BELOW  <<<<<<<<<<
// =================================================================

// 约束（连同 mini_types.h 全文一起写进你的 prompt）：
//   只准使用 mini_types.h 的类型/方法 + <cmath>；
//   禁止 STL 容器、禁止异常、禁止 geo.h 的 Point3D；
//   退化输入（共线/重合）返回相应 GxStatus，容差用 gx::kEpsGeom。
gx::GxStatus circumcircle(const gx::Vec3& a, const gx::Vec3& b, const gx::Vec3& c,
                          gx::Circle3& out) {
    // TODO: 在这里粘贴 / 实现三维三点定圆
    // 提示：经典公式 center = a + [ |ac|²(n×ab) + |ab|²(ac×n) ] / (2|n|²)，n = ab×ac
    //       退化检测先于除法（重合点、共线各自返回什么 status？）
    (void)a; (void)b; (void)c; (void)out;
    return gx::GxStatus::kNumericFailure;  // 占位：未实现
}

// =================================================================
// >>>>>>>>>>             END OF PASTE ZONE              <<<<<<<<<<
// =================================================================

// 独立验证路径：裸 double 的 2D 行列式闭式解（刻意不用 gx 类型，仅适用于 z=0）
static void circumcircle_ref2d(double ax, double ay, double bx, double by,
                               double cx, double cy,
                               double& ux, double& uy, double& r) {
    const double d = 2.0 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
    const double a2 = ax * ax + ay * ay;
    const double b2 = bx * bx + by * by;
    const double c2 = cx * cx + cy * cy;
    ux = (a2 * (by - cy) + b2 * (cy - ay) + c2 * (ay - by)) / d;
    uy = (a2 * (cx - bx) + b2 * (ax - cx) + c2 * (bx - ax)) / d;
    r = std::sqrt((ax - ux) * (ax - ux) + (ay - uy) * (ay - uy));
}

// harness 基础设施：gx::Vec3 → geo.h Point3D 桥接（仅 SVG 用，不进 mini_types.h）
static Point3D to_p3(const gx::Vec3& v) { return {v.x(), v.y(), v.z()}; }

int main() {
    enable_utf8_console();

    bool all_pass = true;
    auto check = [&](const std::string& name, double got, double expect,
                     double tol = 1e-9) {
        const bool ok = std::abs(got - expect) < tol;
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << name
                  << "  got=" << got << "  expect=" << expect << "\n";
        if (!ok) all_pass = false;
    };
    auto check_true = [&](const std::string& name, bool ok) {
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << name << "\n";
        if (!ok) all_pass = false;
    };

    // 留给 SVG：z=0 平面上的（三角形, 求出的圆）
    struct Drawn { gx::Vec3 a, b, c; gx::Circle3 circ; bool ok; };
    std::vector<Drawn> drawn;

    std::cout << "=== Test 1: 由答案构造（圆心 (1,2,0)、r=2.5 上取三点） ===\n";
    {
        const double R = 2.5, CX = 1.0, CY = 2.0;
        auto on_circle = [&](double deg) {
            const double a = deg * M_PI / 180.0;
            return gx::Vec3(CX + R * std::cos(a), CY + R * std::sin(a), 0.0);
        };
        const gx::Vec3 a = on_circle(20), b = on_circle(100), c = on_circle(200);
        gx::Circle3 out;
        const gx::GxStatus s = circumcircle(a, b, c, out);
        check_true("status == kOk", s == gx::GxStatus::kOk);
        check("center.x", out.center.x(), CX);
        check("center.y", out.center.y(), CY);
        check("center.z", out.center.z(), 0.0);
        check("radius",   out.radius,    R);
        // 双算对照：2D 行列式参考
        double ux, uy, r;
        circumcircle_ref2d(a.x(), a.y(), b.x(), b.y(), c.x(), c.y(), ux, uy, r);
        check("vs ref2d center.x", out.center.x(), ux);
        check("vs ref2d radius",   out.radius,    r);
        drawn.push_back({a, b, c, out, s == gx::GxStatus::kOk});
    }

    std::cout << "\n=== Test 2: 等距性质（任意三角形，圆心到三点距离相等） ===\n";
    {
        const gx::Vec3 a(0, 0, 0), b(4, 0, 0), c(1, 3, 0);
        gx::Circle3 out;
        const gx::GxStatus s = circumcircle(a, b, c, out);
        check_true("status == kOk", s == gx::GxStatus::kOk);
        check("d(center,a) == r", out.center.distance_to(a), out.radius);
        check("d(center,b) == r", out.center.distance_to(b), out.radius);
        check("d(center,c) == r", out.center.distance_to(c), out.radius);
        double ux, uy, r;
        circumcircle_ref2d(0, 0, 4, 0, 1, 3, ux, uy, r);
        check("vs ref2d radius", out.radius, r);
        drawn.push_back({a, b, c, out, s == gx::GxStatus::kOk});
    }

    std::cout << "\n=== Test 3: 直角三角形特例（外心 = 斜边中点） ===\n";
    {
        const gx::Vec3 a(0, 0, 0), b(4, 0, 0), c(0, 3, 0);
        gx::Circle3 out;
        const gx::GxStatus s = circumcircle(a, b, c, out);
        check_true("status == kOk", s == gx::GxStatus::kOk);
        check("center.x", out.center.x(), 2.0);    // 斜边 bc 中点 (2, 1.5)
        check("center.y", out.center.y(), 1.5);
        check("radius",   out.radius,    2.5);
        drawn.push_back({a, b, c, out, s == gx::GxStatus::kOk});
    }

    std::cout << "\n=== Test 4: 三维倾斜圆（法向 (1,1,1)/√3，验证不是只会 2D） ===\n";
    {
        // 单位正交基（设计时已验证 e1⊥e2、均为单位长）
        const double s2 = std::sqrt(2.0), s6 = std::sqrt(6.0);
        const gx::Vec3 e1(1 / s2, -1 / s2, 0);
        const gx::Vec3 e2(1 / s6, 1 / s6, -2 / s6);
        auto on_circle = [&](double deg) {
            const double a = deg * M_PI / 180.0;
            return e1.scaled(2.0 * std::cos(a)).add(e2.scaled(2.0 * std::sin(a)));
        };
        gx::Circle3 out;
        const gx::GxStatus s = circumcircle(on_circle(0), on_circle(120), on_circle(240), out);
        check_true("status == kOk", s == gx::GxStatus::kOk);
        check("center == 原点 (x)", out.center.x(), 0.0);
        check("center == 原点 (y)", out.center.y(), 0.0);
        check("center == 原点 (z)", out.center.z(), 0.0);
        check("radius", out.radius, 2.0);
    }

    std::cout << "\n=== Test 5: 三点共线（必须失败） ===\n";
    {
        gx::Circle3 out;
        const gx::GxStatus s = circumcircle({0, 0, 0}, {1, 0, 0}, {2, 0, 0}, out);
        check_true("status != kOk", s != gx::GxStatus::kOk);
        std::cout << "  返回 status = " << gx::status_name(s)
                  << "（语义上应是 kDegenerateInput）\n";
    }

    std::cout << "\n=== Test 6: 重合点（必须失败，且输出不得含 NaN） ===\n";
    {
        gx::Circle3 out;   // 注意默认构造是全 0，函数失败时不应往里写垃圾
        const gx::GxStatus s1 = circumcircle({1, 1, 0}, {1, 1, 0}, {3, 2, 0}, out);
        check_true("两点重合 status != kOk", s1 != gx::GxStatus::kOk);
        check_true("两点重合输出无 NaN",
                   std::isfinite(out.center.x()) && std::isfinite(out.center.y())
                && std::isfinite(out.center.z()) && std::isfinite(out.radius));
        const gx::GxStatus s2 = circumcircle({1, 1, 0}, {1, 1, 0}, {1, 1, 0}, out);
        check_true("三点全重合 status != kOk", s2 != gx::GxStatus::kOk);
    }

    std::cout << "\n=== Test 7: 近共线（观察题：阈值哲学） ===\n";
    {
        gx::Circle3 out;
        const gx::GxStatus s = circumcircle({0, 0, 0}, {2, 1e-9, 0}, {4, 0, 0}, out);
        std::cout << "  三点 (0,0) (2,1e-9) (4,0)：status = " << gx::status_name(s);
        if (s == gx::GxStatus::kOk) std::cout << "  radius = " << out.radius << "（≈1e9 量级）";
        std::cout << "\n  问题：AI 的共线阈值是多少？绝对 eps 还是相对边长的 eps？\n";
        std::cout << "        它用了 gx::kEpsGeom 吗——风格注入是否真的生效？\n";
    }

    // -------- SVG：z=0 平面的三组（三角形 + 外接圆） --------
    const std::string svg_name = "circumcircle_output.svg";
    {
        SVG svg(svg_name, -3, -2, 7, 6);
        // Test 1 的"真值圆"垫底（灰色）——蓝圆应严丝合缝盖住它
        {
            std::vector<Point3D> truth;
            truth.reserve(91);
            for (int k = 0; k <= 90; ++k) {
                const double a = k * 2.0 * M_PI / 90.0;
                truth.push_back({1.0 + 2.5 * std::cos(a), 2.0 + 2.5 * std::sin(a), 0});
            }
            svg.polyline(truth, "#cccccc", 0.04);
        }
        for (const Drawn& d : drawn) {
            svg.polyline({to_p3(d.a), to_p3(d.b), to_p3(d.c), to_p3(d.a)}, "black", 0.03);
            svg.dots({to_p3(d.a), to_p3(d.b), to_p3(d.c)}, "black", 0.06);
            if (!d.ok) continue;
            std::vector<Point3D> circ;
            circ.reserve(91);
            for (int k = 0; k <= 90; ++k) {
                const double a = k * 2.0 * M_PI / 90.0;
                circ.push_back({d.circ.center.x() + d.circ.radius * std::cos(a),
                                d.circ.center.y() + d.circ.radius * std::sin(a), 0});
            }
            svg.polyline(circ, "#2266ff", 0.025);
            svg.dots({to_p3(d.circ.center)}, "red", 0.07);
        }
    }

    const auto abs_path = std::filesystem::absolute(svg_name);
    std::cout << "\n生成: " << abs_path.string() << "\n";
    std::cout << "可视判断：每个蓝圆都应穿过对应三角形的三个顶点；\n";
    std::cout << "          Test 1 的蓝圆应严丝合缝盖住灰色真值圆。\n";

    std::cout << "\n―― 风格自查（本周真正的考点，肉眼 review PASTE ZONE）――\n";
    std::cout << "  ① 是否出现 std:: 容器（vector/array/optional）？\n";
    std::cout << "  ② 是否用了 a+b 这类运算符，而不是 a.add(b)？\n";
    std::cout << "  ③ 错误路径是否返回了语义正确的 GxStatus？容差用的是 kEpsGeom 吗？\n";
    std::cout << (all_pass ? "\nAll numeric checks passed.\n"
                           : "\nSome checks failed. 检查 AI 代码。\n");
    return all_pass ? 0 : 1;
}
