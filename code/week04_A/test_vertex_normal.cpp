// test_vertex_normal.cpp — 面积加权顶点法向的验证闭环
// 用法：把 AI 生成的 vertex_normal 函数粘到 PASTE ZONE 之间，重新编译即可
//
// 金标准：倾斜平面上每个顶点的法向必须严格等于解析法向 normalize(-a,-b,1)
//          —— 平面网格的面积加权法向没有离散化误差，是"由答案构造输入"的硬对照。

#include "mesh.h"

#include <cmath>
#include <iostream>
#include <string>

// =================================================================
// >>>>>>>>>>  PASTE AI-GENERATED vertex_normal BELOW  <<<<<<<<<<
// =================================================================

// 顶点 v 的单位法向：把 v 的所有关联三角形的"面积加权法向"求和后归一化。
// 要求：零面积（退化）三角形不得产生 NaN；高度场应返回朝上（z>0）的法向。
V3 vertex_normal(const Mesh& m, int v) {
    // TODO: 在这里粘贴 / 实现面积加权顶点法向
    //   提示：遍历 m.incident_tris(v)，累加 m.tri_area_normal(t)，最后 normalized(...)
    (void)m; (void)v;
    return V3{0, 0, 0};  // 占位：未实现
}

// =================================================================
// >>>>>>>>>>             END OF PASTE ZONE              <<<<<<<<<<
// =================================================================

// 独立验证路径：高度场 z=f(x,y) 的解析法向 = normalize(-fx, -fy, 1)
static V3 analytic_normal(double fx, double fy) {
    return normalized(V3{-fx, -fy, 1.0});
}

int main() {
    enable_utf8_console();

    bool all_pass = true;
    auto check = [&](const std::string& name, const V3& a, const V3& b, double tol) {
        const double e = norm(a - b);
        const bool ok = e < tol;
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << name
                  << "  got=(" << a.x << "," << a.y << "," << a.z << ")"
                  << "  expect=(" << b.x << "," << b.y << "," << b.z << ")"
                  << "  err=" << e << "\n";
        if (!ok) all_pass = false;
    };

    // ---- Test 1: 倾斜平面（精确金标准，tol 1e-12） ----
    // z = a*x + b*y + c，所有三角形共面 → 面积加权法向无离散化误差
    std::cout << "=== Test 1: Tilted plane (exact analytic normal) ===\n";
    {
        const double a = 0.3, b = -0.2, c = 1.0;
        Mesh m = make_heightfield(9, 9, 1.0, 1.0,
                                  [&](double x, double y) { return a * x + b * y + c; });
        const V3 ref = analytic_normal(a, b);
        for (int j = 1; j < m.ny - 1; ++j)
            for (int i = 1; i < m.nx - 1; ++i)
                check("v(" + std::to_string(i) + "," + std::to_string(j) + ")",
                      vertex_normal(m, m.vid(i, j)), ref, 1e-12);
    }

    // ---- Test 2: 曲面（抛物面）—— 观察离散化误差（tol 5e-2） ----
    std::cout << "\n=== Test 2: Paraboloid (discrete vs analytic, loose tol) ===\n";
    {
        const double k = 0.05, cx = 4.0, cy = 4.0;
        Mesh m = make_heightfield(9, 9, 1.0, 1.0, [&](double x, double y) {
            return k * ((x - cx) * (x - cx) + (y - cy) * (y - cy));
        });
        for (int j = 2; j < m.ny - 2; ++j)
            for (int i = 2; i < m.nx - 2; ++i) {
                const double x = i * 1.0, y = j * 1.0;
                const V3 ref = analytic_normal(2 * k * (x - cx), 2 * k * (y - cy));
                check("v(" + std::to_string(i) + "," + std::to_string(j) + ")",
                      vertex_normal(m, m.vid(i, j)), ref, 5e-2);
            }
        std::cout << "  说明：曲面上离散法向与解析法向有 O(h) 偏差，这里只要求量级正确。\n";
    }

    // ---- Test 3: 退化三角形（零面积）不得返回 NaN（观察题 + 断言） ----
    std::cout << "\n=== Test 3: Degenerate zero-area triangle (no NaN) ===\n";
    {
        Mesh m = make_heightfield(3, 3, 1.0, 1.0, [](double, double) { return 0.0; });
        // 把中心顶点塌到一个邻居上，使若干关联三角形零面积
        m.pos[m.vid(1, 1)] = m.pos[m.vid(0, 1)];
        const V3 n = vertex_normal(m, m.vid(1, 1));
        const bool finite = std::isfinite(n.x) && std::isfinite(n.y) && std::isfinite(n.z);
        std::cout << (finite ? "  [PASS] " : "  [FAIL] ")
                  << "center normal finite = (" << n.x << "," << n.y << "," << n.z << ")\n";
        std::cout << "  问题：AI 的实现是跳过零面积三角形，还是让它污染成 NaN / 0-除？\n";
        if (!finite) all_pass = false;
    }

    // ---- Test 4: 朝向（高度场法向应朝上 z>0） ----
    std::cout << "\n=== Test 4: Orientation (heightfield normal points up) ===\n";
    {
        Mesh m = make_heightfield(5, 5, 1.0, 1.0,
                                  [](double x, double y) { return 0.1 * x - 0.05 * y; });
        const V3 n = vertex_normal(m, m.vid(2, 2));
        const bool up = n.z > 0.0;
        std::cout << (up ? "  [PASS] " : "  [FAIL] ") << "normal.z > 0  (z=" << n.z << ")\n";
        std::cout << "  说明：绕序 (v00,v10,v11)/(v00,v11,v01) 已保证朝上；若翻了说明 AI 反了 cross 参数序。\n";
        if (!up) all_pass = false;
    }

    std::cout << (all_pass ? "\nAll numeric checks passed.\n"
                           : "\nSome checks failed. 检查 AI 代码。\n");
    return all_pass ? 0 : 1;
}
