// energy.h — Week 5 minimal lab utilities（SymPy 梯度 × C++ 有限差分验证）
// 公共头：一个几何能量 E(x,y) + 中心差分梯度/Hessian 工具 + 能量场 SVG writer
// 整个第 5 周不需要修改本文件（PASTE ZONE 只在 test_*.cpp 里）
//
// 能量模型：一个自由点 p=(x,y) 被 3 根弹簧连到 3 个固定锚点
//   E(p) = Σ_i ½·k_i·(d_i − L_i)²,   d_i = ‖p − a_i‖
// —— 变量只有 2 个（有限差分廉价、参考可信），却含 sqrt/链式求导，
//    最容易暴露 AI 手推时"漏负号 / 漏 1/d_i"的错误。第 6 周的 CSE 也复用这个能量。
#pragma once

#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

// -------------------------------------------------------------------
// 固定常量：3 个锚点、静止长度、劲度（写死在头里，让生成代码签名保持 (x,y) → out）
// -------------------------------------------------------------------
namespace ge {

constexpr int N_ANCHOR = 3;
constexpr double AX[N_ANCHOR]   = {0.0, 4.0, 2.0};   // 锚点 x
constexpr double AY[N_ANCHOR]   = {0.0, 0.0, 3.0};   // 锚点 y
constexpr double REST[N_ANCHOR] = {2.0, 2.0, 2.0};   // 静止长度 L_i
constexpr double STIF[N_ANCHOR] = {1.0, 1.5, 0.8};   // 劲度 k_i

// 能量 E(x,y)
inline double energy(const double* p) {
    double E = 0.0;
    for (int i = 0; i < N_ANCHOR; ++i) {
        const double u = p[0] - AX[i];
        const double v = p[1] - AY[i];
        const double d = std::sqrt(u * u + v * v);
        const double s = d - REST[i];
        E += 0.5 * STIF[i] * s * s;
    }
    return E;
}

// 中心差分梯度：g_k ≈ (E(p+h e_k) − E(p−h e_k)) / (2h)
//   —— 独立于任何解析公式，是验证 compute_gradient 的金标准
inline void fd_gradient(const double* p, double* g, double h = 1e-6) {
    for (int k = 0; k < 2; ++k) {
        double pp[2] = {p[0], p[1]}, pm[2] = {p[0], p[1]};
        pp[k] += h; pm[k] -= h;
        g[k] = (energy(pp) - energy(pm)) / (2.0 * h);
    }
}

// 用"解析梯度"再做一次中心差分，得到参考 Hessian（比二阶差分能量稳得多）
//   H[i][j] ≈ (grad_i(p+h e_j) − grad_i(p−h e_j)) / (2h)，行主序存 H_out[2*i+j]
inline void fd_hessian_from_grad(const std::function<void(const double*, double*)>& grad,
                                 const double* p, double* H_out, double h = 1e-5) {
    for (int j = 0; j < 2; ++j) {
        double pp[2] = {p[0], p[1]}, pm[2] = {p[0], p[1]};
        pp[j] += h; pm[j] -= h;
        double gp[2], gm[2];
        grad(pp, gp); grad(pm, gm);
        for (int i = 0; i < 2; ++i) H_out[2 * i + j] = (gp[i] - gm[i]) / (2.0 * h);
    }
}

}  // namespace ge

// -------------------------------------------------------------------
// 能量场 SVG：热力图（按 E 染色）+ 负梯度箭头（下坡方向）
//   梯度用传入的回调（就是你 PASTE 的 compute_gradient）——占位桩时箭头消失，
//   正确实现时箭头应汇向能量盆地，是数值判定之外的第二条验证通道。
// -------------------------------------------------------------------
inline std::string energy_heat_color(double t) {
    t = t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t);
    double r, g, b;
    if (t < 0.5) { const double u = t / 0.5; r = 0.15 + 0.1 * u; g = 0.2 + 0.6 * u; b = 0.6 - 0.2 * u; }
    else         { const double u = (t - 0.5) / 0.5; r = 0.25 + 0.7 * u; g = 0.8 - 0.6 * u; b = 0.4 - 0.35 * u; }
    auto ch = [](double v) { return static_cast<int>((v < 0 ? 0 : v > 1 ? 1 : v) * 255.0 + 0.5); };
    char buf[16];
    std::snprintf(buf, sizeof(buf), "#%02x%02x%02x", ch(r), ch(g), ch(b));
    return std::string(buf);
}

class EnergySVG {
    std::ofstream f;
public:
    EnergySVG(const std::string& path, double x0, double y0, double x1, double y1,
              const std::function<void(const double*, double*)>& gradient) {
        f.open(path);
        const double w = x1 - x0, h = y1 - y0;
        f << "<svg xmlns='http://www.w3.org/2000/svg' viewBox='"
          << x0 << " " << -y1 << " " << w << " " << h
          << "' width='720' height='" << static_cast<int>(720 * h / w)
          << "' style='background:white'>\n<g transform='matrix(1 0 0 -1 0 0)'>\n";

        // 找能量范围用于染色
        const int NGX = 48, NGY = 40;
        double emin = 1e300, emax = -1e300;
        for (int j = 0; j <= NGY; ++j)
            for (int i = 0; i <= NGX; ++i) {
                double p[2] = {x0 + w * i / NGX, y0 + h * j / NGY};
                const double e = ge::energy(p);
                emin = std::min(emin, e); emax = std::max(emax, e);
            }
        // 热力图方块
        for (int j = 0; j < NGY; ++j)
            for (int i = 0; i < NGX; ++i) {
                double p[2] = {x0 + w * (i + 0.5) / NGX, y0 + h * (j + 0.5) / NGY};
                const double t = (ge::energy(p) - emin) / (emax - emin);
                f << "<rect x='" << (x0 + w * i / NGX) << "' y='" << (y0 + h * j / NGY)
                  << "' width='" << (w / NGX) << "' height='" << (h / NGY)
                  << "' fill='" << energy_heat_color(t) << "'/>\n";
            }
        // 负梯度箭头（下坡）
        const int AX_ = 14, AY_ = 12;
        for (int j = 1; j < AY_; ++j)
            for (int i = 1; i < AX_; ++i) {
                double p[2] = {x0 + w * i / AX_, y0 + h * j / AY_};
                double g[2] = {0, 0};
                gradient(p, g);
                const double gn = std::sqrt(g[0] * g[0] + g[1] * g[1]);
                if (gn < 1e-9) continue;
                const double sc = 0.22 * std::min(w, h) / (1.0 + gn);  // 长度随梯度饱和
                const double ex = p[0] - g[0] / gn * sc, ey = p[1] - g[1] / gn * sc;
                f << "<line x1='" << p[0] << "' y1='" << p[1] << "' x2='" << ex << "' y2='" << ey
                  << "' stroke='#111' stroke-width='0.015'/>\n";
                f << "<circle cx='" << ex << "' cy='" << ey << "' r='0.03' fill='#111'/>\n";
            }
        // 锚点
        for (int i = 0; i < ge::N_ANCHOR; ++i)
            f << "<circle cx='" << ge::AX[i] << "' cy='" << ge::AY[i]
              << "' r='0.08' fill='white' stroke='#c00' stroke-width='0.04'/>\n";
    }
    ~EnergySVG() { f << "</g></svg>\n"; }
};

// -------------------------------------------------------------------
// Windows 控制台 UTF-8 输出
// -------------------------------------------------------------------
#ifdef _WIN32
#include <windows.h>
inline void enable_utf8_console() { SetConsoleOutputCP(CP_UTF8); }
#else
inline void enable_utf8_console() {}
#endif
