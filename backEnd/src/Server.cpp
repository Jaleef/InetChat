// Copyright 2024
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include <cstring>

#include <iostream>
#include <vector>
#include <algorithm>

constexpr int kMaxEvents = 10;
constexpr uint16_t kPort = 8080;
constexpr int kBufferSize = 1024;

int CreateServerSocket() {
  int server_fd;

  // IPv4地址的结构体，包含in_addr_t类型的s_addr字段
  struct sockaddr_in address;
  int opt = 1;
  int return_flag;

  // socket第一个参数是domain: AF_INET(IPv4) AF_INET6(IPv6) AF_UNIX(本地套接字)
  // 第二个参数是type: SOCK_STREAM(基于TCP的流式套接字)
  // SOCK_DGRAM(基于UPD的数据报套接字) SOCK_RAW(原始套接字)
  // 第三个参数是protocal: 通常为0
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == 0) {
    std::cerr << "Socket creation failed" << strerror(errno) << std::endl;
    // exit是stdlib.h中正常终止程序的函数， EXIT_SUCESS, EXIT_FAILURE
    exit(EXIT_FAILURE);
  }

  // setsockopt设置套接字选项
  return_flag = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                           &opt, sizeof(opt));
  if (return_flag == -1) {
    std::cerr << "Setcoketopt failed: " << strerror(errno) << std::endl;
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  // sin_family指定地址组
  address.sin_family = AF_INET;
  // sin_addr是struct in_addr类型字段，存储IP地址
  // s_addr: in_addr_t类型字段，存储32位IPv4地址
  address.sin_addr.s_addr = INADDR_ANY;
  // sin_port: 指定端口号
  // htons: 将16位端口号从主机序转为网络字节序
  address.sin_port = htons(kPort);

  // 绑定套接字
  return_flag = bind(server_fd, reinterpret_cast<struct sockaddr*>(&address),
                     sizeof(address));
  if (return_flag < 0) {
    std::cerr << "Bind failed: " << strerror(errno) << std::endl;
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  // 监听套接字
  return_flag = listen(server_fd, 0);
  if (return_flag < 0) {
    std::cerr << "Listen failed: " << strerror(errno) << std::endl;
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  return server_fd;
}

void SetNonBlocking(int fd) {
  // fcntl()是一个获取文件描述符状态标志的系统调用， 在fcntl.h中
  // F_GETFL: 获取文件描述符的状态标志
  // 此时函数返回文件描述符的状态
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    std::cerr << "Fcntl F_GETFL failed: " << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }

  int return_flags = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  if (return_flags == -1) {
    std::cerr << "Fcntl F_SETFL failed: " << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char* argv[]) {
  int server_fd = CreateServerSocket();
  // epoll_create()创建epoll系统调用
  int epoll_fd = epoll_create(1);
  if (epoll_fd == -1) {
    std::cerr << "Epoll createion failed " << strerror(errno) << std::endl;
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  // epoll_event是epoll系统调用的事件结构体
  // 描述需要监控的fd及其事件类型
  /*
  struct epoll_event {
    uint32_t events; // epoll事件类型: EPOLLIN(可读) EPOLLOUT(可写) EPOLLERR(错误) 可以用按位与组合多个事件类型
    epoll_data_t data; // 用户数据: 是一个联合体，包含fd和ptr两个字段
  }
  */
  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = server_fd;
  // epoll_ctl()注册需要监控的fd及其事件类型
  // EPOLL_CTL_ADD: 添加server_fd到epoll_fd中
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
    std::cerr << "Epoll_ctl failed: " << strerror(errno) << std::endl;
    close(server_fd);
    close(epoll_fd);
    exit(EXIT_FAILURE);
  }

  std::vector<struct epoll_event> events(kMaxEvents);
  std::vector<int> client_fds;

  while (true) {
    // epoll_wait()等待epoll实例中文件描述符事件
    // events.data(): 返回一个指向底层数组的指针, events.data() == &events[0]
    // kMaxEvents: 事件数组的最大长度
    // -1: 表示阻塞等待， 0: 表示立即返回
    // 返回值是就绪事件的个数
    int nfds = epoll_wait(epoll_fd, events.data(), kMaxEvents, -1);
    if (nfds == -1) {
      std::cerr << "Epoll_wait failed: " << strerror(errno) << std::endl;
      break;
    }

    for (int i = 0 ; i < nfds ; ++i) {
      if (events[i].data.fd == server_fd) {
        // 处理新的客户端连接
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd,
                              reinterpret_cast<struct sockaddr*>(&client_addr),
                              &client_addr_len);
        if (client_fd == -1) {
          std::cerr << "Accept failed: " << strerror(errno) << std::endl;
          continue;
        }

        SetNonBlocking(client_fd);
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = client_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
          std::cerr << "Epoll_ctl failed: " << strerror(errno) << std::endl;
          close(client_fd);
          continue;
        }

        client_fds.push_back(client_fd);
        std::cout << "New client connected: " << inet_ntoa(client_addr.sin_addr)
                  << std::endl;
      } else {
        // 处理客户端消息
        int client_fd = events[i].data.fd;
        char buffer[kBufferSize]{};
        ssize_t valread = read(client_fd, buffer, kBufferSize);
        if (valread <= 0) {
          // 客户端关闭连接
          std::cout << "Client disconnected: " << client_fd << std::endl;
          close(client_fd);
          auto it = std::remove(client_fds.begin(),
                                client_fds.end(),
                                client_fd);
          client_fds.erase(it, client_fds.end());
        } else {
          // 转发消息给所有客户端
          buffer[valread] = '\0';
          std::cout << "Received message from client "
                    << client_fd << ": " << buffer << std::endl;

          for (int fd : client_fds) {
            if (fd != client_fd) {
              send(fd, buffer, valread, 0);
            }
          }
        }
      }
    }
  }

  return 0;
}
