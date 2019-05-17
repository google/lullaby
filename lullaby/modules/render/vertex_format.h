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

#ifndef LULLABY_MODULES_RENDER_VERTEX_FORMAT_H_
#define LULLABY_MODULES_RENDER_VERTEX_FORMAT_H_

#include <cstddef>
#include <initializer_list>
#include <ostream>

#include "lullaby/util/math.h"
#include "lullaby/generated/vertex_attribute_def_generated.h"

namespace lull {

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

  // Appends the specified attribute to the internal list of attributes.
  void AppendAttribute(const VertexAttribute& attribute);

  size_t GetNumAttributes() const { return num_attributes_; }

  // Returns the attribute at the specified index if valid, else nullptr.
  const VertexAttribute* GetAttributeAt(size_t index) const;

  // Returns the n'th attribute which has the specified |usage|, else nullptr.
  const VertexAttribute* GetAttributeWithUsage(VertexAttributeUsage usage,
                                               int n = 0) const;

  // Returns the size of a single vertex.
  size_t GetVertexSize() const { return vertex_size_; }

  // Returns the offset of the attribute at |index|.
  size_t GetAttributeOffsetAt(size_t index) const;

  // Returns |attribute|'s offset within the vertex.
  size_t GetAttributeOffset(const VertexAttribute* attribute) const;

  // Returns the size of a vertex attribute.
  static size_t GetAttributeSize(const VertexAttribute& attr);

  // Queries whether a specific vertex type matches this format.
  template <typename Vertex>
  bool Matches() const;

  // Tests if the structures described by two VertexFormats are equal.
  bool operator==(const VertexFormat& rhs) const;
  bool operator!=(const VertexFormat& rhs) const;

 private:
  static constexpr uint32_t kAlignment = 4;

  // This class's data must be restricted to POD, as we rely on static const
  // VertexFormats for dynamic rendering.
  VertexAttribute attributes_[kMaxAttributes];
  size_t num_attributes_ = 0;
  size_t vertex_size_ = 0;
};

template <typename Iterator>
VertexFormat::VertexFormat(Iterator begin, Iterator end) {
  for (auto attrib = begin; attrib != end; ++attrib) {
    AppendAttribute(*attrib);
  }
}

template <typename Vertex>
bool VertexFormat::Matches() const {
  return (sizeof(Vertex) == vertex_size_ &&
          (this == &Vertex::kFormat || *this == Vertex::kFormat));
}

std::ostream& operator<<(std::ostream& os, const VertexFormat& vf);

}  // namespace lull

#endif  // LULLABY_MODULES_RENDER_VERTEX_FORMAT_H_
