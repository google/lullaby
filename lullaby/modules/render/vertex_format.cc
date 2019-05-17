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

#include "lullaby/modules/render/vertex_format.h"

#include <ostream>

#include "lullaby/util/logging.h"

namespace lull {
namespace {

inline bool operator==(const VertexAttribute& a, const VertexAttribute& b) {
  return (a.usage() == b.usage() && a.type() == b.type());
}
inline bool operator!=(const VertexAttribute& a, const VertexAttribute& b) {
  return (a.usage() != b.usage() || a.type() != b.type());
}

}  // namespace

const VertexAttribute* VertexFormat::GetAttributeAt(size_t index) const {
  if (index < num_attributes_) {
    return &attributes_[index];
  } else {
    return nullptr;
  }
}

void VertexFormat::AppendAttribute(const VertexAttribute& attribute) {
  if (num_attributes_ == kMaxAttributes) {
    LOG(DFATAL) << "Cannot exceed max attributes size of " << kMaxAttributes;
    return;
  }
  attributes_[num_attributes_] = attribute;
  vertex_size_ += GetAttributeSize(attribute);
  ++num_attributes_;
}

const VertexAttribute* VertexFormat::GetAttributeWithUsage(
    VertexAttributeUsage usage, int n) const {
  for (size_t i = 0; i < num_attributes_; ++i) {
    if (attributes_[i].usage() == usage) {
      if (n == 0) {
        return &attributes_[i];
      } else {
        --n;
      }
    }
  }
  return nullptr;
}

size_t VertexFormat::GetAttributeOffsetAt(size_t index) const {
  CHECK_LT(index, num_attributes_);
  size_t offset = 0;
  for (size_t i = 0; i < index; ++i) {
    offset += GetAttributeSize(attributes_[i]);
  }
  return offset;
}

size_t VertexFormat::GetAttributeOffset(
    const VertexAttribute* attribute) const {
  CHECK_GE(attribute, &attributes_[0]);
  CHECK_LT(attribute, &attributes_[num_attributes_]);

  const size_t index = attribute - &attributes_[0];
  return GetAttributeOffsetAt(index);
}

bool VertexFormat::operator==(const VertexFormat& rhs) const {
  if (vertex_size_ != rhs.vertex_size_) {
    return false;
  }
  if (num_attributes_ != rhs.num_attributes_) {
    return false;
  }
  for (size_t i = 0; i < num_attributes_; ++i) {
    if (attributes_[i] != rhs.attributes_[i]) {
      return false;
    }
  }
  return true;
}

bool VertexFormat::operator!=(const VertexFormat& rhs) const {
  return !(*this == rhs);
}

size_t VertexFormat::GetAttributeSize(const VertexAttribute& attr) {
  switch (attr.type()) {
    case VertexAttributeType_Scalar1f:
      return 1 * sizeof(float);
    case VertexAttributeType_Vec2f:
      return 2 * sizeof(float);
    case VertexAttributeType_Vec3f:
      return 3 * sizeof(float);
    case VertexAttributeType_Vec4f:
      return 4 * sizeof(float);
    case VertexAttributeType_Vec2us:
      return 2 * sizeof(uint16_t);
    case VertexAttributeType_Vec4ub:
      return 4 * sizeof(uint8_t);
    case VertexAttributeType_Empty:
      return 0;
    default:
      LOG(DFATAL) << "Unsupported attrib type: " << attr.type();
      return 0;
  }
}

std::ostream& operator<<(std::ostream& os, const VertexFormat& vf) {
  for (size_t i = 0; i < vf.GetNumAttributes(); ++i) {
    const auto* attribute = vf.GetAttributeAt(i);
    os << i << ": " << EnumNameVertexAttributeType(attribute->type()) << " "
       << EnumNameVertexAttributeUsage(attribute->usage()) << std::endl;
  }
  return os;
}

}  // namespace lull
