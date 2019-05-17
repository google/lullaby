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

#ifndef LULLABY_TESTS_UTIL_ENTITY_TEST_H_
#define LULLABY_TESTS_UTIL_ENTITY_TEST_H_

#include <vector>

#include "flatbuffers/flatbuffers.h"
#include "gtest/gtest.h"

// Helper macro to specify an EntityTestTypeTraits using EntityDef and
// ComponentDef.
#define LULL_ENTITY_TEST_TYPE(EntityDef, ComponentDef)              \
  EntityTestTypeTraits<EntityDef, ComponentDef, EntityDef##Builder, \
                       ComponentDef##Builder, ComponentDef##Type,   \
                       ComponentDef##TypeTraits,                    \
                       EnumNames##ComponentDef##Type>

namespace lull {
namespace testing {

// Container class for holding all types and functions related to an EntityDef.
// This allows the EntityDefTest to be parameterized using only 1 template
// parameter.
template <typename EntityDef, typename ComponentDef, typename EntityDefBuilder,
          typename ComponentDefBuilder, typename ComponentDefType,
          template <typename T> class ComponentDefTypeTraits,
          const char* const * (*const enum_component_names_fn)()>
class EntityTestTypeTraits {
 public:
  using entity_type = EntityDef;
  using component_type = ComponentDef;
  using entity_builder_type = EntityDefBuilder;
  using component_builder_type = ComponentDefBuilder;
  using component_union_type = ComponentDefType;

  static const char* const * component_def_type_names() {
    return enum_component_names_fn();
  }
  static const char* component_def_type_name(ComponentDefType type) {
    return enum_component_names_fn()[type];
  }

  template <typename T>
  static const ComponentDefType component_def_type_value() {
    return ComponentDefTypeTraits<T>::enum_value;
  }

  static const EntityDef* get_entity(const void* buf) {
    return flatbuffers::GetRoot<EntityDef>(buf);
  }
};

// Undefined class template that is used to force a pretty compile-time error.
template <typename TypeParam>
class ErrorMustSpecializeWithEntityTestTypeTraits;

// Base class for tests that wish to take advantage of EntityTestTypeTraits
// functionality and operate on EntityDef/ComponentDef types.
//
// Users should derive their typed Entity tests from EntityTest<TypeParam>,
// which should use the specialization defined below.
//
// Non-EntityTestTypeTraits specializations are errors, caused here by
// inheriting from an undefined class template.
template <typename TypeParam>
class EntityTest
    : public ErrorMustSpecializeWithEntityTestTypeTraits<TypeParam> {
};

// EntityTest specialization for EntityTestTypeTraits.  User tests will
// ultimately be subclasses of this specialization.
//
// Users can customize their tests by further specializing on their specific
// types using the LULL_ENTITY_TEST_TYPE macro.  Ex:
//   template <>
//   class EntityTest<LULL_ENTITY_TEST_TYPE(MyEntityDef, MyComponentDef)>
//         : public ::testing::Test {
//      protected:
//       EntityTest() { LOG(INFO) << "Testing MyEntityDef!"; }
//   };
template <typename EntityDef, typename ComponentDef, typename EntityDefBuilder,
          typename ComponentDefBuilder, typename ComponentDefType,
          template <typename T> class ComponentDefTypeTraits,
          const char* const * (*const enum_component_names_fn)()>
class EntityTest<EntityTestTypeTraits<
    EntityDef, ComponentDef, EntityDefBuilder, ComponentDefBuilder,
    ComponentDefType, ComponentDefTypeTraits, enum_component_names_fn>>
    : public ::testing::Test {
};

}  // namespace testing
}  // namespace lull

#endif  // LULLABY_TESTS_UTIL_ENTITY_TEST_H_
