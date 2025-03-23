// copyright
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <fstream>
#include <iostream>
#include <utility>

constexpr size_t kMaxThreads = 2;
constexpr size_t kMaxBufferSize = 1024;
constexpr char *kFileName = "message.txt";


class MessageLogger {
 public:
  MessageLogger(): running_{true} {
    for (size_t i = 0; i < kMaxThreads; ++i) {
      threads_.emplace_back(&MessageLogger::writeThread, this);
    }
  }

  ~MessageLogger() {
    running_ = false;
    cv_.notify_all();
    for (auto& thread : threads_) {
      if (thread.joinable()) {
        thread.join();
      }
    }
    flushBuffer();
  }

  void saveMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (write_buffer_.size() + message.size() + 1 > kMaxBufferSize) {
      std::swap(write_buffer_, flush_buffer_);
      cv_.notify_one();
      write_buffer_.clear();
    }
    write_buffer_ += message + "\n";
  }

  void printMessage() {
    std::ifstream file(kFileName);
    if (!file.is_open()) {
      std::cerr << "(Failed to open file)" << std::endl;
      return;
    }
    std::string line;
    while (std::getline(file, line)) {
      std::cout << line << std::endl;
    }

    file.close();
  }

 private:
  void writeThread() {
    while (running_) {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this] { return !flush_buffer_.empty() || !running_; });

      if (!flush_buffer_.empty()) {
        std::string buffer_to_write = std::move(flush_buffer_);
        flush_buffer_.clear();
        lock.unlock();

        // 写入文件
        std::ofstream file(kFileName, std::ios::app);
        if (file.is_open()) {
          file << buffer_to_write;
          file.close();
        } else {
          std::cerr << "Failed to open file" << std::endl;
        }
      }
    }
  }

  void flushBuffer() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!write_buffer_.empty()) {
      std::swap(write_buffer_, flush_buffer_);
      lock.unlock();

      std::ofstream file(kFileName, std::ios::app);
      if (file.is_open()) {
        file << flush_buffer_;
        file.close();
      } else {
        std::cerr << "Failed to open file" << std::endl;
      }
      flush_buffer_.clear();
    }
  }
  std::vector<std::thread> threads_;
  std::string write_buffer_;
  std::string flush_buffer_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::atomic<bool> running_{true};
};
