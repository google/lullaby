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

#include "gtest/gtest.h"

#include "lullaby/util/random_number_generator.h"

namespace lull {
namespace {

TEST(RandomNumberGenerator, Int) {
  RandomNumberGenerator rng;
  int value = rng.GenerateUniform(0, 100);
  EXPECT_LE(0, value);
  EXPECT_GE(100, value);
}

TEST(RandomNumberGenerator, Float) {
  RandomNumberGenerator rng;
  float value = rng.GenerateUniform(0.f, 100.f);
  EXPECT_LE(0.f, value);
  EXPECT_GE(100.f, value);
}

}  // namespace
}  // namespace lull
