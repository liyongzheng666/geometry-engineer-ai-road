// test_hessian.cpp — SymPy 生成 Hessian 的 C++ 有限差分验证闭环
// 用法：把 SymPy 脚本 sympy.hessian(...) → ccode() 吐出的 compute_hessian 粘到 PASTE ZONE
//
// 金标准：对**已验证的解析梯度**再做一次中心差分（比二阶差分能量稳），得到参考 Hessian；
//         外加对称性 H == Hᵀ（解析 Hessian 必须严格对称）。
// compute_gradient 在此为"给定基础设施"（上一支 test_gradient 已验证过它），
// 把它的签名贴进 prompt，让 AI 的 SymPy 脚本只负责 Hessian。

#include "energy.h"

#include <algorithm>
#include <cmath>
#include <iostream>

// -------- 给定基础设施：已验证的解析梯度（供 FD 参考 Hessian 使用） --------
// [[maybe_unused]]：占位桩阶段还没间接用到，先压警告
[[maybe_unused]] static void grad_ref(const double* p, double* g) {
    g[0] = 0.0; g[1] = 0.0;
    for (int i = 0; i < ge::N_ANCHOR; ++i) {
        const double u = p[0] - ge::AX[i];
        const double v = p[1] - ge::AY[i];
        const double d = std::sqrt(u * u + v * v);
        const double f = ge::STIF[i] * (1.0 - ge::REST[i] / d);
        g[0] += f * u; g[1] += f * v;
    }
}

// =================================================================
// >>>>>>>>>>  PASTE AI-GENERATED compute_hessian BELOW  <<<<<<<<<<
// =================================================================

// 解析 Hessian，行主序 2×2：H_out[0]=Hxx, H_out[1]=Hxy, H_out[2]=Hyx, H_out[3]=Hyy
//   参考数学形式（对每个锚点 i 求和，u=x−ax_i, v=y−ay_i, d=√(u²+v²)）：
//     Hxx = Σ k_i·(1 − L_i/d + L_i·u²/d³)
//     Hyy = Σ k_i·(1 − L_i/d + L_i·v²/d³)
//     Hxy = Σ k_i·(L_i·u·v / d³)
void compute_hessian(const double* p, double* H_out) {
    // TODO: 在这里粘贴 SymPy 生成的解析 Hessian C++
    (void)p;
    H_out[0] = H_out[1] = H_out[2] = H_out[3] = 0.0;  // 占位：未实现
}

// =================================================================
// >>>>>>>>>>             END OF PASTE ZONE              <<<<<<<<<<
// =================================================================

int main() {
    enable_utf8_console();
    bool all_pass = true;

    auto rel_err = [](double a, double b) {
        const double d = std::fabs(a - b);
        const double s = std::max(1.0, std::max(std::fabs(a), std::fabs(b)));
        return d / s;
    };

    const double PTS[][2] = {{1.5, 1.0}, {3.0, 2.0}, {0.5, 2.5}, {3.5, 0.5}};

    // ---- Test 1: 与"解析梯度的中心差分"双算对照（rel 误差 < 1e-5） ----
    std::cout << "=== Test 1: analytic Hessian vs finite-difference of gradient ===\n";
    for (const auto& p : PTS) {
        double Ha[4], Hf[4];
        compute_hessian(p, Ha);
        ge::fd_hessian_from_grad(grad_ref, p, Hf);
        double e = 0;
        for (int k = 0; k < 4; ++k) e = std::max(e, rel_err(Ha[k], Hf[k]));
        const bool ok = e < 1e-5;
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ")
                  << "p=(" << p[0] << "," << p[1] << ")  Hxx=" << Ha[0] << " Hxy=" << Ha[1]
                  << " Hyy=" << Ha[3] << "  relerr=" << e << "\n";
        if (!ok) all_pass = false;
    }

    // ---- Test 2: 对称性 Hxy == Hyx（解析 Hessian 必须严格对称） ----
    std::cout << "\n=== Test 2: symmetry H == H^T ===\n";
    for (const auto& p : PTS) {
        double H[4];
        compute_hessian(p, H);
        const double asym = std::fabs(H[1] - H[2]);
        const bool ok = asym < 1e-12;
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ")
                  << "p=(" << p[0] << "," << p[1] << ")  |Hxy-Hyx|=" << asym << "\n";
        if (!ok) all_pass = false;
    }

    // ---- Test 3: 观察题 —— 局部凸性（对称 2×2 的 det / trace） ----
    std::cout << "\n=== Test 3: local convexity (observation) ===\n";
    for (const auto& p : PTS) {
        double H[4];
        compute_hessian(p, H);
        const double det = H[0] * H[3] - H[1] * H[2];
        const double tr = H[0] + H[3];
        std::cout << "  p=(" << p[0] << "," << p[1] << ")  det=" << det << "  trace=" << tr
                  << (det > 0 && tr > 0 ? "  → SPD(局部凸)" : "  → 非正定(远离极小或鞍点)") << "\n";
    }
    std::cout << "  问题：牛顿法要用 H 求解，非正定处怎么办（加阻尼 / 特征值截断）？这是 AI 给不了的决策。\n";

    std::cout << (all_pass ? "\nAll numeric checks passed.\n"
                           : "\nSome checks failed. 检查 AI / SymPy 输出。\n");
    return all_pass ? 0 : 1;
}
