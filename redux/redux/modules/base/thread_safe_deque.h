/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#ifndef REDUX_MODULES_BASE_THREAD_SAFE_DEQUE_H_
#define REDUX_MODULES_BASE_THREAD_SAFE_DEQUE_H_

#include <algorithm>
#include <deque>
#include <optional>

#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"

namespace redux {

// Thread-safe wrapper around std::deque.
//
// The Push/Pop functions work as expected for a deque. A WaitPopFront function
// is also provided which blocks the calling thread until an element is
// available to be popped.
template <typename T>
class ThreadSafeDeque {
 public:
  ThreadSafeDeque() = default;

  ThreadSafeDeque(const ThreadSafeDeque&) = delete;
  ThreadSafeDeque& operator=(const ThreadSafeDeque&) = delete;

  // Adds an object onto the back the deque.
  void PushBack(T obj) {
    absl::MutexLock lock(&mutex_);
    deque_.push_back(std::move(obj));
  }

  // Adds an object to the front of the deque.
  void PushFront(T obj) {
    absl::MutexLock lock(&mutex_);
    deque_.push_front(std::move(obj));
  }

  // Attempts to remove an element from the front of the deque by returning it.
  // Returns nullopt if the deque is empty.
  std::optional<T> TryPopFront() {
    std::optional<T> ret = std::nullopt;
    if (absl::MutexLock lock(&mutex_); !deque_.empty()) {
      ret = std::move(deque_.front());
      deque_.pop_front();
    }
    return ret;
  }

  // Attempts to remove an element from the back of the deque by returning it.
  // Returns nullopt if the deque is empty.
  std::optional<T> TryPopBack() {
    std::optional<T> ret = std::nullopt;
    if (absl::MutexLock lock(&mutex_); !deque_.empty()) {
      ret = std::move(deque_.back());
      deque_.pop_back();
    }
    return ret;
  }

  // Pops and returns the front element from the deque.  This function will
  // block the calling thread until an element is available to be popped.
  T WaitPopFront() {
    absl::MutexLock lock(&mutex_, absl::Condition(IsNonEmpty, &deque_));
    T obj = std::move(deque_.front());
    deque_.pop_front();
    return obj;
  }

  // Pops and returns the back element from the deque.  This function will block
  // the calling thread until an element is available to be popped.
  T WaitPopBack() {
    absl::MutexLock lock(&mutex_, absl::Condition(IsNonEmpty, &deque_));
    T obj = std::move(deque_.back());
    deque_.pop_back();
    return obj;
  }

  // Removes all entries for which `cond(T)` returns true.
  template <typename Fn>
  void RemoveIf(Fn cond) {
    absl::MutexLock lock(&mutex_);
    deque_.erase(std::remove_if(deque_.begin(), deque_.end(), cond),
                 deque_.end());
  }

 private:
  static bool IsNonEmpty(std::deque<T>* d) { return !d->empty(); }

  std::deque<T> deque_ ABSL_GUARDED_BY(mutex_);
  absl::Mutex mutex_;
};

}  // namespace redux


#endif  // REDUX_MODULES_BASE_THREAD_SAFE_DEQUE_H_
