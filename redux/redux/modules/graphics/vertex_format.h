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

#ifndef REDUX_MODULES_GRAPHICS_VERTEX_FORMAT_H_
#define REDUX_MODULES_GRAPHICS_VERTEX_FORMAT_H_

#include <cstddef>
#include <initializer_list>

#include "redux/modules/graphics/vertex_attribute.h"

namespace redux {

// Describes the layout of the data in a vertex structure.
//
// This information is used by the GPU to interpret the vertex stream and align
// the attributes with their respective shader inputs.
//
// Attributes are sorted by offset.  Vertex size will be padded as necessary.
class VertexFormat {
 public:
  enum { kMaxAttributes = 12 };

  VertexFormat() = default;

  template <typename Iterator>
  VertexFormat(Iterator begin, Iterator end);

  VertexFormat(std::initializer_list<VertexAttribute> attributes)
      : VertexFormat(std::begin(attributes), std::end(attributes)) {}

  // Appends the specified attribute to the internal list of attributes.
  void AppendAttribute(const VertexAttribute& attribute);

  // Returns the number of attributes in this format.
  std::size_t GetNumAttributes() const { return num_attributes_; }

  // Returns the attribute at the specified index if valid, else nullptr.
  const VertexAttribute* GetAttributeAt(std::size_t index) const;

  // Returns the attribute withthe specified |usage|, else nullptr.
  const VertexAttribute* GetAttributeWithUsage(VertexUsage usage) const;

  // Returns the size of a single vertex.
  std::size_t GetVertexSize() const { return vertex_size_; }

  // Returns the offset of the attribute at |index|.
  std::size_t GetAttributeOffsetAt(std::size_t index) const;

  // Returns |attribute|'s offset within the vertex.
  std::size_t GetAttributeOffset(const VertexAttribute* attribute) const;

  // Returns the size of a vertex attribute.
  static std::size_t GetAttributeSize(const VertexAttribute& attr);

  // Tests if the structures described by two VertexFormats are equal.
  bool operator==(const VertexFormat& rhs) const;
  bool operator!=(const VertexFormat& rhs) const;

 private:
  VertexAttribute attributes_[kMaxAttributes];
  std::size_t num_attributes_ = 0;
  std::size_t vertex_size_ = 0;
};

template <typename Iterator>
VertexFormat::VertexFormat(Iterator begin, Iterator end) {
  for (auto attrib = begin; attrib != end; ++attrib) {
    AppendAttribute(*attrib);
  }
}

}  // namespace redux

#endif  // REDUX_MODULES_GRAPHICS_VERTEX_FORMAT_H_
