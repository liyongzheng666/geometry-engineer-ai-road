// mesh.h — Week 4 minimal lab utilities（保特征网格平滑）
// 公共头：V3 向量代数 + 规则栅格高度场三角网 Mesh + 俯视热力图 SVG writer
// 整个第 4 周不需要修改本文件（PASTE ZONE 只在 test_*.cpp 里）
#pragma once

#include <cmath>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

// -------------------------------------------------------------------
// V3: 三维向量（带运算符重载）+ 常用向量代数
//   风格对齐 week01_A/geo.h 的 Point3D，但网格算子需要 dot/cross/normalize
// -------------------------------------------------------------------
struct V3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    V3 operator+(const V3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    V3 operator-(const V3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    V3 operator*(double s)    const { return {x * s,   y * s,   z * s  }; }
};

inline V3 operator*(double s, const V3& p) { return p * s; }
inline double dot(const V3& a, const V3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline V3 cross(const V3& a, const V3& b) {
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}
inline double norm(const V3& a) { return std::sqrt(dot(a, a)); }
inline V3 normalized(const V3& a) {
    const double n = norm(a);
    return n > 0.0 ? a * (1.0 / n) : V3{0, 0, 0};
}

// -------------------------------------------------------------------
// Mesh: 规则栅格高度场三角网
//   顶点 (i,j) 位于 (i*dx, j*dy, z_ij)，i∈[0,nx), j∈[0,ny)
//   每个栅格单元 (i,j)-(i+1,j+1) 沿对角线 (i,j)-(i+1,j+1) 剖成两个三角形
//   —— 既是真实的三角网（有邻域/法向/边界），又能俯视渲染成 SVG 色图，
//      还能"由答案构造输入"（已知折痕 → 加噪 → 平滑）做金标准。
// -------------------------------------------------------------------
struct Tri { int a, b, c; };

struct Mesh {
    int nx = 0, ny = 0;
    double dx = 1.0, dy = 1.0;
    std::vector<V3> pos;    // 顶点坐标，size = nx*ny
    std::vector<Tri> tris;  // 三角形（顶点索引）

    int vid(int i, int j) const { return j * nx + i; }
    bool is_boundary(int i, int j) const {
        return i == 0 || j == 0 || i == nx - 1 || j == ny - 1;
    }
    // 顶点 v 是否在开放边界上（用于"边界不漂移"约定）
    bool is_boundary(int v) const {
        const int i = v % nx, j = v / nx;
        return is_boundary(i, j);
    }

    void build_tris() {
        tris.clear();
        for (int j = 0; j < ny - 1; ++j)
            for (int i = 0; i < nx - 1; ++i) {
                const int v00 = vid(i, j),     v10 = vid(i + 1, j);
                const int v01 = vid(i, j + 1), v11 = vid(i + 1, j + 1);
                tris.push_back({v00, v10, v11});
                tris.push_back({v00, v11, v01});
            }
    }

    // 顶点 v 的关联三角形（O(F)，小网格足够）
    std::vector<int> incident_tris(int v) const {
        std::vector<int> out;
        for (int t = 0; t < static_cast<int>(tris.size()); ++t)
            if (tris[t].a == v || tris[t].b == v || tris[t].c == v) out.push_back(t);
        return out;
    }

    // 顶点 v 的 one-ring 邻居（去重）
    std::vector<int> one_ring(int v) const {
        std::vector<int> out;
        for (int t : incident_tris(v))
            for (int w : {tris[t].a, tris[t].b, tris[t].c})
                if (w != v) {
                    bool found = false;
                    for (int u : out)
                        if (u == w) { found = true; break; }
                    if (!found) out.push_back(w);
                }
        return out;
    }

    // 三角形 t 的"面积加权法向"：方向=单位法向，模长=三角形面积
    //   area = |cross(B-A, C-A)| / 2；退化（零面积）三角形返回零向量
    V3 tri_area_normal(int t) const {
        const V3& A = pos[tris[t].a];
        const V3& B = pos[tris[t].b];
        const V3& C = pos[tris[t].c];
        return cross(B - A, C - A) * 0.5;
    }
};

// 由解析高度函数 f(x,y) 构造高度场网格
inline Mesh make_heightfield(int nx, int ny, double dx, double dy,
                             const std::function<double(double, double)>& f) {
    Mesh m;
    m.nx = nx; m.ny = ny; m.dx = dx; m.dy = dy;
    m.pos.resize(static_cast<size_t>(nx) * ny);
    for (int j = 0; j < ny; ++j)
        for (int i = 0; i < nx; ++i) {
            const double x = i * dx, y = j * dy;
            m.pos[m.vid(i, j)] = {x, y, f(x, y)};
        }
    m.build_tris();
    return m;
}

// 确定性噪声：固定种子 LCG，给高度加 ±amp 的 z 扰动（不动边界）
//   固定种子 → 每次运行输入完全一致，PASS/FAIL 可复现
inline void add_deterministic_noise(Mesh& m, double amp, unsigned seed = 12345u) {
    unsigned s = seed;
    auto next = [&s]() {  // 经典 LCG，产出 [0,1)
        s = 1664525u * s + 1013904223u;
        return (s >> 8) / static_cast<double>(1u << 24);
    };
    for (int j = 0; j < m.ny; ++j)
        for (int i = 0; i < m.nx; ++i) {
            if (m.is_boundary(i, j)) { next(); continue; }  // 边界不加噪，仍消耗随机数保持序列稳定
            m.pos[m.vid(i, j)].z += (next() * 2.0 - 1.0) * amp;
        }
}

// -------------------------------------------------------------------
// 俯视热力图 SVG：把两张网格（before / after）按 z 值染色并排画出
//   每个三角形填成一块，颜色 = 三顶点平均 z 映射到 蓝→青→绿→黄→红 色带
// -------------------------------------------------------------------
inline std::string heat_color(double t) {
    t = t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t);
    // 5 段线性色带：蓝(0)→青(0.25)→绿(0.5)→黄(0.75)→红(1)
    double r, g, b;
    if (t < 0.25)      { const double u = t / 0.25;        r = 0;             g = u;             b = 1; }
    else if (t < 0.5)  { const double u = (t - 0.25) / 0.25; r = 0;           g = 1;             b = 1 - u; }
    else if (t < 0.75) { const double u = (t - 0.5) / 0.25;  r = u;           g = 1;             b = 0; }
    else               { const double u = (t - 0.75) / 0.25; r = 1;           g = 1 - u;         b = 0; }
    auto ch = [](double v) { return static_cast<int>(v * 255.0 + 0.5); };
    char buf[16];
    std::snprintf(buf, sizeof(buf), "#%02x%02x%02x", ch(r), ch(g), ch(b));
    return std::string(buf);
}

class MeshSVG {
    std::ofstream f;
    double zmin, zmax, panelW, gap;

    void panel(const Mesh& m, double xoff) {
        for (int t = 0; t < static_cast<int>(m.tris.size()); ++t) {
            const V3& A = m.pos[m.tris[t].a];
            const V3& B = m.pos[m.tris[t].b];
            const V3& C = m.pos[m.tris[t].c];
            const double zavg = (A.z + B.z + C.z) / 3.0;
            const double tt = (zmax > zmin) ? (zavg - zmin) / (zmax - zmin) : 0.5;
            f << "<polygon points='"
              << (A.x + xoff) << "," << A.y << " "
              << (B.x + xoff) << "," << B.y << " "
              << (C.x + xoff) << "," << C.y
              << "' fill='" << heat_color(tt) << "' stroke='#33333322' stroke-width='0.01'/>\n";
        }
    }

public:
    // 两张网格并排：world 尺寸取自 before；zmin/zmax 用同一色标便于对比
    MeshSVG(const std::string& path, const Mesh& before, const Mesh& after,
            double zmin_, double zmax_)
        : zmin(zmin_), zmax(zmax_) {
        panelW = (before.nx - 1) * before.dx;
        gap = panelW * 0.15;
        const double totalW = panelW * 2 + gap;
        const double totalH = (before.ny - 1) * before.dy;
        f.open(path);
        f << "<svg xmlns='http://www.w3.org/2000/svg' viewBox='"
          << -0.5 << " " << -1.0 << " " << (totalW + 1.0) << " " << (totalH + 1.5)
          << "' width='960' height='420' style='background:white'>\n"
          << "<g transform='matrix(1 0 0 -1 0 " << totalH << ")'>\n";
        panel(before, 0.0);                 // 左：before（noisy）
        panel(after, panelW + gap);         // 右：after（smoothed）
    }
    ~MeshSVG() { f << "</g></svg>\n"; }
};

// -------------------------------------------------------------------
// Windows 控制台 UTF-8 输出（让中文 std::cout 不乱码）
// -------------------------------------------------------------------
#ifdef _WIN32
#include <windows.h>
inline void enable_utf8_console() { SetConsoleOutputCP(CP_UTF8); }
#else
inline void enable_utf8_console() {}
#endif
