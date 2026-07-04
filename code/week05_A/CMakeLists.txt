# week05_A CMake project
# 目标：在 Windows + MSVC + Visual Studio 2022/2026 上零摩擦运行
# 用法：见 README.md（构建方法与 week01_A 相同，零依赖）

cmake_minimum_required(VERSION 3.20)
project(week05_A
    VERSION 1.0
    DESCRIPTION "Week 5 geometry lab: SymPy-gradient vs C++ finite-difference harness"
    LANGUAGES CXX
)

# ---- 导出 compile_commands.json（给 clangd 等语言服务器用） ----
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- C++ 标准 ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- 输出目录统一 ----
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---- MSVC 专属设置 ----
if(MSVC)
    add_compile_options(/W4 /utf-8 /permissive- /Zc:__cplusplus /MP)
    add_compile_definitions(_USE_MATH_DEFINES NOMINMAX _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
else()
    add_compile_options(-Wall -Wextra)
    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    endif()
endif()

# ---- 公共头库（INTERFACE，仅头文件） ----
add_library(geo_common INTERFACE)
target_include_directories(geo_common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# ---- 两个测试可执行文件 ----
set(TEST_TARGETS test_gradient test_hessian)

foreach(target ${TEST_TARGETS})
    add_executable(${target} ${target}.cpp)
    target_link_libraries(${target} PRIVATE geo_common)
    set_target_properties(${target} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target}>"
        FOLDER "Tests"
    )
endforeach()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT test_gradient)

message(STATUS "")
message(STATUS "==== week05_A configured ====")
message(STATUS "  Generator   : ${CMAKE_GENERATOR}")
message(STATUS "  Compiler    : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Targets     : ${TEST_TARGETS}")
message(STATUS "=============================")
message(STATUS "")
