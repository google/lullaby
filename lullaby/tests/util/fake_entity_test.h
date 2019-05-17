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

#ifndef LULLABY_TESTS_UTIL_FAKE_ENTITY_TEST_H_
#define LULLABY_TESTS_UTIL_FAKE_ENTITY_TEST_H_

#include "lullaby/tests/util/entity_test.h"

#include "lullaby/tests/test_def_generated.h"
#include "lullaby/tests/util/fake_entity_def.h"

namespace lull {
namespace testing {

// EntityTest specialization for FakeEntityDefs.
//
// This allows properly setting up and tearing down the FakeFlatbuffersUnion
// for each test.
template <>
class EntityTest<LULL_ENTITY_TEST_TYPE(FakeEntityDef, FakeComponentDef)>
    : public ::testing::Test {
 protected:
  EntityTest() {
    test_component_def_union_ =
        FakeFlatbufferUnion::Create<ValueDefT, ComplexDefT>();
    FakeFlatbufferUnion::SetActive(test_component_def_union_.get());
  }
  ~EntityTest() override { FakeFlatbufferUnion::ClearActive(); }

  std::unique_ptr<FakeFlatbufferUnion> test_component_def_union_;
};

}  // namespace testing
}  // namespace lull

#endif  // LULLABY_TESTS_UTIL_FAKE_ENTITY_TEST_H_
