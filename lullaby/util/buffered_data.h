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

#ifndef LULLABY_UTIL_BUFFERED_DATA_H_
#define LULLABY_UTIL_BUFFERED_DATA_H_

#include <algorithm>
#include <cstddef>
#include <mutex>

#include "lullaby/util/logging.h"

namespace lull {

/// The Buffered Data class holds |N| instances of a data type |T|. This is
/// intended to be used in a single-producer, single-consumer pattern, allowing
/// two threads to manipulate data, usually with one being a writer and the
/// other processing the written data.
///
/// In the multi-threaded use case (N > 1), this is used as such:
/// - Write thread locks an old buffer of the data by calling |LockWriteBuffer|.
///
/// - Meanwhile, processing thread locks the most up to date set of data by
/// calling |LockReadBuffer|.
///
/// - When writing is finished, call |UnlockWriteBuffer|. This will set the
/// newly unlocked buffer as the read, "fresh" buffer.
///
/// - When done processing, call |UnlockReadBuffer|.
///
/// - Repeat the sequence. The next time |LockReadBuffer| is called, it should
/// get the new version of the data submitted by the write thread.
///
/// Note that some cases may cause the |LockReadBuffer| call to return stale
/// data. This is usually caused when the number of buffers |N| is 2 and the
/// writer thread started processing a buffer right after submitting one and
/// while the process thread still has a lock on its buffer. If this case is a
/// common scenario, then consider using at least 3 buffers.
///
/// In a single threaded use case (N == 1), use as follows:
/// - Lock the buffer for writing by calling |LockWriteBuffer|.
/// - Write data into the buffer.
/// - Unlock the buffer by calling |UnlockWriteBuffer|.
/// - Lock the buffer for processing by calling |LockReadBuffer|.
/// - Process the data.
/// - Unlock the buffer by calling |UnlockReadBuffer|.
/// - Repeat.
template <typename T, size_t N = 2>
class BufferedData {
 public:
  BufferedData() {
    static_assert(N != 0, "BufferedData cannot have 0 buffers!");

    // Initialize indices for the least recently used list.
    for (size_t i = 0; i < N; ++i) {
      lru_[i] = i;
    }
  }

  /// Locks and retrieves the read buffer, which should be the most up to date.
  /// The buffer must not already be locked before calling this function. If
  /// there is no buffer available for locking, an assertion will be caused.
  ///
  /// @return Returns the most up to data buffer available for processing. Once
  /// done processing, the buffer should be unlocked by calling
  /// |UnlockReadBuffer|.
  T* LockReadBuffer() {
    std::unique_lock<std::mutex> lock(mutex_);

    DCHECK_EQ(locked_read_, N) << "Read buffer already locked.";
    DCHECK(locked_write_ == N || N > 1);

    // Attempt using the front buffer.
    size_t front = lru_[0];
    if (front == locked_write_) {
      if (N == 1) {
        return nullptr;
      }
      // The front buffer is already locked, so use the next one.
      front = lru_[1];
    }

    locked_read_ = front;
    return &data_[front];
  }

  /// Unlocks the read buffer and frees it for writing new data.
  void UnlockReadBuffer() {
    std::unique_lock<std::mutex> lock(mutex_);

    DCHECK_NE(locked_read_, N) << "Read buffer was not locked!";
    locked_read_ = N;
  }

  /// Locks a stale data buffer for writing. This may return the most up to date
  /// buffer if there are less than 3 buffers and the up to date buffer is the
  /// only buffer available for writing.
  ///
  /// @return Returns the oldest data buffer available for writing. Once done
  /// processing, the buffer should be unlocked by calling |UnlockWriteBuffer|.
  T* LockWriteBuffer() {
    std::unique_lock<std::mutex> lock(mutex_);

    DCHECK_EQ(locked_write_, N) << "Write buffer already locked.";
    DCHECK(locked_read_ == N || N > 1);

    size_t back = lru_[N - 1];
    if (back == locked_read_) {
      if (N == 1) {
        return nullptr;
      }
      back = lru_[N - 2];
    }

    locked_write_ = back;
    return &data_[back];
  }

  /// Unlocks the currently designated write buffer and promotes it to be the
  /// read buffer.
  void UnlockWriteBuffer() {
    std::unique_lock<std::mutex> lock(mutex_);

    DCHECK_NE(locked_write_, N) << "Write buffer was not locked!";

    // Bring the write buffer to the front, maintaining the order of the array.
    for (size_t i = (N - 1); i > 0; --i) {
      if (lru_[i] == locked_write_) {
        std::swap(lru_[i], lru_[i - 1]);
      }
    }
    locked_write_ = N;
  }

 private:
  /// Array of the data stored.
  T data_[N];
  /// Least recently used list, designating the order the buffers were updated.
  size_t lru_[N];
  /// Mutex used to safeguard the LRU list.
  std::mutex mutex_;
  /// Index of the buffer locked as the read buffer.
  size_t locked_read_ = N;
  /// Index of the buffer locked as the write buffer.
  size_t locked_write_ = N;
};

}  // namespace lull

#endif  // LULLABY_UTIL_BUFFERED_DATA_H_
