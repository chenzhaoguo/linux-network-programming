#ifndef LINUX_NETWORK_PROGRAMMING_EXAMPLES_BLOCK_QUEUE_HPP_
#define LINUX_NETWORK_PROGRAMMING_EXAMPLES_BLOCK_QUEUE_HPP_

#include <queue>
#include <mutex>
#include <condition_variable>

template <class T>
class BlockQueue {
 public:
  void Push(const T&& t) {
    std::unique_lock<std::mutex> lock(locker_);
    queue_.push(t);
    cv_.notify_one();
  }
  T Pop() {
    std::unique_lock<std::mutex> lock(locker_);
    while(queue_.size()== 0) {
      cv_.wait(lock);
    }
    return queue_.pop();
  }
 private:
  std::queue<T> queue_;
  std::mutex locker_;
  std::condition_variable cv_;
};

#endif //LINUX_NETWORK_PROGRAMMING_EXAMPLES_BLOCK_QUEUE_HPP_
