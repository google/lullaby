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

#include "lullaby/tests/util/fake_entity_def.h"

#include "gtest/gtest.h"
#include "lullaby/tests/util/entity_factory_test.h"
#include "lullaby/tests/util/fake_entity_test.h"

namespace lull {
namespace testing {

INSTANTIATE_TYPED_TEST_SUITE_P(Fake, EntityFactoryTest,
                               LULL_ENTITY_TEST_TYPE(FakeEntityDef,
                                                     FakeComponentDef));

INSTANTIATE_TYPED_TEST_SUITE_P(Fake, EntityFactoryDeathTest,
                               LULL_ENTITY_TEST_TYPE(FakeEntityDef,
                                                     FakeComponentDef));

}  // namespace testing
}  // namespace lull
