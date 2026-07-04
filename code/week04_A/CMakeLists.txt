# week04_A CMake project
# 目标：在 Windows + MSVC + Visual Studio 2022/2026 上零摩擦运行
# 用法：见 README.md（构建方法与 week01_A 相同，零依赖）

cmake_minimum_required(VERSION 3.20)
project(week04_A
    VERSION 1.0
    DESCRIPTION "Week 4 geometry lab: feature-preserving mesh smoothing harness"
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
        /W4              # 高警告级别
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

# ---- 公共头库（INTERFACE，仅头文件） ----
add_library(geo_common INTERFACE)
target_include_directories(geo_common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# ---- 三个测试可执行文件 ----
set(TEST_TARGETS test_vertex_normal test_bilateral_weight test_mesh_smooth)

foreach(target ${TEST_TARGETS})
    add_executable(${target} ${target}.cpp)
    target_link_libraries(${target} PRIVATE geo_common)

    # Visual Studio 调试时的工作目录 = exe 所在目录
    set_target_properties(${target} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target}>"
        FOLDER "Tests"
    )
endforeach()

# ---- 启动项目（VS 里按 F5 默认运行哪个） ----
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT test_mesh_smooth)

# ---- 完成提示 ----
message(STATUS "")
message(STATUS "==== week04_A configured ====")
message(STATUS "  Generator   : ${CMAKE_GENERATOR}")
message(STATUS "  Compiler    : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Output dir  : ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "  Targets     : ${TEST_TARGETS}")
message(STATUS "=============================")
message(STATUS "")
