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

#ifndef LULLABY_TESTS_UTIL_FAKE_ENTITY_DEF_H_
#define LULLABY_TESTS_UTIL_FAKE_ENTITY_DEF_H_

#include "flatbuffers/flatbuffers.h"
#include "lullaby/tests/util/fake_flatbuffer_union.h"

namespace lull {
namespace testing {

// Entity, Component, and Builder definitions which conform to the Flatbuffers
// API and use the FakeFlatbufferUnion to construct their list of components at
// runtime. Useful for testing Blueprints without using any .fbs file.
using FakeComponentDefType = FakeFlatbufferUnion::DefId;

struct FakeComponentDef FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  static FLATBUFFERS_CONSTEXPR const char* GetFullyQualifiedName() {
    return "FakeComponentDef";
  }
  enum { VT_DEF_TYPE = 4, VT_DEF = 6 };
  FakeComponentDefType def_type() const {
    return GetField<FakeComponentDefType>(VT_DEF_TYPE, 0);
  }
  const void* def() const { return GetPointer<const void*>(VT_DEF); }
};

struct FakeComponentDefBuilder {
  flatbuffers::FlatBufferBuilder& fbb;
  flatbuffers::uoffset_t start;
  void add_def_type(FakeComponentDefType def_type) {
    fbb.AddElement<FakeComponentDefType>(FakeComponentDef::VT_DEF_TYPE,
                                         def_type, 0);
  }
  void add_def(flatbuffers::Offset<void> def) {
    fbb.AddOffset(FakeComponentDef::VT_DEF, def);
  }
  explicit FakeComponentDefBuilder(
      flatbuffers::FlatBufferBuilder& _fbb)
      : fbb(_fbb) {
    start = fbb.StartTable();
  }
  flatbuffers::Offset<FakeComponentDef> Finish() {
    const auto end = fbb.EndTable(start, 2);
    auto o = flatbuffers::Offset<FakeComponentDef>(end);
    return o;
  }
};

struct FakeEntityDef FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  static FLATBUFFERS_CONSTEXPR const char* GetFullyQualifiedName() {
    return "FakeEntityDef";
  }
  enum { VT_COMPONENTS = 4, VT_CHILDREN = 6 };
  const flatbuffers::Vector<flatbuffers::Offset<FakeComponentDef>>* components()
      const {
    return GetPointer<
        const flatbuffers::Vector<flatbuffers::Offset<FakeComponentDef>>*>(
        VT_COMPONENTS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<FakeEntityDef>>* children()
      const {
    return GetPointer<
        const flatbuffers::Vector<flatbuffers::Offset<FakeEntityDef>>*>(
        VT_CHILDREN);
  }
  bool Verify(flatbuffers::Verifier&) const { return true; }
};

struct FakeEntityDefBuilder {
  flatbuffers::FlatBufferBuilder& fbb;
  flatbuffers::uoffset_t start;
  void add_components(
      flatbuffers::Offset<
          flatbuffers::Vector<flatbuffers::Offset<FakeComponentDef>>>
          components) {
    fbb.AddOffset(FakeEntityDef::VT_COMPONENTS, components);
  }
  void add_children(
      flatbuffers::Offset<
          flatbuffers::Vector<flatbuffers::Offset<FakeEntityDef>>> children) {
    fbb.AddOffset(FakeEntityDef::VT_CHILDREN, children);
  }
  explicit FakeEntityDefBuilder(
      flatbuffers::FlatBufferBuilder& _fbb)
      : fbb(_fbb) {
    start = fbb.StartTable();
  }
  flatbuffers::Offset<FakeEntityDef> Finish() {
    const auto end = fbb.EndTable(start, 1);
    auto o = flatbuffers::Offset<FakeEntityDef>(end);
    return o;
  }
};

// This helper template allows mapping directly from a type to its integral
// FakeComponentDefType.  It is automatically populated by the active
// FakeFlatbufferUnion.
template <typename T>
struct FakeComponentDefTypeTraits {
  static const FakeComponentDefType& enum_value;
};

template <typename T>
const FakeComponentDefType& FakeComponentDefTypeTraits<T>::enum_value =
    FakeFlatbufferUnion::TypeToDefId<T>::value;

inline const char* const * EnumNamesFakeComponentDefType() {
  return FakeFlatbufferUnion::GetActiveTypeNames();
}

inline const FakeEntityDef* GetFakeEntityDef(const void* buf) {
  return flatbuffers::GetRoot<FakeEntityDef>(buf);
}

}  // namespace testing
}  // namespace lull

#endif  // LULLABY_TESTS_UTIL_FAKE_ENTITY_DEF_H_
