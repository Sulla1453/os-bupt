cmake_minimum_required(VERSION 3.10)

# 项目名称
project(OS_Simulator)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 包含头文件目录
include_directories(include)

# 源文件目录
file(GLOB SOURCES
    src/*.cpp
)

# 可执行文件
add_executable(os_sim ${SOURCES} main.cpp)