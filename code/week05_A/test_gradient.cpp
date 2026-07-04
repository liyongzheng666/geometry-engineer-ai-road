// test_gradient.cpp — SymPy 生成梯度的 C++ 有限差分验证闭环
// 用法：把 SymPy 脚本 ccode() 吐出的 compute_gradient 粘到 PASTE ZONE，重新编译即可
//
// 范式（坚持第 1 周的 C++ 数值金标准）：
//   AI 写 SymPy 脚本 → sympy.ccode() 生成 compute_gradient 的 C++ → 粘进来
//   → 用 energy.h 里手写的**中心差分** fd_gradient 双算对照。
// 有限差分和解析式走完全不同的数值路径，任何"漏负号 / 漏 1/d_i / 链式求导错"
// 都会让相对误差从 ~1e-11 暴涨到 O(1)，一眼现形。

#include "energy.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

// =================================================================
// >>>>>>>>>>  PASTE AI-GENERATED compute_gradient BELOW  <<<<<<<<<<
// =================================================================

// 解析梯度：g[0]=∂E/∂x, g[1]=∂E/∂y（SymPy: diff(E,x)/diff(E,y) → cse → ccode）
//   参考数学形式：g = Σ_i k_i·(1 − L_i/d_i)·(p − a_i)
void compute_gradient(const double* p, double* g) {
    // TODO: 在这里粘贴 SymPy 生成的解析梯度 C++
    (void)p;
    g[0] = 0.0; g[1] = 0.0;  // 占位：未实现
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

    // ---- Test 1: 与中心差分梯度双算对照（max 相对误差 < 1e-6） ----
    std::cout << "=== Test 1: analytic gradient vs central finite difference ===\n";
    const double PTS[][2] = {{1.5, 1.0}, {3.0, 2.0}, {0.5, 2.5}, {2.0, 1.0}, {3.5, 0.5}};
    for (const auto& p : PTS) {
        double ga[2], gf[2];
        compute_gradient(p, ga);
        ge::fd_gradient(p, gf);
        const double e = std::max(rel_err(ga[0], gf[0]), rel_err(ga[1], gf[1]));
        const bool ok = e < 1e-6;
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ")
                  << "p=(" << p[0] << "," << p[1] << ")"
                  << "  analytic=(" << ga[0] << "," << ga[1] << ")"
                  << "  fd=(" << gf[0] << "," << gf[1] << ")  relerr=" << e << "\n";
        if (!ok) all_pass = false;
    }

    // ---- Test 2: 下坡方向（沿 −grad 能量必须下降，专抓全局符号翻转） ----
    std::cout << "\n=== Test 2: -gradient points downhill (catches sign flip) ===\n";
    {
        const double p[2] = {1.5, 1.0};
        double g[2];
        compute_gradient(p, g);
        const double t = 1e-3;
        const double pm[2] = {p[0] - t * g[0], p[1] - t * g[1]};
        const bool ok = ge::energy(pm) < ge::energy(p);
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ")
                  << "E(p - t·grad)=" << ge::energy(pm) << " < E(p)=" << ge::energy(p) << "\n";
        std::cout << "  说明：若 AI 把梯度整体写反了号，此项立刻 FAIL（Test 1 的相对误差也会爆）。\n";
        if (!ok) all_pass = false;
    }

    // ---- Test 3: 观察题 —— 靠近锚点的奇异（d_i → 0，L_i/d_i 爆） ----
    std::cout << "\n=== Test 3: singularity near an anchor (observation) ===\n";
    {
        const double p[2] = {ge::AX[0] + 1e-7, ge::AY[0] + 1e-7};  // 几乎压在锚点 0 上
        double g[2];
        compute_gradient(p, g);
        std::cout << "  grad @near-anchor = (" << g[0] << "," << g[1] << ")\n";
        std::cout << "  问题：SymPy 生成的式子里 1/d_i 在 d_i→0 时发散——AI 的脚本有没有\n"
                     "        在生成前声明这个奇异、或在 C++ 里对 d_i 加 epsilon 保护？\n";
    }

    // -------- SVG 可视化（能量热力图 + 负梯度箭头） --------
    const std::string svg_name = "gradient_output.svg";
    { EnergySVG svg(svg_name, -1.0, -1.0, 5.0, 4.0,
                    [](const double* p, double* g) { compute_gradient(p, g); }); }
    const auto abs_path = std::filesystem::absolute(svg_name);
    std::cout << "\n=== SVG ===\n生成: " << abs_path.string() << "\n";
    std::cout << "拖进浏览器：黑色箭头 = −梯度（下坡）。正确实现时箭头应汇向能量盆地（暖色低谷）。\n";

    std::cout << (all_pass ? "\nAll numeric checks passed.\n"
                           : "\nSome checks failed. 检查 AI / SymPy 输出。\n");
    return all_pass ? 0 : 1;
}
