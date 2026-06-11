// geo.h — Week 1 minimal lab utilities (MSVC / VS2022+ compatible)
// 公共头：Point3D + SVG writer
// 整个第 1 周不需要修改本文件
#pragma once

#include <cmath>
#include <fstream>
#include <string>
#include <vector>

// -------------------------------------------------------------------
// Point3D: 项目中的基础几何类型（带运算符重载）
// -------------------------------------------------------------------
struct Point3D {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    Point3D operator+(const Point3D& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Point3D operator-(const Point3D& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Point3D operator*(double s)         const { return {x * s,   y * s,   z * s  }; }
};

// 支持 (scalar * Point3D) 形式
inline Point3D operator*(double s, const Point3D& p) { return p * s; }

// -------------------------------------------------------------------
// SVG writer: 轻量 SVG 输出，自动 y 轴上翻（数学习惯）
//   - polyline: 折线
//   - dots: 实心圆点
//   - line: 单条线段
// 写入是 RAII：对象析构时自动闭合 </g></svg>
// -------------------------------------------------------------------
class SVG {
    std::ofstream f;

public:
    SVG(const std::string& path, double x0, double y0, double x1, double y1) {
        f.open(path);
        const double w = x1 - x0;
        const double h = y1 - y0;
        f << "<svg xmlns='http://www.w3.org/2000/svg' viewBox='"
          << x0 << " " << -y1 << " " << w << " " << h
          << "' width='800' height='600' style='background:white'>\n"
          << "<g transform='matrix(1 0 0 -1 0 0)'>\n";  // y 轴翻转
    }

    void polyline(const std::vector<Point3D>& pts,
                  const std::string& color,
                  double width) {
        if (pts.empty()) return;
        f << "<polyline points='";
        for (const auto& p : pts) f << p.x << "," << p.y << " ";
        f << "' fill='none' stroke='" << color
          << "' stroke-width='" << width << "'/>\n";
    }

    void dots(const std::vector<Point3D>& pts,
              const std::string& color,
              double r) {
        for (const auto& p : pts) {
            f << "<circle cx='" << p.x << "' cy='" << p.y
              << "' r='" << r << "' fill='" << color << "'/>\n";
        }
    }

    void line(double x0, double y0, double x1, double y1,
              const std::string& color, double width) {
        f << "<line x1='" << x0 << "' y1='" << y0
          << "' x2='" << x1 << "' y2='" << y1
          << "' stroke='" << color
          << "' stroke-width='" << width << "'/>\n";
    }

    ~SVG() { f << "</g></svg>\n"; }
};

// -------------------------------------------------------------------
// Windows 控制台 UTF-8 输出（让中文 std::cout 不乱码）
// 在每个 main() 最前面调用一次即可
// -------------------------------------------------------------------
#ifdef _WIN32
#include <windows.h>
inline void enable_utf8_console() {
    SetConsoleOutputCP(CP_UTF8);
}
#else
inline void enable_utf8_console() {}
#endif
