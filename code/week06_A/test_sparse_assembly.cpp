// test_sparse_assembly.cpp — 把逐顶点局部 Hessian 组装成全局稀疏矩阵的验证闭环
// 用法：把 AI 写的 assemble_sparse（Eigen::Triplet + setFromTriplets）粘进 PASTE ZONE
//
// 金标准：与一个"稠密直接组装"的参考逐元对照（< 1e-12）+ 全局对称性。
// 局部 4×4 块 local_hessian 是给定基础设施（弹簧边的二阶导），你只写 scatter 外壳。
// 大纲要点：用 Triplet 收集 + 一次 setFromTriplets（会自动对共享 dof 求和），
//           不要用 insert()（多次 insert 触发多次重排）。

#include "codegen_ref.h"  // 仅复用 time_ms / enable_utf8_console

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// -------- 给定基础设施：弹簧边的局部 Hessian（不在 PASTE ZONE） --------
namespace sp_ {
constexpr double K = 1.0;    // 劲度
constexpr double L = 0.8;    // 静止长度
struct Edge { int i, j; };

// 单根弹簧 ½K(‖pi−pj‖−L)² 对端点 pi 的 2×2 二阶导块
inline Eigen::Matrix2d block2(const Eigen::Vector2d& pi, const Eigen::Vector2d& pj) {
    const Eigen::Vector2d dvec = pi - pj;
    const double d = dvec.norm();
    const Eigen::Vector2d n = dvec / d;
    const Eigen::Matrix2d nnT = n * n.transpose();
    const Eigen::Matrix2d I = Eigen::Matrix2d::Identity();
    return K * (nnT + (1.0 - L / d) * (I - nnT));
}
// 边的 4×4 局部 Hessian（dof 顺序 [pi_x,pi_y,pj_x,pj_y]）
inline Eigen::Matrix4d local_hessian(const Eigen::Vector2d& pi, const Eigen::Vector2d& pj) {
    const Eigen::Matrix2d A = block2(pi, pj);
    Eigen::Matrix4d H;
    H.block<2, 2>(0, 0) = A;   H.block<2, 2>(0, 2) = -A;
    H.block<2, 2>(2, 0) = -A;  H.block<2, 2>(2, 2) = A;
    return H;
}
// 稠密参考组装：直接按 dof 下标累加（与稀疏走不同代码路径）
inline Eigen::MatrixXd dense_assemble(int nv, const std::vector<Edge>& E,
                                      const std::vector<Eigen::Vector2d>& pos) {
    Eigen::MatrixXd H = Eigen::MatrixXd::Zero(2 * nv, 2 * nv);
    for (const auto& e : E) {
        const Eigen::Matrix4d L4 = local_hessian(pos[e.i], pos[e.j]);
        const int idx[4] = {2 * e.i, 2 * e.i + 1, 2 * e.j, 2 * e.j + 1};
        for (int a = 0; a < 4; ++a)
            for (int b = 0; b < 4; ++b) H(idx[a], idx[b]) += L4(a, b);
    }
    return H;
}
}  // namespace sp_

// =================================================================
// >>>>>>>>>>  PASTE AI-GENERATED assemble_sparse BELOW  <<<<<<<<<<
// =================================================================

// 用 Eigen::Triplet 收集所有非零项，最后一次 setFromTriplets（自动对重复项求和）。
// dof 映射：顶点 v 的两个自由度是全局下标 2v, 2v+1。
Eigen::SparseMatrix<double> assemble_sparse(int nv, const std::vector<sp_::Edge>& E,
                                            const std::vector<Eigen::Vector2d>& pos) {
    // TODO: 粘贴 / 实现 Triplet 收集 + setFromTriplets
    (void)E; (void)pos;
    return Eigen::SparseMatrix<double>(2 * nv, 2 * nv);  // 占位：空矩阵
}

// =================================================================
// >>>>>>>>>>             END OF PASTE ZONE              <<<<<<<<<<
// =================================================================

