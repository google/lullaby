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

#ifndef LULLABY_UTIL_VERTEX_FORMAT_H_
#define LULLABY_UTIL_VERTEX_FORMAT_H_

#include <stddef.h>

#include <initializer_list>

#include "lullaby/util/math.h"

namespace lull {

// A VertexAttribute describes the location and format of a single vertex
// element within the vertex struct.
struct VertexAttribute {
  enum Usage { kPosition, kTexCoord, kColor, kIndex, kNormal };
  enum Type {
    kUnsignedInt8,
    kUnsignedInt16,
    kFloat32,
  };

  VertexAttribute() {}
  VertexAttribute(int offset, Usage usage, int count, Type type)
      : offset(offset), usage(usage), count(count), type(type) {}
  VertexAttribute(int offset, Usage usage, int count, Type type, int index)
      : offset(offset), usage(usage), count(count), type(type), index(index) {}

  int offset = 0;
  Usage usage = kPosition;
  int count = 0;
  Type type = kFloat32;
  int index = 0;
};

// A VertexFormat details all data within a vertex structure.  This is needed to
// instruct the GL how to interpret the vertex stream and align the attributes
// with their respective shader inputs.
//
// Attributes are sorted by offset.  Vertex size will be padded as necessary.
class VertexFormat {
 public:
  enum { kMaxAttributes = 12 };

  VertexFormat() {}

  template <typename Iterator>
  VertexFormat(Iterator begin, Iterator end);

  VertexFormat(std::initializer_list<VertexAttribute> attributes)
      : VertexFormat(std::begin(attributes), std::end(attributes)) {}

  size_t GetNumAttributes() const { return num_attributes_; }

  const VertexAttribute& GetAttributeAt(size_t index) const;

  // Returns the attribute which has both |usage| and |usage_index|, else
  // nullptr.
  const VertexAttribute* GetAttributeWithUsage(VertexAttribute::Usage usage,
                                               int usage_index = 0) const;

  // Returns the size of a single vertex, padded out to kAlignment.
  size_t GetVertexSize() const { return vertex_size_; }

  // Queries whether a specific vertex type matches this format.
  template <typename Vertex>
  bool Matches() const;

  // Tests if the structures described by two VertexFormats are equal.
  bool operator==(const VertexFormat& rhs) const;

 private:
  static constexpr uint32_t kAlignment = 4;

  static uint32_t GetSize(VertexAttribute::Type type);

  // This class's data must be restricted to POD, as we rely on static const
  // VertexFormats for dynamic rendering.
  VertexAttribute attributes_[kMaxAttributes];
  size_t num_attributes_ = 0;
  size_t vertex_size_ = 0;
};

template <typename Iterator>
VertexFormat::VertexFormat(Iterator begin, Iterator end) {
  for (auto attrib = begin; attrib != end; ++attrib) {
    if (num_attributes_ == kMaxAttributes) {
      LOG(DFATAL) << "Cannot exceed max attributes size of " << kMaxAttributes;
      return;
    }

    DCHECK_EQ(attrib->offset % kAlignment, 0U)
        << "Misaligned vertex attribute; offset: " << attrib->offset
        << ", usage: " << attrib->usage;
    attributes_[num_attributes_] = *attrib;
    ++num_attributes_;
  }

  if (num_attributes_ > 0) {
    std::sort(attributes_, attributes_ + num_attributes_,
              [](const VertexAttribute& a, const VertexAttribute& b) {
                return a.offset < b.offset;
              });

    const VertexAttribute& last = attributes_[num_attributes_ - 1];
    vertex_size_ = last.offset +
                   AlignToPowerOf2(last.count * GetSize(last.type), kAlignment);
  }
}

template <typename Vertex>
bool VertexFormat::Matches() const {
  return (sizeof(Vertex) == vertex_size_ &&
          (this == &Vertex::kFormat || *this == Vertex::kFormat));
}

}  // namespace lull

#endif  // LULLABY_UTIL_VERTEX_FORMAT_H_
