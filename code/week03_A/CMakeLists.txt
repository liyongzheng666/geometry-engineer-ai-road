# week03_A CMake project
# 目标：在 Windows + MSVC + Visual Studio 2022/2026 上零摩擦运行
# 用法：见 README.md（构建方法与 week01_A 相同；本周多一个 Eigen 依赖）

cmake_minimum_required(VERSION 3.20)
project(week03_A
    VERSION 1.0
    DESCRIPTION "Week 3 geometry lab: context-injection harness for circumcircle / plane-fit"
    LANGUAGES CXX
)

# ---- 导出 compile_commands.json（给 clangd 等语言服务器用） ----
# 注意：只对 Makefile/Ninja generator 有效，Visual Studio generator 不支持
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- C++ 标准 ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- 输出目录统一 ----
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---- MSVC 专属设置 ----
if(MSVC)
    add_compile_options(
        /W4              # 高警告级别（注意：Eigen 头文件在 /W4 下可能刷少量警告，可忽略）
        /utf-8           # 源文件按 UTF-8 解析（必需，否则中文字符串和注释会让 MSVC 报 C4819）
        /permissive-     # 更严格的标准遵从
        /Zc:__cplusplus  # 让 __cplusplus 宏报告真实值（默认会撒谎报 199711L）
        /MP              # 多核并行编译
    )
    add_compile_definitions(
        _USE_MATH_DEFINES        # 启用 <cmath> 中的 M_PI 等常量
        NOMINMAX                 # 阻止 windows.h 定义 min/max 宏污染 std
        _CRT_SECURE_NO_WARNINGS  # 屏蔽 fopen 等"不安全"警告
        WIN32_LEAN_AND_MEAN      # windows.h 精简模式，编译更快
    )
else()
    add_compile_options(-Wall -Wextra)
    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    endif()
endif()

# ---- Eigen3：优先用本机安装，找不到则 FetchContent 在线获取 ----
# （仅 test_plane_fit 需要；首次无本机 Eigen 时 configure 需联网约 30s）
find_package(Eigen3 QUIET CONFIG)
if(NOT TARGET Eigen3::Eigen)
    message(STATUS "本机未找到 Eigen3，改用 FetchContent 拉取 3.4.0（仅首次配置需联网）")
    include(FetchContent)
    set(BUILD_TESTING         OFF CACHE BOOL "" FORCE)   # 只要头文件，关掉 Eigen 自带测试/文档
    set(EIGEN_BUILD_DOC       OFF CACHE BOOL "" FORCE)
    set(EIGEN_BUILD_TESTING   OFF CACHE BOOL "" FORCE)
    set(EIGEN_BUILD_PKGCONFIG OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(eigen
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG        3.4.0
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(eigen)   # 提供 Eigen3::Eigen INTERFACE target
endif()

# ---- 公共头库（INTERFACE，仅头文件） ----
add_library(geo_common INTERFACE)
target_include_directories(geo_common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# ---- 两个测试可执行文件 ----
set(TEST_TARGETS test_circumcircle test_plane_fit)

foreach(target ${TEST_TARGETS})
    add_executable(${target} ${target}.cpp)
    target_link_libraries(${target} PRIVATE geo_common)

    # Visual Studio 调试时的工作目录 = exe 所在目录
    set_target_properties(${target} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target}>"
        FOLDER "Tests"
    )
endforeach()

# 只有 plane_fit 需要 Eigen；circumcircle 保持零依赖（mini_types.h 纯头）
target_link_libraries(test_plane_fit PRIVATE Eigen3::Eigen)

# ---- 启动项目（VS 里按 F5 默认运行哪个） ----
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT test_circumcircle)

# ---- 完成提示 ----
message(STATUS "")
message(STATUS "==== week03_A configured ====")
message(STATUS "  Generator   : ${CMAKE_GENERATOR}")
message(STATUS "  Compiler    : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Output dir  : ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "  Targets     : ${TEST_TARGETS}")
message(STATUS "=============================")
message(STATUS "")
