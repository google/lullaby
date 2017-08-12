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

#ifndef LULLABY_UTIL_THREAD_SAFE_DEQUE_H_
#define LULLABY_UTIL_THREAD_SAFE_DEQUE_H_

#include <condition_variable>
#include <deque>
#include <mutex>

#include "lullaby/util/logging.h"

namespace lull {

// The ThreadSafeDeque is a simple wrapper around std::deque which provides
// thread-safe operations.  The Push/Pop functions work as expected for a Deque.
// A WaitPopFront function is also provided which blocks the calling thread
// until an element is available to be popped.
template <typename T>
class ThreadSafeDeque {
  typedef std::unique_lock<std::mutex> Lock;

 public:
  ThreadSafeDeque() {}

  ThreadSafeDeque(const ThreadSafeDeque&) = delete;
  ThreadSafeDeque& operator=(const ThreadSafeDeque&) = delete;

  // Adds an object onto the back the deque.
  void PushBack(T obj) {
    Lock lock(mutex_);
    deque_.push_back(std::move(obj));
    condvar_.notify_one();
  }

  // Adds an object to the front of the deque.
  void PushFront(T obj) {
    Lock lock(mutex_);
    deque_.push_front(std::move(obj));
    condvar_.notify_one();
  }

  // Pops the front element from the deque by moving it into the object as
  // specified by |out| and returns true.  If the deque is empty, the function
  // does not modify the |out| parameter and returns false.
  bool PopFront(T* out) {
    Lock lock(mutex_);
    if (deque_.empty()) {
      return false;
    }
    if (out != nullptr) {
      *out = std::move(deque_.front());
    }
    deque_.pop_front();
    return true;
  }

  // Pops the front element from the deque.  This function will block the
  // calling thread until an element is available to be popped.
  T WaitPopFront() {
    Lock lock(mutex_);
    while (deque_.empty()) {
      condvar_.wait(lock);
    }

    T obj = std::move(deque_.front());
    deque_.pop_front();
    return obj;
  }

  // Reports whether the deque is empty or not.
  bool Empty() const {
    Lock lock(mutex_);
    return deque_.empty();
  }

  // Removes all entries for which test(entry) returns true.
  template <typename Fn>
  void RemoveIf(Fn test) {
    Lock lock(mutex_);
    auto iter = deque_.begin();
    while (iter != deque_.end()) {
      if (test(*iter)) {
        iter = deque_.erase(iter);
      } else {
        ++iter;
      }
    }
  }

 private:
  std::deque<T> deque_;
  mutable std::mutex mutex_;
  std::condition_variable condvar_;
};

}  // namespace lull

#endif  // LULLABY_UTIL_THREAD_SAFE_DEQUE_H_
