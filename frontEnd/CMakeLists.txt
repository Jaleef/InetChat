cmake_minimum_required(VERSION 3.20)

project(frontEnd)

set(SOURCES
  src/Client.cpp
  src/MessageLogger.cpp
)

add_executable(frontEnd ${SOURCES})
