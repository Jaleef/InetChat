// copyright 闫佳乐
#include <arpa/inet.h>
#include <unistd.h>

#include <cstring>

#include <iostream>
#include <string>
#include <thread>

constexpr int kBufferSize = 1024;
constexpr int kPort = 8080;
constexpr char* kServerIp = "127.0.0.1";

class Client {
 public:
  Client() = default;
  ~Client() { close(sock); }

  void connectServer() {
    struct sockaddr_in serv_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
      throw std::runtime_error("Socket creation error");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(kPort);

    // 该函数将点分十进制的IP地址转换为网络字节序的二进制地址
    // 将转换的二进制地址存在sin_addr中
    if (inet_pton(AF_INET, kServerIp, &serv_addr.sin_addr) <= 0) {
      throw std::runtime_error("Invalid address / Address not supported");
    }

    if (connect(sock, reinterpret_cast<struct sockaddr*>(&serv_addr),
                sizeof(serv_addr)) < 0) {
      throw std::runtime_error("Connection failed");
    }

    std::cout << "Connected to server" << std::endl;
  }

  void loop() {
    std::thread send_thread(&Client::SendThread, this);
    std::thread recv_thread(&Client::RecvThread, this);

    send_thread.join();
    recv_thread.join();
  }

  // 发送消息的线程函数
  void SendThread() {
    while (true) {
      std::string message{};
      std::getline(std::cin, message);

      if (send(sock, message.c_str(), message.size(), 0) < 0) {
        std::cerr << "Send failed: " << strerror(errno) << std::endl;
      }
    }
  }

  // 接收消息的线程函数
  void RecvThread() {
    char buffer[kBufferSize]{};
    while (true) {
      ssize_t valread = recv(sock, buffer, kBufferSize - 1, 0);
      if (valread <= 0) {
        if (valread == 0) {
          std::cout << "Server closed the connection." << std::endl;
        } else {
          std::cerr << "Recv failed: " << strerror(errno) << std::endl;
        }
        break;
      }
      buffer[valread] = '\0';

      uint32_t network_number;
      memcpy(&network_number, buffer, 4);
      int client_fd = ntohl(network_number);

      std::cout << "user " << client_fd << ": " << buffer + 4 << std::endl;
    }
  }

 private:
  int sock{};
  char buffer[kBufferSize]{};
};

int main() {
  Client client{};
  try {
    client.connectServer();
    client.loop();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
  return 0;
}
