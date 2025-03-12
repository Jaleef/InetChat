// Copyright 2025 闫佳乐
// Socket类的声明
#pragma once
#include <iostream>

class Socket {
  public:
    Socket();
    ~Socket();
    void print() {
      std::cout << "Socket" << std::endl;
    }
};
