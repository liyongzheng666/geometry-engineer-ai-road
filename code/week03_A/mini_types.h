// mini_types.h — 模拟"公司项目核心几何头文件"（第 3 周上下文注入练习材料，不要修改）
//
// 练习方式：把本文件【全文】贴进 AI 上下文，并加上禁令：
//   "只准使用 gx:: 命名空间里的类型和方法 + <cmath>；
//    禁止 STL 容器、禁止异常/std::optional、禁止运算符重载式写法（用 .add()/.sub()）；
//    容差一律使用 gx::kEpsGeom。"
//
// 这套类型的风格【刻意】与 geo.h 的 Point3D 相反（成员函数而非运算符、
// 错误码而非异常、访问器而非裸成员）——AI 是否服从注入的风格，就是本周的判分点。

#pragma once
#include <cmath>

namespace gx {

using scalar_t = double;

// 项目统一几何容差
inline constexpr scalar_t kEpsGeom = 1e-10;

// 项目统一错误码：所有可失败的几何例程返回 GxStatus + 输出参数
enum class GxStatus {
    kOk = 0,
    kNotEnoughPoints,   // 输入数量不足
    kDegenerateInput,   // 几何退化（共线 / 重合 / 零向量）
    kNumericFailure,    // 数值失败（除零、不收敛）
};

inline const char* status_name(GxStatus s) {
    switch (s) {
        case GxStatus::kOk:              return "kOk";
        case GxStatus::kNotEnoughPoints: return "kNotEnoughPoints";
        case GxStatus::kDegenerateInput: return "kDegenerateInput";
        case GxStatus::kNumericFailure:  return "kNumericFailure";
    }
    return "<unknown>";
}

class Vec3 {
public:
    Vec3() : v_{0.0, 0.0, 0.0} {}
    Vec3(scalar_t x, scalar_t y, scalar_t z) : v_{x, y, z} {}

    scalar_t x() const { return v_[0]; }
    scalar_t y() const { return v_[1]; }
    scalar_t z() const { return v_[2]; }

    Vec3 add(const Vec3& o) const {
        return Vec3(v_[0] + o.v_[0], v_[1] + o.v_[1], v_[2] + o.v_[2]);
    }
    Vec3 sub(const Vec3& o) const {
        return Vec3(v_[0] - o.v_[0], v_[1] - o.v_[1], v_[2] - o.v_[2]);
    }
    Vec3 scaled(scalar_t s) const {
        return Vec3(v_[0] * s, v_[1] * s, v_[2] * s);
    }
    scalar_t dot(const Vec3& o) const {
        return v_[0] * o.v_[0] + v_[1] * o.v_[1] + v_[2] * o.v_[2];
    }
    Vec3 cross(const Vec3& o) const {
        return Vec3(v_[1] * o.v_[2] - v_[2] * o.v_[1],
                    v_[2] * o.v_[0] - v_[0] * o.v_[2],
                    v_[0] * o.v_[1] - v_[1] * o.v_[0]);
    }
    scalar_t squared_norm() const { return dot(*this); }
    scalar_t norm() const { return std::sqrt(squared_norm()); }

    // 归一化可能失败（零向量）→ 错误码 + 输出参数，项目里禁止抛异常
    GxStatus normalized(Vec3& out) const {
        const scalar_t n = norm();
        if (n < kEpsGeom) return GxStatus::kDegenerateInput;
        out = scaled(1.0 / n);
        return GxStatus::kOk;
    }

    scalar_t distance_to(const Vec3& o) const { return sub(o).norm(); }

private:
    scalar_t v_[3];
};

// 三维空间中的圆（本周练习只用到圆心+半径；法向留作扩展）
struct Circle3 {
    Vec3     center;
    scalar_t radius = 0.0;
};

}  // namespace gx
