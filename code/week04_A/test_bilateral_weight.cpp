// test_bilateral_weight.cpp — 双边滤波权重核的验证闭环
// 用法：把 AI 生成的 bilateral_weight 函数粘到 PASTE ZONE 之间，重新编译即可
//
// 权重核 = 空间核 × range 核：
//   w(d_c, d_s) = exp(-d_c² / (2σc²)) · exp(-d_s² / (2σs²))
// 其中 d_c 是邻居的空间距离，d_s 是邻居相对当前顶点沿法向的偏移（range/信号差）。
// 金标准是核的代数性质（对称、非负、单调衰减、d=0 时=1），无需外部数据。

#include "mesh.h"

#include <cmath>
#include <iostream>
#include <string>

// =================================================================
// >>>>>>>>>>  PASTE AI-GENERATED bilateral_weight BELOW  <<<<<<<<<<
// =================================================================

// 双边权重：spatial_dist=空间距离 d_c，range_diff=沿法向的信号差 d_s。
double bilateral_weight(double spatial_dist, double range_diff,
                        double sigma_c, double sigma_s) {
    // TODO: 在这里粘贴 / 实现 高斯空间核 × 高斯 range 核
    (void)spatial_dist; (void)range_diff; (void)sigma_c; (void)sigma_s;
    return 0.0;  // 占位：未实现
}

// =================================================================
// >>>>>>>>>>             END OF PASTE ZONE              <<<<<<<<<<
// =================================================================

// 独立验证路径：单个一维高斯（与被测的乘积式实现走不同书写路径）
static double gauss_ref(double d, double sigma) {
    return std::exp(-(d * d) / (2.0 * sigma * sigma));
}

int main() {
    enable_utf8_console();

    bool all_pass = true;
    auto check = [&](const std::string& name, double got, double expect, double tol) {
        const double e = std::fabs(got - expect);
        const bool ok = e < tol;
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << name
                  << "  got=" << got << "  expect=" << expect << "  err=" << e << "\n";
        if (!ok) all_pass = false;
    };
    auto check_true = [&](const std::string& name, bool ok) {
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << name << "\n";
        if (!ok) all_pass = false;
    };

    const double sc = 1.5, ss = 0.4;

    // ---- Test 1: 原点权重 = 1 ----
    std::cout << "=== Test 1: w(0,0) == 1 ===\n";
    check("w(0,0)", bilateral_weight(0.0, 0.0, sc, ss), 1.0, 1e-15);

    // ---- Test 2: 与"两个独立一维高斯之积"双算对照 ----
    std::cout << "\n=== Test 2: separability (vs product of two Gaussians) ===\n";
    for (double dc : {0.0, 0.5, 1.2, 2.0})
        for (double ds : {0.0, 0.1, 0.3, 0.9}) {
            const double ref = gauss_ref(dc, sc) * gauss_ref(ds, ss);
            check("w(" + std::to_string(dc) + "," + std::to_string(ds) + ")",
                  bilateral_weight(dc, ds, sc, ss), ref, 1e-12);
        }

    // ---- Test 3: 非负且 ≤ 1 ----
    std::cout << "\n=== Test 3: non-negativity and w<=1 ===\n";
    {
        bool ok = true;
        for (double dc = 0; dc <= 5; dc += 0.37)
            for (double ds = 0; ds <= 3; ds += 0.29) {
                const double w = bilateral_weight(dc, ds, sc, ss);
                if (!(w >= 0.0 && w <= 1.0 + 1e-12)) ok = false;
            }
        check_true("all weights in [0,1]", ok);
    }

    // ---- Test 4: 单调衰减（距离/信号差越大权重越小） ----
    std::cout << "\n=== Test 4: monotonic decay ===\n";
    {
        bool ok_c = true, ok_s = true;
        double prev = bilateral_weight(0.0, 0.2, sc, ss);
        for (double dc = 0.2; dc <= 4; dc += 0.2) {
            const double w = bilateral_weight(dc, 0.2, sc, ss);
            if (w >= prev) ok_c = false;
            prev = w;
        }
        prev = bilateral_weight(0.5, 0.0, sc, ss);
        for (double ds = 0.1; ds <= 2; ds += 0.1) {
            const double w = bilateral_weight(0.5, ds, sc, ss);
            if (w >= prev) ok_s = false;
            prev = w;
        }
        check_true("spatial: 距离↑ → 权重严格↓", ok_c);
        check_true("range:   信号差↑ → 权重严格↓", ok_s);
    }

    // ---- Test 5: 偶函数（对 ±d 对称） ----
    std::cout << "\n=== Test 5: even symmetry w(d)=w(-d) ===\n";
    check("spatial ±", bilateral_weight(-1.3, 0.5, sc, ss),
          bilateral_weight(1.3, 0.5, sc, ss), 1e-15);
    check("range   ±", bilateral_weight(0.7, -0.6, sc, ss),
          bilateral_weight(0.7, 0.6, sc, ss), 1e-15);

    // ---- Test 6: 观察题 —— σs 越小，跨折痕（大 range 差）越被压制 ----
    std::cout << "\n=== Test 6: role of sigma_s (feature stopping) ===\n";
    {
        const double across = 1.0;  // 跨折痕的大信号差
        std::cout << "  σs=0.1  跨折痕权重 = " << bilateral_weight(0.5, across, sc, 0.1) << "\n";
        std::cout << "  σs=1.0  跨折痕权重 = " << bilateral_weight(0.5, across, sc, 1.0) << "\n";
        std::cout << "  问题：σs 小 → 跨折痕邻居几乎不参与平均（特征被保住）；这就是双边滤波保边的关键旋钮。\n";
    }

    std::cout << (all_pass ? "\nAll numeric checks passed.\n"
                           : "\nSome checks failed. 检查 AI 代码。\n");
    return all_pass ? 0 : 1;
}
