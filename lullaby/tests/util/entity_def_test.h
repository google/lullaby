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

#ifndef LULLABY_TESTS_UTIL_ENTITY_DEF_TEST_H_
#define LULLABY_TESTS_UTIL_ENTITY_DEF_TEST_H_

#include <type_traits>
#include <vector>

#include "flatbuffers/flatbuffers.h"
#include "gtest/gtest.h"
#include "lullaby/tests/test_def_generated.h"
#include "lullaby/tests/util/entity_test.h"

namespace lull {
namespace testing {

// Type-parameterized test case for testing whether a given EntityDef and
// ComponentDef implementation is compatible with the Flatbuffers API,
// particularly as used by Lullaby.
template <typename TypeParam>
class EntityDefTest : public EntityTest<TypeParam> {};
TYPED_TEST_SUITE_P(EntityDefTest);

TYPED_TEST_P(EntityDefTest, EntitySchemaIsValid) {
  using EntityDef = typename TypeParam::entity_type;
  using ComponentDef = typename TypeParam::component_type;
  using ComponentDefType = typename TypeParam::component_union_type;

  // Verify EntityDef / ComponentDef methods and return types.
  //
  // EntityDef should have:
  //    const Vector<ComponentDef>* components() const
  // Vector should have:
  //    const ComponentDef* Get() const
  // ComponentDef should have:
  //    ComponentDefType def_type() const
  //    const void* def() const
  using EntityComponentsReturnType = typename std::result_of<decltype (
      &EntityDef::components)(EntityDef)>::type;
  using ComponentsVector = typename std::remove_const<
      typename std::remove_pointer<EntityComponentsReturnType>::type>::type;
  using VectorGetReturnType = typename std::result_of<decltype (
      &ComponentsVector::Get)(ComponentsVector, flatbuffers::uoffset_t)>::type;
  using ComponentDefTypeReturnType = typename std::result_of<decltype (
      &ComponentDef::def_type)(ComponentDef)>::type;
  using ComponentDefReturnType = typename std::result_of<decltype (
      &ComponentDef::def)(ComponentDef)>::type;
  EXPECT_FALSE(std::is_const<EntityComponentsReturnType>::value);
  EXPECT_FALSE(std::is_volatile<EntityComponentsReturnType>::value);
  EXPECT_TRUE(std::is_pointer<EntityComponentsReturnType>::value);
  EXPECT_FALSE(std::is_const<ComponentsVector>::value);
  EXPECT_FALSE(std::is_volatile<ComponentsVector>::value);
  EXPECT_FALSE(std::is_pointer<ComponentsVector>::value);
  EXPECT_TRUE((std::is_same<VectorGetReturnType, const ComponentDef*>::value));
  EXPECT_TRUE(
      (std::is_same<ComponentDefTypeReturnType, ComponentDefType>::value));
  EXPECT_TRUE((std::is_same<ComponentDefReturnType, const void*>::value));

  // Verify ComponentDefType is integral / enum.
  EXPECT_TRUE(((std::is_integral<ComponentDefType>::value &&
                !std::is_same<ComponentDefType, bool>::value) ||
               std::is_enum<ComponentDefType>::value));
}

TYPED_TEST_P(EntityDefTest, ComponentDefEnumsAreValid) {
  using EntityDef = typename TypeParam::entity_type;
  using ComponentDef = typename TypeParam::component_type;
  using ComponentDefType = typename TypeParam::component_union_type;

  const char* const* component_names = TypeParam::component_def_type_names();

  EXPECT_STREQ(component_names[0], "NONE");
  EXPECT_STREQ(component_names[1], "ValueDef");
  EXPECT_STREQ(component_names[2], "ComplexDef");
  EXPECT_EQ(component_names[3], nullptr);
  EXPECT_EQ(TypeParam::template component_def_type_value<void>(), 0);
  EXPECT_EQ(TypeParam::template component_def_type_value<int>(), 0);
  EXPECT_EQ(TypeParam::template component_def_type_value<EntityDef>(), 0);
  EXPECT_EQ(TypeParam::template component_def_type_value<ComponentDef>(), 0);
  EXPECT_EQ(TypeParam::template component_def_type_value<ComponentDefType>(),
            0);
  EXPECT_EQ(TypeParam::template component_def_type_value<ValueDef>(), 1);
  EXPECT_EQ(TypeParam::template component_def_type_value<ComplexDef>(), 2);
}

TYPED_TEST_P(EntityDefTest, CanReadAndWriteBuffer) {
  using EntityDef = typename TypeParam::entity_type;
  using ComponentDef = typename TypeParam::component_type;
  using EntityDefBuilder = typename TypeParam::entity_builder_type;
  using ComponentDefBuilder = typename TypeParam::component_builder_type;

  // Build a sample EntityDef with 2 components: ValueDef and ComplexDef.
  flatbuffers::FlatBufferBuilder fbb;
  {
    std::vector<flatbuffers::Offset<ComponentDef>> components;
    {
      auto value_def_offset = CreateValueDefDirect(fbb, "hello world", 42);
      ComponentDefBuilder value_component_builder(fbb);
      value_component_builder.add_def_type(
          TypeParam::template component_def_type_value<ValueDef>());
      value_component_builder.add_def(value_def_offset.Union());
      components.push_back(value_component_builder.Finish());
    }
    {
      auto complex_def_offset =
          CreateComplexDefDirect(fbb, "foo bar baz", CreateIntData(fbb, 256));
      ComponentDefBuilder complex_component_builder(fbb);
      complex_component_builder.add_def_type(
          TypeParam::template component_def_type_value<ComplexDef>());
      complex_component_builder.add_def(complex_def_offset.Union());
      components.push_back(complex_component_builder.Finish());
    }
    auto components_offset = fbb.CreateVector(components);
    EntityDefBuilder entity_builder(fbb);
    entity_builder.add_components(components_offset);
    fbb.Finish(entity_builder.Finish());
  }

  // Read and verify the buffer.
  const EntityDef* buffer_entity =
      TypeParam::get_entity(fbb.GetBufferPointer());
  const auto* buffer_components = buffer_entity->components();
  EXPECT_EQ(buffer_components->size(), static_cast<size_t>(2));
  const ComponentDef* buffer_component0 = buffer_components->Get(0);
  EXPECT_EQ(buffer_component0->def_type(),
            TypeParam::template component_def_type_value<ValueDef>());
  const ValueDef* buffer_value_def =
      reinterpret_cast<const ValueDef*>(buffer_component0->def());
  EXPECT_STREQ(buffer_value_def->name()->c_str(), "hello world");
  EXPECT_EQ(buffer_value_def->value(), 42);
  const ComponentDef* buffer_component1 = buffer_components->Get(1);
  EXPECT_EQ(buffer_component1->def_type(),
            TypeParam::template component_def_type_value<ComplexDef>());
  const ComplexDef* buffer_complex_def =
      reinterpret_cast<const ComplexDef*>(buffer_component1->def());
  EXPECT_STREQ(buffer_complex_def->name()->c_str(), "foo bar baz");
  EXPECT_NE(buffer_complex_def->data(), nullptr);
  EXPECT_EQ(buffer_complex_def->data()->value(), 256);
}

REGISTER_TYPED_TEST_SUITE_P(EntityDefTest, EntitySchemaIsValid,
                            ComponentDefEnumsAreValid, CanReadAndWriteBuffer);

}  // namespace testing
}  // namespace lull

#endif  // LULLABY_TESTS_UTIL_ENTITY_DEF_TEST_H_
