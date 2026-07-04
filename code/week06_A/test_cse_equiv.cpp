// test_cse_equiv.cpp — 符号→C++：CSE 前后"结果一致 + 更快"的验证闭环
// 用法：把 SymPy ccode 的两版输出粘进 PASTE ZONE：
//   hessian_naive = ccode(H)          （不上 cse，子表达式重复）
//   hessian_cse   = ccode(cse(H))     （上 cse，公共子表达式只算一次）
//
// 金标准两条：
//   ① 两版结果在随机输入下逐点一致（< 1e-12）——抓 CSE 重排 / 依赖排序 bug；
//   ② 都与 codegen_ref.h 的手写 hessian_ref 一致（< 1e-9）——抓"两版同错"；
//   外加计时：cse 版应更快（少算 sqrt/pow）。

#include "codegen_ref.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

// =================================================================
// >>>>>>>>>>  PASTE AI-GENERATED hessian_naive / hessian_cse BELOW  <<<<<<<<<<
// =================================================================

// 不上 CSE 的版本（ccode(H) 直出，每个输出项各自重算 sqrt / pow）
void hessian_naive(const double* p, double* H) {
    // TODO: 粘贴 SymPy ccode(H)（未经 cse）
    (void)p; H[0] = H[1] = H[2] = H[3] = 0.0;  // 占位
}

// 上 CSE 的版本（ccode(cse(H))，公共子表达式提成 const double 只算一次）
void hessian_cse(const double* p, double* H) {
    // TODO: 粘贴 SymPy ccode(cse(H))
    (void)p; H[0] = H[1] = H[2] = H[3] = 0.0;  // 占位
}

// =================================================================
// >>>>>>>>>>             END OF PASTE ZONE              <<<<<<<<<<
// =================================================================

int main() {
    enable_utf8_console();
    bool all_pass = true;

    // 固定种子 LCG 产随机点（避开锚点，d 不至于为 0），保证可复现
    unsigned s = 20260601u;
    auto rnd = [&s](double lo, double hi) {
        s = 1664525u * s + 1013904223u;
        return lo + (hi - lo) * ((s >> 8) / static_cast<double>(1u << 24));
    };
    std::vector<std::array<double, 2>> pts;
    for (int i = 0; i < 200; ++i) pts.push_back({rnd(-3, 7), rnd(-3, 6)});

    auto maxdiff = [](const double* A, const double* B) {
        double m = 0;
        for (int k = 0; k < 4; ++k) m = std::max(m, std::fabs(A[k] - B[k]));
        return m;
    };

    // ---- Test 1: naive vs 手写参考（< 1e-9），确认 ccode 本身对 ----
    std::cout << "=== Test 1: hessian_naive vs hessian_ref ===\n";
    {
        double worst = 0;
        for (const auto& p : pts) {
            double Hn[4], Hr[4];
            hessian_naive(p.data(), Hn);
            cg::hessian_ref(p.data(), Hr);
            worst = std::max(worst, maxdiff(Hn, Hr));
        }
        const bool ok = worst < 1e-9;
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << "max diff over 200 pts = " << worst << "\n";
        if (!ok) all_pass = false;
    }

    // ---- Test 2: cse vs naive 逐点一致（< 1e-12）——CSE 的核心承诺 ----
    std::cout << "\n=== Test 2: hessian_cse == hessian_naive (CSE preserves result) ===\n";
    {
        double worst = 0;
        for (const auto& p : pts) {
            double Hc[4], Hn[4];
            hessian_cse(p.data(), Hc);
            hessian_naive(p.data(), Hn);
            worst = std::max(worst, maxdiff(Hc, Hn));
        }
        const bool ok = worst < 1e-12;
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << "max diff over 200 pts = " << worst << "\n";
        std::cout << "  说明：CSE 只是把重复子表达式提出来，绝不能改变数值——差异应到机器精度。\n";
        if (!ok) all_pass = false;
    }

    // ---- Test 3: 计时——cse 版应更快（少算 sqrt/pow） ----
    std::cout << "\n=== Test 3: timing (CSE should be faster) ===\n";
    {
        const int REPS = 2000000;
        double H[4];
        const double p0[2] = {1.3, 0.7};
        // 预热
        for (int i = 0; i < 1000; ++i) { hessian_naive(p0, H); hessian_cse(p0, H); }
        const double t_naive = cg::time_ms([&] { hessian_naive(p0, H); }, REPS);
        const double t_cse   = cg::time_ms([&] { hessian_cse(p0, H); }, REPS);
        std::cout << "  naive = " << t_naive << " ms,  cse = " << t_cse << " ms"
                  << "   speedup = " << (t_naive / t_cse) << "x\n";
        const bool ok = t_cse < t_naive;   // cse 不该比 naive 慢
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << "cse < naive\n";
        std::cout << "  说明：naive 版每个输出项重算 sqrt/pow，cse 版每个 d_i 只算一次——加速主要来自省掉的超越函数。\n";
        if (!ok) all_pass = false;
    }

    // -------- SVG：沿 y=1.0 扫 x，画 naive/cse 两版 Hxx(x)，应重合成一条 --------
    const std::string svg_name = "cse_equiv_output.svg";
    {
        std::vector<double> xs, yn, yc;
        double ymin = 1e300, ymax = -1e300;
        for (int i = 0; i <= 300; ++i) {
            const double x = -2.0 + 8.0 * i / 300.0;
            const double p[2] = {x, 1.0};
            double Hn[4], Hc[4];
            hessian_naive(p, Hn); hessian_cse(p, Hc);
            xs.push_back(x); yn.push_back(Hn[0]); yc.push_back(Hc[0]);
            ymin = std::min({ymin, Hn[0], Hc[0]}); ymax = std::max({ymax, Hn[0], Hc[0]});
        }
        if (ymax <= ymin) { ymin -= 1; ymax += 1; }
        ScanSVG svg(svg_name, -2.0, 6.0, ymin, ymax);
        svg.curve(xs, yn, "#2266ff", 3.0);   // naive：蓝粗
        svg.curve(xs, yc, "#ff5522", 1.2);   // cse：橙细（叠在蓝上，重合则只见一条镶边）
    }
    const auto abs_path = std::filesystem::absolute(svg_name);
    std::cout << "\n=== SVG ===\n生成: " << abs_path.string() << "\n";
    std::cout << "拖进浏览器：蓝(naive)粗线 + 橙(cse)细线，数值一致时橙线严丝合缝叠在蓝线上。\n";

    std::cout << (all_pass ? "\nAll numeric checks passed.\n"
                           : "\nSome checks failed. 检查 CSE 输出。\n");
    return all_pass ? 0 : 1;
}
