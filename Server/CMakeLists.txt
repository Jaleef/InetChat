cmake_minimum_required(VERSION 3.20)

project(server)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  src/Server.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

# 添加头文件搜索路径
target_include_directories(${PROJECT_NAME}
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
