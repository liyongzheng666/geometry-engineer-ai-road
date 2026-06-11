// test_ray_triangle.cpp — Möller-Trumbore 射线-三角形相交的验证闭环
// 用法：把 AI 生成的 ray_triangle 函数粘到 PASTE ZONE 之间，重新编译即可
//
// 第 2 周周五的玩法是反着用 AI：先让它讲清 Möller-Trumbore 的几何推导
// （为什么不用显式平面方程、行列式为零意味着什么），读懂后再让它写代码。
// 独立验证路径：传统两段法（平面求交 + 重心坐标），与 MT 完全不同的计算顺序。

#include "geo.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

// =================================================================
// 基础设施：三维向量运算（geo.h 的 Point3D 没带，PASTE ZONE 内允许调用）
// =================================================================

static double dot3(const Point3D& a, const Point3D& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
static Point3D cross3(const Point3D& a, const Point3D& b) {
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}
static double norm3(const Point3D& a) { return std::sqrt(dot3(a, a)); }

// =================================================================
// >>>>>>>>>>  PASTE AI-GENERATED ray_triangle BELOW  <<<<<<<<<<
// =================================================================

// 约定（写进你的 prompt）：
//   交点 = orig + t * dir（dir 不要求归一化，t 以 dir 长度为单位）
//   重心坐标：交点 = (1-u-v)*A + u*B + v*C
//   只算 t >= 0 的正向命中；平行 / 零面积三角形必须安全返回 miss（不得 NaN）
struct RayHit {
    bool   hit = false;
    double t   = 0.0;
    double u   = 0.0;
    double v   = 0.0;
};

RayHit ray_triangle(const Point3D& orig, const Point3D& dir,
                    const Point3D& A, const Point3D& B, const Point3D& C) {
    // TODO: 在这里粘贴 / 实现 Möller-Trumbore
    // 提示（建议先让 AI 讲清推导再写码，prompt 要点）：
    //   1. 为什么不显式构造平面方程也能判定相交
    //   2. 行列式 det ≈ 0 意味着什么几何状态（平行？退化？）
    //   3. 退化输入（零面积三角形、三点全重合）下 eps 怎么取才不会漏判
    //      （小心：相对 eps 在边向量为零时也会归零）
    //   4. t < 0（三角形在身后）必须返回 miss
    (void)orig; (void)dir; (void)A; (void)B; (void)C;
    return RayHit{};  // 占位：未实现
}

// =================================================================
// >>>>>>>>>>             END OF PASTE ZONE              <<<<<<<<<<
// =================================================================

// 独立验证路径：传统两段法——先解平面交点，再解 2×2 方程拿重心坐标
static RayHit plane_bary_ref(const Point3D& orig, const Point3D& dir,
                             const Point3D& A, const Point3D& B, const Point3D& C) {
    RayHit h;
    const Point3D e1 = B - A, e2 = C - A;
    const Point3D n = cross3(e1, e2);
    const double n_len = norm3(n);
    if (n_len < 1e-14) return h;                       // 零面积三角形
    const double denom = dot3(n, dir);
    if (std::abs(denom) < 1e-12 * n_len * norm3(dir)) return h;   // 平行
    const double t = dot3(n, A - orig) / denom;
    if (t < 0.0) return h;
    const Point3D q = (orig + dir * t) - A;
    const double d00 = dot3(e1, e1), d01 = dot3(e1, e2), d11 = dot3(e2, e2);
    const double d20 = dot3(q, e1),  d21 = dot3(q, e2);
    const double det = d00 * d11 - d01 * d01;
    const double u = (d11 * d20 - d01 * d21) / det;
    const double v = (d00 * d21 - d01 * d20) / det;
    if (u < -1e-12 || v < -1e-12 || u + v > 1.0 + 1e-12) return h;
    h.hit = true; h.t = t; h.u = u; h.v = v;
    return h;
}

int main() {
    enable_utf8_console();

    // 主测三角形（z=0 平面，重心坐标可手算：u = x/2，v = y/2）
    const Point3D A{0, 0, 0}, B{2, 0, 0}, C{0, 2, 0};

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

    std::cout << "=== Test 1: 正命中（手算值：t=1, u=0.25, v=0.25） ===\n";
    {
        const RayHit h = ray_triangle({0.5, 0.5, 1}, {0, 0, -1}, A, B, C);
        check_true("hit", h.hit);
        check("t", h.t, 1.0);
        check("u", h.u, 0.25);
        check("v", h.v, 0.25);
    }

    std::cout << "\n=== Test 2: 斜射线 vs 参照法（dir 故意不归一化 + 重构校验） ===\n";
    {
        struct Case { Point3D orig, dir; };
        const std::vector<Case> cases = {
            {{-1, -1, 2},     {1.2, 1.1, -2}},    // t=1, (u,v)=(0.1,0.05)
            {{2, 2, 1},       {-1.2, -1.4, -1}},  // t=1, (u,v)=(0.4,0.3)
            {{0.5, 0.25, 3},  {0, 0.05, -1}},     // t=3, (u,v)=(0.25,0.2)
            {{0, 0, 1},       {2, 2, -0.5}},      // 平面交点 (4,4,0) → miss
        };
        for (size_t k = 0; k < cases.size(); ++k) {
            const RayHit got = ray_triangle(cases[k].orig, cases[k].dir, A, B, C);
            const RayHit ref = plane_bary_ref(cases[k].orig, cases[k].dir, A, B, C);
            const std::string tag = "case" + std::to_string(k);
            check_true(tag + " hit==ref", got.hit == ref.hit);
            if (got.hit && ref.hit) {
                check(tag + " t", got.t, ref.t);
                check(tag + " u", got.u, ref.u);
                check(tag + " v", got.v, ref.v);
                // 重构校验：orig + t·dir 必须等于 (1-u-v)A + uB + vC
                const Point3D hit_pt = cases[k].orig + cases[k].dir * got.t;
                const Point3D bary_pt = A * (1.0 - got.u - got.v) + B * got.u + C * got.v;
                check(tag + " 重构误差", norm3(hit_pt - bary_pt), 0.0);
            }
        }
    }

    std::cout << "\n=== Test 3: 未命中（平面交点在三角形外） ===\n";
    check_true("miss", !ray_triangle({3, 3, 1}, {0, 0, -1}, A, B, C).hit);

    std::cout << "\n=== Test 4: 负 t（三角形在射线身后） ===\n";
    check_true("behind", !ray_triangle({0.5, 0.5, 1}, {0, 0, 1}, A, B, C).hit);

    std::cout << "\n=== Test 5: 平行射线（退化） ===\n";
    {
        const RayHit off = ray_triangle({0.5, 0.5, 1}, {1, 0, 0}, A, B, C);
        check_true("离面平行 → miss 且无 NaN",
                   !off.hit && std::isfinite(off.t)
                            && std::isfinite(off.u) && std::isfinite(off.v));
        const RayHit cop = ray_triangle({-1, 0.5, 0}, {1, 0, 0}, A, B, C);
        std::cout << "  共面射线（几何上穿过三角形）：hit="
                  << (cop.hit ? "true" : "false") << "\n";
        std::cout << "  观察：标准 MT 因 det≈0 报 miss，但几何上射线确实穿过三角形。\n";
        std::cout << "        AI 的注释里有没有写明这条约定？\n";
    }

    std::cout << "\n=== Test 6: 零面积三角形（退化） ===\n";
    {
        const RayHit col = ray_triangle({1, 0, 1}, {0, 0, -1},
                                        {0, 0, 0}, {1, 0, 0}, {2, 0, 0});  // 三点共线
        check_true("共线三角形 → miss 且无 NaN",
                   !col.hit && std::isfinite(col.t)
                            && std::isfinite(col.u) && std::isfinite(col.v));
        const Point3D Q{1, 1, 0};
        const RayHit dup = ray_triangle({1, 1, 1}, {0, 0, -1}, Q, Q, Q);   // 三点重合
        check_true("三点重合 → miss 且无 NaN",
                   !dup.hit && std::isfinite(dup.t)
                            && std::isfinite(dup.u) && std::isfinite(dup.v));
    }

    std::cout << "\n=== Test 7: 边/顶点命中（观察题：watertight 问题） ===\n";
    {
        const RayHit e1 = ray_triangle({1, 0, 1}, {0, 0, -1}, A, B, C);  // 边 AB 中点
        const RayHit v0 = ray_triangle({0, 0, 1}, {0, 0, -1}, A, B, C);  // 顶点 A
        const RayHit hy = ray_triangle({1, 1, 1}, {0, 0, -1}, A, B, C);  // 斜边中点
        std::cout << "  边AB中点: hit=" << e1.hit << " u=" << e1.u << " v=" << e1.v << "\n";
        std::cout << "  顶点A   : hit=" << v0.hit << " u=" << v0.u << " v=" << v0.v << "\n";
        std::cout << "  斜边中点: hit=" << hy.hit << " u=" << hy.u << " v=" << hy.v << "\n";
        std::cout << "  问题：AI 的边界比较用 u >= 0 还是 u > -eps？\n";
        std::cout << "        两个共享这条边的相邻三角形，会双命中还是漏命中（watertight）？\n";
    }

    std::cout << "\n=== Test 8: 背面入射（观察题：culling 约定） ===\n";
    {
        const RayHit bk = ray_triangle({0.5, 0.5, -1}, {0, 0, 1}, A, B, C);
        std::cout << "  从 -z 侧打向三角形背面: hit=" << (bk.hit ? "true" : "false");
        if (bk.hit) std::cout << "  t=" << bk.t;
        std::cout << "\n  问题：AI 默认单面（backface culling）还是双面？你的 prompt 里写了吗？\n";
    }

    // -------- SVG：俯视图（三角形在 z=0 平面） --------
    const std::string svg_name = "ray_triangle_output.svg";
    {
        SVG svg(svg_name, -2, -2, 5, 5);
        svg.polyline({A, B, C, A}, "#2266ff", 0.04);          // 三角形 - 蓝
        struct Case { Point3D orig, dir; };
        const std::vector<Case> rays = {
            {{0.5, 0.5, 1}, {0, 0, -1}},   {{-1, -1, 2}, {1.2, 1.1, -2}},
            {{2, 2, 1}, {-1.2, -1.4, -1}}, {{0.5, 0.25, 3}, {0, 0.05, -1}},
            {{0, 0, 1}, {2, 2, -0.5}},     {{3, 3, 1}, {0, 0, -1}},
            {{1, 0, 1}, {0, 0, -1}},       {{1, 1, 1}, {0, 0, -1}},
        };
        for (const auto& r : rays) {
            if (std::abs(r.dir.z) < 1e-12) continue;          // 与平面平行，画不出穿越点
            const double t = -r.orig.z / r.dir.z;             // 与 z=0 平面的交点
            if (t < 0) continue;
            const Point3D Q = r.orig + r.dir * t;
            svg.line(r.orig.x, r.orig.y, Q.x, Q.y, "#bbbbbb", 0.02);   // 射线投影
            const RayHit h = ray_triangle(r.orig, r.dir, A, B, C);
            svg.dots({Q}, h.hit ? "#4daf4a" : "#e41a1c", 0.08);        // 命中绿 / 未中红
        }
    }

    const auto abs_path = std::filesystem::absolute(svg_name);
    std::cout << "\n生成: " << abs_path.string() << "\n";
    std::cout << "可视判断：绿色点必须全部落在蓝色三角形内（含边界），红色点必须全部在外。\n";
    std::cout << (all_pass ? "\nAll numeric checks passed.\n"
                           : "\nSome checks failed. 检查 AI 代码。\n");
    return all_pass ? 0 : 1;
}
