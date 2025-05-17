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

#ifndef REDUX_TOOLS_SCENE_PIPELINE_VARIANT_H_
#define REDUX_TOOLS_SCENE_PIPELINE_VARIANT_H_

#include <cstddef>
#include <initializer_list>
#include <memory>

#include "redux/tools/scene_pipeline/type_id.h"
#include "absl/types/span.h"

namespace redux::tool {

// A typesafe variant for holding trivially copyable values
class Variant {
 public:
  Variant() = default;

  // Creates a Variant with the given value.
  template <typename T>
  explicit Variant(const T& value) {
    Set(value);
  }

  // Creates a Variant with the given values.
  template <typename T>
  Variant(const std::initializer_list<T>& values) {
    Set(values);
  }

  // Sets the value of the variant.
  template <typename T>
  void Set(const T& value) {
    Assign<T>(absl::Span<const T>(&value, 1));
  }

  // Sets the values of the variant.
  template <typename T>
  void Set(const std::initializer_list<T>& values) {
    Assign<T>(absl::Span<const T>(values.begin(), std::size(values)));
  }

  // Returns the TypeId of the type of value stored in the variant.
  TypeId type() const { return type_; }

  // Returns the number of elements in the variant.
  int size() const { return count_; }

  // Returns a raw byte pointer to the variant data.
  const std::byte* data() const { return payload_.get(); }

  // Clears the variant of all data.
  void reset() { *this = Variant(); }

  // Returns true if the variant contains the given type.
  template <typename T>
  bool is() const {
    return Is<T>(type_);
  }

  // Returns the span of the given type of the data stored in the variant. If
  // the variant does not contain the given type, then an empty span is
  // returned.
  template <typename T>
  absl::Span<const T> span() const {
    if (!is<T>()) {
      return {};
    }

    const T* ptr = reinterpret_cast<const T*>(data());
    return absl::Span<const T>(ptr, count_);
  }

 private:
  template <typename T>
  void Assign(absl::Span<const T> values) {
    static_assert(std::is_trivially_copyable_v<T>);

    type_ = GetTypeId<T>();
    count_ = values.size();
    const size_t num_bytes = sizeof(T) * values.size();
    payload_ = std::make_unique<std::byte[]>(num_bytes);
    memcpy(payload_.get(), values.data(), num_bytes);
  }

  TypeId type_ = kInvalidTypeId;
  std::unique_ptr<std::byte[]> payload_;
  int count_ = 0;  // >1 for arrays
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SCENE_PIPELINE_VARIANT_H_
