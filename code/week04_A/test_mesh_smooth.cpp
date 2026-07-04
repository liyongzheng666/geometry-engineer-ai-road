// test_mesh_smooth.cpp — 保特征网格平滑（双边法向滤波）的验证闭环
// 用法：把 AI 生成的 smooth_step 函数粘到 PASTE ZONE 之间，重新编译即可
//
// 本周的"圆性测试"等价物：构造两平面沿锐折痕相交（ridge）的高度场 + 固定种子噪声，
// 跑 N 次平滑后必须同时满足——
//   ① 平坦区到理论平面的 RMS 显著下降（去噪有效）；
//   ② 折痕不被抹平（feature preserved，区分双边滤波 vs 朴素拉普拉斯）；
//   ③ 开放边界顶点不漂移。
//
// vertex_normal / bilateral_weight 是"给定基础设施"（下方 static），你只写 smooth_step 外壳
// —— 把它们的签名贴进 prompt，让 AI 专注写"逐顶点沿法向的加权更新 + 边界/收敛处理"。

#include "mesh.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

// -------- 给定基础设施（不在 PASTE ZONE，供 smooth_step 调用） --------
// [[maybe_unused]]：初始占位桩还没调用它们，先压掉"未使用"警告；粘入实现后自然用到
[[maybe_unused]] static V3 mesh_vertex_normal(const Mesh& m, int v) {
    V3 sum{0, 0, 0};
    for (int t : m.incident_tris(v)) sum = sum + m.tri_area_normal(t);
    return normalized(sum);
}
[[maybe_unused]] static double bilateral_weight(double spatial_dist, double range_diff,
                                                double sigma_c, double sigma_s) {
    return std::exp(-(spatial_dist * spatial_dist) / (2.0 * sigma_c * sigma_c))
         * std::exp(-(range_diff * range_diff) / (2.0 * sigma_s * sigma_s));
}

// =================================================================
// >>>>>>>>>>  PASTE AI-GENERATED smooth_step BELOW  <<<<<<<<<<
// =================================================================

// 一次保特征平滑：对每个"内部"顶点 v，沿其法向 n 移动一个双边加权量
//   new_v = v + n · ( Σ_j w_j·h_j / Σ_j w_j )，  h_j = (neighbor_j - v)·n
//   w_j = bilateral_weight(||neighbor_j - v||, h_j, σc, σs)
// 约定：开放边界顶点保持不动；Σw≈0 时该顶点不动。
Mesh smooth_step(const Mesh& in, double sigma_c, double sigma_s) {
    // TODO: 在这里粘贴 / 实现双边法向滤波的单步更新
    (void)sigma_c; (void)sigma_s;
    return in;  // 占位：原样返回（未平滑）
}

// =================================================================
// >>>>>>>>>>             END OF PASTE ZONE              <<<<<<<<<<
// =================================================================

// 理论屋脊高度场：z = -slope·|x - xc|，折痕沿 y 方向在 x=xc 处
static double ridge_ideal(double x, double xc, double slope) {
    return -slope * std::fabs(x - xc);
}

