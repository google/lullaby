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

#ifndef LULLABY_MODULES_ECS_BLUEPRINT_BUILDER_H_
#define LULLABY_MODULES_ECS_BLUEPRINT_BUILDER_H_

#include <vector>

#include "flatbuffers/flatbuffers.h"
#include "lullaby/generated/flatbuffers/blueprint_def_generated.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/span.h"
#include "lullaby/util/string_view.h"

namespace lull {
namespace detail {

// Manages a FlatBufferBuilder to simplify building BlueprintDef flatbuffer
// binaries. This is an internal class not intended for public use. Instead, use
// BlueprintFromJsonCompiler for compiling from JSON or BlueprintWriter for
// writing BlueprintTrees.
//
// Example usage:
//   StartChildren()
//     AddComponent(D1)
//     AddComponent(D2)
//     FinishChild()
//     StartChildren()
//       AddComponent(C1)
//       AddComponent(C2)
//       FinishChild()
//     FinishChildren()
//     AddComponent(B1)
//     AddComponent(B2)
//     FinishChild()
//   FinishChildren()
//   AddComponent(A1)
//   AddComponent(A2)
//   Finish()
// Will create a hierarchy like:
//   A -> D
//     -> B -> C

class BlueprintBuilder {
 public:
  // The 4 letter file identifier used for the BlueprintDef flatbuffer's
  // file_identifier property.  The value is set to "BLPT".
  static const char* const kBlueprintFileIdentifier;

  BlueprintBuilder() {}

  // Adds a component to the current Entity. |def| should be the already
  // finalized flatbuffer binary of a component. |def_type| should be the
  // name of the flatbuffer ComponentDef without namespaces, e.g.
  // "TransformDef", or the Hash() of that string.
  void AddComponent(string_view def_type, Span<uint8_t> def);
  void AddComponent(HashValue def_type, Span<uint8_t> def);

  // Finishes the root Entity and return the binary flatbuffer for the whole
  // blueprint. The writer is immediately reusable to create a new Entity after
  // this.
  flatbuffers::DetachedBuffer Finish(
      const char* identifier = kBlueprintFileIdentifier);

  // Before adding any components to the current Entity, call StartChildren() to
  // start creating an array of children Entities. Multiple generations can be
  // nested, as long as all the children of an Entity are completed before the
  // components of the Entity. This acts likes "pushing" a stack, where each
  // level is one generation further down from the root, and must be matched
  // by a corresponding call to FinishChildren().
  void StartChildren();

  // Finishes the current Entity and adds it to the current children array. This
  // can only be used between StartChildren() and FinishChildren(). Every entity
  // after StartChildren() must use FinishChild(), while the root entity should
  // use Finish() instead. Returns true if there were no errors.
  bool FinishChild();

  // Finishes the array of children, effectively "popping" the children array
  // stack that was pushed by StartChildren(). After this you can call
  // AddComponent() for the Entity that has these children. Returns true if
  // there were no errors.
  bool FinishChildren();

 private:
  using ComponentOffset = flatbuffers::Offset<BlueprintComponentDef>;
  using EntityOffset = flatbuffers::Offset<BlueprintDef>;
  template <typename T>
  using VectorOffset = flatbuffers::Offset<flatbuffers::Vector<T>>;

  EntityOffset FinishEntity();

  flatbuffers::FlatBufferBuilder fbb_;
  std::vector<ComponentOffset> components_vector_;
  std::vector<std::vector<EntityOffset>> children_vector_stack_;
  VectorOffset<EntityOffset> children_offsets_;

  BlueprintBuilder(const BlueprintBuilder& rhs) = delete;
  BlueprintBuilder& operator=(const BlueprintBuilder& rhs) = delete;
};

}  // namespace detail
}  // namespace lull

#endif  // LULLABY_MODULES_ECS_BLUEPRINT_BUILDER_H_
