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

#ifndef REDUX_ENGINES_PLATFORM_BUFFERED_STATE_H_
#define REDUX_ENGINES_PLATFORM_BUFFERED_STATE_H_

namespace redux {
namespace detail {

// A container of type T that acts like a "triple-buffer".
//
// One way to picture this container is like a 3-element queue. The "top" of the
// queue is mutable/writable. Once the user is done mutating the element, it can
// be "committed". This will push the element further down into the container,
// making it the current read-only element. The previous read-only element
// gets pushed back as well, and the one prior to that is removed. And a new
// writable/mutable element is placed at the top.
template <typename T>
class BufferedState {
 public:
  BufferedState() = default;

  // Initializes all internal states to the given |reference_state|.
  void Initialize(const T& reference_state) {
    write_buffer_ = reference_state;
    for (T& state : read_buffers_) {
      state = reference_state;
    }
  }

  // Makes the active mutable value the new "current" read-only value, pushing
  // the other read-only elements down one-level of history. The new mutable
  // element will contain stale data and should be re-written in its entirety.
  void Commit() {
    // Set the current buffer to be the new previous buffer.
    curr_index_ = curr_index_ ? 0 : 1;
    // Copy the contents of the write buffer into the current read buffer.
    read_buffers_[curr_index_] = write_buffer_;
  }

  // Returns the reference to writable state.
  T& GetMutable() { return write_buffer_; }

  // Returns a read-only reference to most recent state.
  const T& GetCurrent() const { return read_buffers_[curr_index_]; }

  // Returns a read-only reference to previous state.
  const T& GetPrevious() const { return read_buffers_[curr_index_ ? 0 : 1]; }

 private:
  static constexpr int kNumReadBuffers = 2;
  T read_buffers_[kNumReadBuffers];
  T write_buffer_;
  int curr_index_ = 0;
};

}  // namespace detail
}  // namespace redux

#endif  // REDUX_ENGINES_PLATFORM_BUFFERED_STATE_H_
