// copyright 闫佳乐
#include <arpa/inet.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>
#include <thread>

constexpr int kBufferSize = 1024;
constexpr int kPort = 8080;

int sock{};

// 发送消息的线程函数
void SendThread() {
  while (true) {
    std::string message{};
    // std::cout << "self: ";
    std::getline(std::cin, message);

    if (send(sock, message.c_str(), message.size(), 0) < 0) {
      std::cerr << "Send failed: " << strerror(errno) << std::endl;
      break;
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
    std::cout << "other: " << buffer << std::endl;
  }
}
int main() {
  struct sockaddr_in serv_addr;
  char buffer[kBufferSize]{};

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    std::cerr << "Socket creation error: " << strerror(errno) << std::endl;
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(kPort);

  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
    std::cerr << "Invalid address / Address not supported" << std::endl;
    close(sock);
    return -1;
  }

  if (connect(sock, reinterpret_cast<struct sockaddr *>(&serv_addr),
              sizeof(serv_addr)) < 0) {
    std::cerr << "Connection failed: " << strerror(errno) << std::endl;
    close(sock);
    return -1;
  }

  std::cout << "Connected to server" << std::endl;

  std::thread send_thread(SendThread);
  std::thread recv_thread(RecvThread);

  send_thread.join();
  recv_thread.join();

  close(sock);
  return 0;
}
