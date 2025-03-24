// copyright 闫佳乐
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <csignal>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "MessageLogger.cpp"

constexpr int kBufferSize = 1024;
constexpr int kPort = 8080;
constexpr char* kServerIp = "8.147.220.117";
constexpr int kMaxRetries = 5;
constexpr int kCheckInterval = 2;

class Client {
 public:
  Client() {
    logger_.printMessage();
    createClientSocket();
    is_connect_ = true;
    std::signal(SIGINT, signalHandler);
  }
  ~Client() { close(sock); }

  void createClientSocket() {
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

    std::cout << "(Connected to server)" << std::endl;
  }

  void loop() {
    std::thread send_thread(&Client::sendThread, this);
    std::thread recv_thread(&Client::recvThread, this);
    std::thread connection_check_thread(&Client::connectionCheckThread, this);

    send_thread.join();
    recv_thread.join();
    connection_check_thread.join();
  }

  // 发送消息的线程函数
  void sendThread() {
    // 设置为非阻塞模式
    setNonBlocking(sock);

    while (true) {
      std::string message{};
      std::getline(std::cin, message);

      if (message == "exit") {
        exit_flag = true;
        is_connect_ = false;
        break;
      }

      int sent = send(sock, message.c_str(), message.size(), 0);
      if (sent > 0) {
        logger_.saveMessage(message);

      } else if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          std::cout << "Send buffer full, retrying..." << std::endl;
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          continue;
        } else {
          std::cerr << "Send failed: " << strerror(errno) << std::endl;
          is_connect_ = false;
        }
      }
    }
  }

  // 接收消息的线程函数
  void recvThread() {
    // 设置为非阻塞模式
    setNonBlocking(sock);

    char buffer[kBufferSize]{};
    while (true) {
      if (exit_flag) {
        break;
      }

      ssize_t valread = recv(sock, buffer, kBufferSize - 1, 0);
      if (valread > 0) {
        buffer[valread] = '\0';

        uint32_t network_number;
        memcpy(&network_number, buffer, 4);
        int user_id = ntohl(network_number);

        std::string message =
            "          user " + std::to_string(user_id) + ": " + (buffer + 4);
        std::cout << message << std::endl;
        logger_.saveMessage(message);

      } else if (valread == 0) {
        // 服务器关闭连接
        std::cout << "Server closed the connection." << std::endl;
        is_connect_ = false;

      } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // 没有数据可读
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

      } else {
        // 接收失败
        std::cerr << "Recv failed: " << strerror(errno) << std::endl;
        is_connect_ = false;
      }
    }
  }

  void connectionCheckThread() {
    int retry_count = 0;
    while (true) {
      if (exit_flag) {
        break;
      }

      if (!is_connect_) {
        if (retry_count >= kMaxRetries) {
          std::cerr << "(Max retries reached, exiting...)" << std::endl;
          exit_flag = true;
          break;
        }
        std::cout << "(Reconnecting (" << retry_count + 1 << "/" << kMaxRetries
                  << ")...)" << std::endl;

        if (sock > 0) {
          close(sock);
        }
        try {
          createClientSocket();
          is_connect_ = true;
          retry_count = 0;
          std::cout << "(Reconnected to server)" << std::endl;
        } catch (const std::runtime_error& e) {
          std::cerr << e.what() << std::endl;
          retry_count++;
          std::this_thread::sleep_for(std::chrono::seconds(kCheckInterval));
        }
      }
    }
  }

  static void signalHandler(int signal) {
    std::cout << "(Signal " << signal << " received)" << std::endl;
    std::exit(signal);
  }

  void setNonBlocking(int socket_fd) {
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) {
      throw std::runtime_error("fcntl failed");
    }

    if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
      throw std::runtime_error("fcntl failed");
    }
  }

 private:
  int sock{-1};
  char buffer[kBufferSize]{};
  std::atomic<bool> is_connect_{false};
  std::atomic<bool> exit_flag{false};
  MessageLogger logger_;
};

int main() {
  Client client{};
  client.loop();
  return 0;
}
