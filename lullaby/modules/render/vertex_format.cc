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

#include "lullaby/modules/render/vertex_format.h"

#include "lullaby/util/logging.h"

namespace lull {
namespace {

inline bool operator==(const VertexAttribute& a, const VertexAttribute& b) {
  return (memcmp(&a, &b, sizeof(a)) == 0);
}

}  // namespace

constexpr uint32_t VertexFormat::kAlignment;

const VertexAttribute& VertexFormat::GetAttributeAt(size_t index) const {
  CHECK(index < num_attributes_) << "Index out of bounds!";
  return attributes_[index];
}

const VertexAttribute* VertexFormat::GetAttributeWithUsage(
    VertexAttribute::Usage usage, int usage_index) const {
  for (size_t i = 0; i < num_attributes_; ++i) {
    const VertexAttribute& attribute = attributes_[i];
    if (attribute.usage == usage && attribute.index == usage_index) {
      return &attribute;
    }
  }
  return nullptr;
}

bool VertexFormat::operator==(const VertexFormat& rhs) const {
  bool equal = (vertex_size_ == rhs.vertex_size_ &&
                num_attributes_ == rhs.num_attributes_);

  for (size_t i = 0; equal && i < num_attributes_; ++i) {
    equal = (attributes_[i] == rhs.attributes_[i]);
  }

  return equal;
}

uint32_t VertexFormat::GetSize(VertexAttribute::Type type) {
  switch (type) {
    case VertexAttribute::kUnsignedInt8:
      return 1;
    case VertexAttribute::kUnsignedInt16:
      return 2;
    case VertexAttribute::kFloat32:
      return 4;
    default:
      LOG(DFATAL) << "Invalid vertex attribute type";
      return 0;
  }
}

}  // namespace lull
