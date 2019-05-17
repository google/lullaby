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

#include "lullaby/util/type_name_generator.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

struct SomeClass {};

template <typename T>
void DoTest(const char* name) {
  EXPECT_EQ(TypeNameGenerator::Generate<T>(), std::string(name));
}

TEST(TypeNameGenerator, Names) {
  DoTest<bool>("bool");
  DoTest<int8_t>("int8_t");
  DoTest<uint8_t>("uint8_t");
  DoTest<int16_t>("int16_t");
  DoTest<uint16_t>("uint16_t");
  DoTest<int32_t>("int32_t");
  DoTest<uint32_t>("uint32_t");
  DoTest<int64_t>("int64_t");
  DoTest<uint64_t>("uint64_t");
  DoTest<float>("float");
  DoTest<double>("double");
  DoTest<mathfu::vec2>("mathfu::vec2");
  DoTest<mathfu::vec2i>("mathfu::vec2i");
  DoTest<mathfu::vec3>("mathfu::vec3");
  DoTest<mathfu::vec3i>("mathfu::vec3i");
  DoTest<mathfu::vec4>("mathfu::vec4");
  DoTest<mathfu::vec4i>("mathfu::vec4i");
  DoTest<mathfu::mat4>("mathfu::mat4");
  DoTest<std::string>("std::string");
  DoTest<lull::SomeClass>("lull::SomeClass");
  DoTest<lull::Optional<float>>("lull::Optional<float>");
  DoTest<std::vector<std::string>>("std::vector<std::string>");
  DoTest<std::unordered_map<int32_t, lull::SomeClass>>(
      "std::unordered_map<int32_t, lull::SomeClass>");
  DoTest<std::map<int32_t, std::vector<std::map<int32_t, std::string>>>>(
      "std::map<int32_t, std::vector<std::map<int32_t, std::string>>>");
}

}  // namespace
}  // namespace lull

LULLABY_SETUP_TYPEID(lull::SomeClass);