// 稀疏矩阵稀疏图 spy SVG（每个非零画一格），一眼看出块三对角结构
static void write_spy(const std::string& path, const Eigen::SparseMatrix<double>& H) {
    const int n = static_cast<int>(H.rows());
    std::ofstream f(path);
    const double cell = 20.0;
    f << "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 " << n * cell << " " << n * cell
      << "' width='400' height='400' style='background:white'>\n";
    for (int k = 0; k < H.outerSize(); ++k)
        for (Eigen::SparseMatrix<double>::InnerIterator it(H, k); it; ++it)
            f << "<rect x='" << it.col() * cell << "' y='" << it.row() * cell << "' width='" << cell
              << "' height='" << cell << "' fill='#2266ff'/>\n";
    f << "</svg>\n";
}

int main() {
    enable_utf8_console();
    bool all_pass = true;

    // 构造一条 zigzag 链（顶点/边），保证 d≠0
    auto make_chain = [](int nv) {
        std::vector<Eigen::Vector2d> pos;
        std::vector<sp_::Edge> E;
        for (int i = 0; i < nv; ++i) pos.emplace_back(i * 1.0, (i % 2) * 0.5);
        for (int i = 0; i + 1 < nv; ++i) E.push_back({i, i + 1});
        return std::make_pair(pos, E);
    };

    // ---- Test 1: 稀疏组装 vs 稠密参考逐元对照（< 1e-12） ----
    std::cout << "=== Test 1: sparse assembly vs dense reference ===\n";
    {
        const int nv = 6;
        auto [pos, E] = make_chain(nv);
        const Eigen::MatrixXd dense = sp_::dense_assemble(nv, E, pos);
        const Eigen::SparseMatrix<double> sparse = assemble_sparse(nv, E, pos);
        const double err = (dense - Eigen::MatrixXd(sparse)).cwiseAbs().maxCoeff();
        const bool ok = err < 1e-12;
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << "max |dense - sparse| = " << err
                  << "   (nnz=" << sparse.nonZeros() << ")\n";
        std::cout << "  说明：内部顶点被两条边共享，setFromTriplets 必须把两份对角块求和——错则此项 FAIL。\n";
        if (!ok) all_pass = false;
        write_spy("sparse_pattern_output.svg", sparse);
    }

    // ---- Test 2: 全局对称性 H == Hᵀ ----
    std::cout << "\n=== Test 2: global symmetry ===\n";
    {
        const int nv = 8;
        auto [pos, E] = make_chain(nv);
        const Eigen::SparseMatrix<double> H = assemble_sparse(nv, E, pos);
        const double asym = (Eigen::MatrixXd(H) - Eigen::MatrixXd(H).transpose()).cwiseAbs().maxCoeff();
        const bool ok = asym < 1e-12;
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << "max |H - Hᵀ| = " << asym << "\n";
        if (!ok) all_pass = false;
    }

    // ---- Test 3: 大规模计时（Triplet 一次组装 vs 参考稠密） ----
    std::cout << "\n=== Test 3: timing on a big chain (observation) ===\n";
    {
        const int nv = 20000;
        const auto chain = make_chain(nv);   // 不用结构化绑定：下面 lambda 捕获它们，C++17 里捕获结构化绑定是扩展
        const auto& pos = chain.first;
        const auto& E = chain.second;
        Eigen::SparseMatrix<double> H;
        const double t = cg::time_ms([&] { H = assemble_sparse(nv, E, pos); }, 1);
        std::cout << "  assemble " << nv << " verts (" << E.size() << " edges) → "
                  << "2N=" << 2 * nv << ", nnz=" << H.nonZeros() << "  in " << t << " ms\n";
        std::cout << "  问题：若改用 insert() 逐个塞（而非 Triplet+setFromTriplets），\n"
                     "        每次插入可能触发列重排，大规模下会慢一到两个量级——这就是大纲要你否决 insert() 的原因。\n";
    }

    const auto abs_path = std::filesystem::absolute("sparse_pattern_output.svg");
    std::cout << "\n=== SVG ===\n生成: " << abs_path.string() << "\n";
    std::cout << "拖进浏览器：12×12 稀疏图应呈块三对角（相邻顶点耦合），孤立顶点无耦合。\n";

    std::cout << (all_pass ? "\nAll numeric checks passed.\n"
                           : "\nSome checks failed. 检查 assemble_sparse。\n");
    return all_pass ? 0 : 1;
}
