// codegen_ref.h — Week 6 minimal lab utilities（符号 → 工业 C++：自动 CSE）
// 公共头：弹簧能量 Hessian 的"干净参考实现" + 计时器 + 扫描曲线 SVG writer
// 整个第 6 周不需要修改本文件（PASTE ZONE 只在 test_*.cpp 里）
//
// 复用第 5 周的能量：自由点 p=(x,y) 连 N 个锚点，E=Σ½k(‖p−a‖−L)²。
// 本周关心的不是"推导对不对"（第 5 周已验），而是"SymPy 的 ccode() 上不上 cse()
// 会不会改变数值结果、又快多少"。故 harness 提供一个手写的 hessian_ref 当正确性锚点。
#pragma once

#include <chrono>
#include <cmath>
#include <fstream>
#include <string>
#include <vector>

namespace cg {

// 用 5 个锚点：重复子表达式（每个 d_i / d_i³）更多，CSE 的收益更明显
constexpr int N_ANCHOR = 5;
constexpr double AX[N_ANCHOR]   = {0.0, 4.0, 2.0, -1.0, 5.0};
constexpr double AY[N_ANCHOR]   = {0.0, 0.0, 3.0,  2.0, 2.5};
constexpr double REST[N_ANCHOR] = {2.0, 2.0, 2.0,  1.5, 2.2};
constexpr double STIF[N_ANCHOR] = {1.0, 1.5, 0.8,  1.2, 0.9};

// 干净参考 Hessian（行主序 2×2：H[0]=Hxx, H[1]=Hxy, H[2]=Hyx, H[3]=Hyy）
//   每个锚点 d_i 只算一次——这是"人会写、cse() 也应逼近"的形态，作正确性金标准。
inline void hessian_ref(const double* p, double* H) {
    H[0] = H[1] = H[2] = H[3] = 0.0;
    for (int i = 0; i < N_ANCHOR; ++i) {
        const double u = p[0] - AX[i];
        const double v = p[1] - AY[i];
        const double d2 = u * u + v * v;
        const double d = std::sqrt(d2);
        const double d3 = d2 * d;
        const double k = STIF[i], L = REST[i];
        H[0] += k * (1.0 - L / d + L * u * u / d3);
        H[1] += k * (L * u * v / d3);
        H[3] += k * (1.0 - L / d + L * v * v / d3);
    }
    H[2] = H[1];
}

// 简易计时器：跑 reps 次，返回总耗时（毫秒）
template <class F>
inline double time_ms(F&& fn, int reps) {
    const auto t0 = std::chrono::high_resolution_clock::now();
    for (int r = 0; r < reps; ++r) fn();
    const auto t1 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

}  // namespace cg

// -------------------------------------------------------------------
// 扫描曲线 SVG：沿一条水平线扫 x，把两版实现算出的 Hxx(x) 各画一条折线。
//   两版数值一致 → 两条线严丝合缝重合成一条；CSE 有 bug → 岔开。
// -------------------------------------------------------------------
class ScanSVG {
    std::ofstream f;
    double x0, x1, y0, y1;
    double sx, sy;
    double px(double x) const { return (x - x0) * sx; }
    double py(double y) const { return (y - y0) * sy; }
public:
    ScanSVG(const std::string& path, double xmin, double xmax, double ymin, double ymax)
        : x0(xmin), x1(xmax), y0(ymin), y1(ymax) {
        const double W = 760, H = 380;
        sx = W / (x1 - x0); sy = H / (y1 - y0);
        f.open(path);
        f << "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 " << W << " " << H
          << "' width='" << W << "' height='" << H << "' style='background:white'>\n";
    }
    void curve(const std::vector<double>& xs, const std::vector<double>& ys,
               const std::string& color, double width) {
        f << "<polyline points='";
        for (size_t i = 0; i < xs.size(); ++i) f << px(xs[i]) << "," << py(ys[i]) << " ";
        f << "' fill='none' stroke='" << color << "' stroke-width='" << width << "'/>\n";
    }
    ~ScanSVG() { f << "</svg>\n"; }
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
