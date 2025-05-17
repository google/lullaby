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

#include <stddef.h>
#include <stdint.h>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "redux/modules/base/logging.h"

namespace redux {

void VertexFormat::AppendAttribute(const VertexAttribute& attribute) {
  AppendAttribute(attribute, vertex_size_, 0);
}

void VertexFormat::AppendAttribute(const VertexAttribute& attribute,
                                   std::size_t offset,
                                   std::size_t byte_stride) {
  CHECK_LT(num_attributes_, kMaxAttributes)
      << "Cannot exceed max attributes size of " << kMaxAttributes;
  attributes_[num_attributes_].attrib = attribute;
  attributes_[num_attributes_].offset = offset;
  attributes_[num_attributes_].byte_stride = byte_stride;
  vertex_size_ += GetVertexTypeSize(attribute.type);
  ++num_attributes_;
}

const VertexAttribute* VertexFormat::GetAttributeAt(std::size_t index) const {
  if (index < num_attributes_) {
    return &attributes_[index].attrib;
  } else {
    return nullptr;
  }
}

std::size_t VertexFormat::GetOffsetOfAttributeAt(std::size_t index) const {
  CHECK_LT(index, num_attributes_) << "Invalid index: " << index;
  return attributes_[index].offset;
}

std::size_t VertexFormat::GetStrideOfAttributeAt(std::size_t index) const {
  CHECK_LT(index, num_attributes_) << "Invalid index: " << index;
  const std::size_t byte_stride = attributes_[index].byte_stride;
  return byte_stride != 0 ? byte_stride : vertex_size_;
}

bool VertexFormat::operator==(const VertexFormat& rhs) const {
  if (vertex_size_ != rhs.vertex_size_) {
    return false;
  }
  if (num_attributes_ != rhs.num_attributes_) {
    return false;
  }
  for (std::size_t i = 0; i < num_attributes_; ++i) {
    if (attributes_[i].attrib != rhs.attributes_[i].attrib) {
      return false;
    }
    if (attributes_[i].offset != rhs.attributes_[i].offset) {
      return false;
    }
    if (attributes_[i].byte_stride != rhs.attributes_[i].byte_stride) {
      return false;
    }
  }
  return true;
}

bool VertexFormat::operator!=(const VertexFormat& rhs) const {
  return !(*this == rhs);
}

std::size_t VertexFormat::GetVertexTypeSize(const VertexType& type) {
  switch (type) {
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
      LOG(FATAL) << "Unsupported attrib type: " << ToString(type);
  }
}
}  // namespace redux
