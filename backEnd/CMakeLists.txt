cmake_minimum_required(VERSION 3.20)

# set(CMAKE_CXX_COMPILER "clang++")
# set(CMAKE_C_COMPILER "clang")
# set(CMAKE_MAKE_PROGRAM "ninja")

project(backEnd)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  src/main.cpp

)

add_executable(backEnd ${SOURCES})

# 添加头文件搜索路径
target_include_directories(backEnd
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
