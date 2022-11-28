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

#include "redux/modules/graphics/vertex_format.h"

#include "redux/modules/base/logging.h"

namespace redux {

const VertexAttribute* VertexFormat::GetAttributeAt(std::size_t index) const {
  if (index < num_attributes_) {
    return &attributes_[index];
  } else {
    return nullptr;
  }
}

void VertexFormat::AppendAttribute(const VertexAttribute& attribute) {
  CHECK_LT(num_attributes_, kMaxAttributes)
      << "Cannot exceed max attributes size of " << kMaxAttributes;
  attributes_[num_attributes_] = attribute;
  vertex_size_ += GetAttributeSize(attribute);
  ++num_attributes_;
}

const VertexAttribute* VertexFormat::GetAttributeWithUsage(
    VertexUsage usage) const {
  for (std::size_t i = 0; i < num_attributes_; ++i) {
    if (attributes_[i].usage == usage) {
      return &attributes_[i];
    }
  }
  return nullptr;
}

std::size_t VertexFormat::GetAttributeOffsetAt(std::size_t index) const {
  CHECK_LT(index, num_attributes_);
  std::size_t offset = 0;
  for (std::size_t i = 0; i < index; ++i) {
    offset += GetAttributeSize(attributes_[i]);
  }
  return offset;
}

std::size_t VertexFormat::GetAttributeOffset(
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
  for (std::size_t i = 0; i < num_attributes_; ++i) {
    if (attributes_[i] != rhs.attributes_[i]) {
      return false;
    }
  }
  return true;
}

bool VertexFormat::operator!=(const VertexFormat& rhs) const {
  return !(*this == rhs);
}

std::size_t VertexFormat::GetAttributeSize(const VertexAttribute& attr) {
  switch (attr.type) {
    case VertexType::Scalar1f:
      return 1 * sizeof(float);
    case VertexType::Vec2f:
      return 2 * sizeof(float);
    case VertexType::Vec3f:
      return 3 * sizeof(float);
    case VertexType::Vec4f:
      return 4 * sizeof(float);
    case VertexType::Vec2us:
      return 2 * sizeof(uint16_t);
    case VertexType::Vec4ub:
      return 4 * sizeof(uint8_t);
    case VertexType::Vec4us:
      return 4 * sizeof(uint16_t);
    default:
      LOG(FATAL) << "Unsupported attrib type: " << ToString(attr.type);
  }
}
}  // namespace redux