int main() {
    enable_utf8_console();
    bool all_pass = true;

    // ---- 构造带折痕的高度场 + 固定种子噪声 ----
    const int NX = 21, NY = 11;
    const double DX = 0.5, DY = 0.5;
    const double XC = (NX - 1) * DX / 2.0;   // 折痕在中列
    const double SLOPE = 1.0;
    const double NOISE = 0.15;

    Mesh ideal = make_heightfield(NX, NY, DX, DY,
                                  [&](double x, double) { return ridge_ideal(x, XC, SLOPE); });
    Mesh noisy = ideal;
    add_deterministic_noise(noisy, NOISE, 2026u);

    // 双边参数：空间核约一个格距，range 核较小以"卡住"折痕（跨脊 h 跳变大）
    const double SIGMA_C = 0.6;
    const double SIGMA_S = 0.12;
    const int ITERS = 12;

    Mesh cur = noisy;
    for (int it = 0; it < ITERS; ++it) cur = smooth_step(cur, SIGMA_C, SIGMA_S);

    // 误差度量工具
    auto rms_flat = [&](const Mesh& m) {
        double s = 0; int cnt = 0;
        for (int j = 1; j < NY - 1; ++j)
            for (int i = 1; i < NX - 1; ++i) {
                const double x = i * DX;
                if (std::fabs(x - XC) < 1.5) continue;  // 排除折痕附近
                const double d = m.pos[m.vid(i, j)].z - ridge_ideal(x, XC, SLOPE);
                s += d * d; ++cnt;
            }
        return std::sqrt(s / cnt);
    };
    // 折痕"下沉量"：ridge 列理论 z=0，越正说明折痕被抹圆（拉普拉斯的通病）
    auto ridge_sink = [&](const Mesh& m) {
        const int ic = static_cast<int>(std::lround(XC / DX));
        double s = 0; int cnt = 0;
        for (int j = 1; j < NY - 1; ++j) {
            s += (0.0 - m.pos[m.vid(ic, j)].z);  // ideal - actual
            ++cnt;
        }
        return s / cnt;
    };

    const double rms0 = rms_flat(noisy), rms1 = rms_flat(cur);
    const double sink0 = ridge_sink(noisy), sink1 = ridge_sink(cur);

    // ---- Test 1: 去噪有效（平坦区 RMS 显著下降） ----
    std::cout << "=== Test 1: Denoising works (flat RMS drops) ===\n";
    std::cout << "  flat RMS  before=" << rms0 << "  after=" << rms1 << "\n";
    {
        const bool ok = rms1 < 0.6 * rms0;   // 显著下降；无操作占位桩 ratio=1.0 会失败
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << "after < 0.6 × before\n";
        std::cout << "  说明：双边滤波保特征的代价是留一层\"噪声地板\"（σs 小 → 平坦噪声也被当特征）；\n"
                     "        想把 RMS 压更低就得调大 σs，但那会抹圆折痕甚至发散——这正是本周要体会的取舍。\n";
        if (!ok) all_pass = false;
    }

    // ---- Test 2: 保特征（折痕不被抹圆） ----
    std::cout << "\n=== Test 2: Feature preserved (ridge not rounded) ===\n";
    std::cout << "  ridge sink  before=" << sink0 << "  after=" << sink1
              << "   (拉普拉斯会让 after 明显变大/正)\n";
    {
        const bool ok = std::fabs(sink1) < 0.12;  // 折痕平均下沉 < 0.12（slope·dx=0.5 作参照）
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << "|ridge sink| < 0.12\n";
        std::cout << "  说明：换成朴素拉普拉斯（去掉 range 核）此项必失败——这是双边滤波价值的客观证据。\n";
        if (!ok) all_pass = false;
    }

    // ---- Test 3: 开放边界不漂移 ----
    std::cout << "\n=== Test 3: Open boundary vertices do not move ===\n";
    {
        double maxmove = 0;
        for (int j = 0; j < NY; ++j)
            for (int i = 0; i < NX; ++i)
                if (noisy.is_boundary(i, j))
                    maxmove = std::max(maxmove, norm(cur.pos[cur.vid(i, j)] - noisy.pos[noisy.vid(i, j)]));
        const bool ok = maxmove < 1e-12;
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << "max boundary move = " << maxmove << "\n";
        if (!ok) all_pass = false;
    }

    // ---- Test 4: 观察题 —— 收敛性（连续两步的变化应递减） ----
    std::cout << "\n=== Test 4: Convergence trend (observation) ===\n";
    {
        Mesh a = noisy, b = smooth_step(a, SIGMA_C, SIGMA_S);
        auto delta = [](const Mesh& p, const Mesh& q) {
            double s = 0; for (size_t k = 0; k < p.pos.size(); ++k) s += norm(q.pos[k] - p.pos[k]); return s;
        };
        double d_prev = delta(a, b);
        std::cout << "  step  1 total move = " << d_prev << "\n";
        for (int it = 2; it <= 5; ++it) {
            Mesh c = smooth_step(b, SIGMA_C, SIGMA_S);
            const double d = delta(b, c);
            std::cout << "  step " << it << " total move = " << d << (d < d_prev ? "  ↓" : "  ↑") << "\n";
            b = c; d_prev = d;
        }
        std::cout << "  问题：AI 有没有做双终止条件（迭代次数 + 法向/位移阈值）？发散或不收敛该怎么兜底？\n";
    }

    // -------- SVG 可视化（before / after 俯视热力图并排） --------
    const std::string svg_name = "mesh_smooth_output.svg";
    {
        MeshSVG svg(svg_name, noisy, cur, -SLOPE * XC, 0.3);
    }
    const auto abs_path = std::filesystem::absolute(svg_name);
    std::cout << "\n=== SVG ===\n";
    std::cout << "生成: " << abs_path.string() << "\n";
    std::cout << "拖进浏览器：左=噪声网格，右=平滑后。应看到平坦区色块变匀、折痕那条色界仍然清晰。\n";

    std::cout << (all_pass ? "\nAll numeric checks passed.\n"
                           : "\nSome checks failed. 检查 AI 代码。\n");
    return all_pass ? 0 : 1;
}
