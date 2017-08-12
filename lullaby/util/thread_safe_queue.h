/*
Copyright 2017 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef LULLABY_UTIL_THREAD_SAFE_QUEUE_H_
#define LULLABY_UTIL_THREAD_SAFE_QUEUE_H_

#include <condition_variable>
#include <mutex>
#include <queue>

#include "lullaby/util/logging.h"

namespace lull {

// The ThreadSafeQueue is a simple wrapper around std::queue /
// std::priority_queue which provides thread-safe queue operations.  The
// Enqueue/Dequeue functions work as expected for a Queue.  A WaitDequeue
// function is also provided which blocks the calling thread until an element is
// available to be dequeued.
template <typename T, typename Q = std::queue<T>>
class ThreadSafeQueue {
  typedef std::unique_lock<std::mutex> Lock;

 public:
  ThreadSafeQueue() {}

  ThreadSafeQueue(const ThreadSafeQueue&) = delete;
  ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

  // Enqueues an object into the queue.
  void Enqueue(T obj) {
    Lock lock(mutex_);
    queue_.push(std::move(obj));
    condvar_.notify_one();
  }

  // Dequeues the next element in the queue by moving it into the object as
  // specified by |out| and returns true.  If the queue is empty, the function
  // does not modify the |out| parameter and returns false.
  bool Dequeue(T* out) {
    Lock lock(mutex_);
    if (queue_.empty()) {
      return false;
    }
    if (out != nullptr) {
      *out = std::move(GetFront(&queue_));
    }
    queue_.pop();
    return true;
  }

  // Dequeues the next element in the queue.  This function will block the
  // calling thread until an element is available to be dequeued.
  T WaitDequeue() {
    Lock lock(mutex_);
    while (queue_.empty()) {
      condvar_.wait(lock);
    }

    T obj = std::move(GetFront(&queue_));
    queue_.pop();
    return obj;
  }

  // Reports whether the queue is empty or not.
  bool Empty() const {
    Lock lock(mutex_);
    return queue_.empty();
  }

 private:
  template <typename U>
  T& GetFront(U* queue) {
    return queue->front();
  }

  T GetFront(std::priority_queue<T>* queue) {
    return queue->top();
  }

  Q queue_;
  mutable std::mutex mutex_;
  std::condition_variable condvar_;
};

}  // namespace lull

#endif  // LULLABY_UTIL_THREAD_SAFE_QUEUE_H_
