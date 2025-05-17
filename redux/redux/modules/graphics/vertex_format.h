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
#include <iterator>

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
  void AppendAttribute(const VertexAttribute& attribute, std::size_t offset,
                       std::size_t byte_stride);

  // Returns the number of attributes in this format.
  std::size_t GetNumAttributes() const { return num_attributes_; }

  // Returns the attribute at the specified index if valid, else nullptr.
  const VertexAttribute* GetAttributeAt(std::size_t index) const;

  // Returns the offset of the vertex attribute at the specified index.
  std::size_t GetOffsetOfAttributeAt(std::size_t index) const;

  // Returns the stride of the vertex attribute at the specified index.
  std::size_t GetStrideOfAttributeAt(std::size_t index) const;

  // Returns the size of a single vertex.
  std::size_t GetVertexSize() const { return vertex_size_; }

  // Returns the size of a vertex attribute.
  static std::size_t GetVertexTypeSize(const VertexType& type);

  // Tests if the structures described by two VertexFormats are equal.
  bool operator==(const VertexFormat& rhs) const;
  bool operator!=(const VertexFormat& rhs) const;

 private:
  struct Attribute {
    VertexAttribute attrib;
    std::size_t offset = 0;
    std::size_t byte_stride = 0;
  };

  Attribute attributes_[kMaxAttributes];
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
