cmake_minimum_required(VERSION 3.20)

project(client)

set(SOURCES
  src/Client.cpp
  src/MessageLogger.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
