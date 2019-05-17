/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_RENDER_HANDLE_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_RENDER_HANDLE_H_

#include <stdint.h>

namespace lull {

/// A simple pointer-like wrapper around low-level GL handles like GLint and
/// GLuint.
template <typename T, T kInvalidValue = 0>
class RenderHandle {
 public:
  RenderHandle() {}
  RenderHandle(T handle)
      : handle_(handle) {}

  /// Dereferences the underlying handle.
  T operator*() const { return handle_; }

  /// Allows handle to be used in boolean operations.
  explicit operator bool() const { return handle_ != kInvalidValue; }

  /// Returns the underlying handle.
  T Get() const { return handle_; }

  /// Returns true if the underlying handle is valid.
  bool Valid() const { return handle_ != kInvalidValue; }

  /// Resets the handle to an invalid value.
  void Reset() { handle_ = kInvalidValue; }

 private:
  T handle_ = kInvalidValue;
};

/// Type-aliases for standard GL handles.
using BufferHnd = RenderHandle<uint32_t, 0>;
using ShaderHnd = RenderHandle<uint32_t, 0>;
using ProgramHnd = RenderHandle<uint32_t, 0>;
using UniformBufferHnd = RenderHandle<uint32_t, 0>;
using UniformHnd = RenderHandle<int32_t, -1>;
using TextureHnd = RenderHandle<uint32_t, 0>;

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_RENDER_HANDLE_H_
