/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#include "lullaby/tests/mathfu_matchers.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mathfu/glsl_mappings.h"

namespace lull {
namespace {

using ::testing::Not;
using testing::EqualsMathfu;
using testing::NearMathfu;

const float kEpsilon = 2e-5f;
const float kLessThanEpsilon = 1e-5f;
const float kMoreThanEpsilon = 3e-5f;

TEST(MathfuMatchersTest, Vector2) {
  const mathfu::vec2 v1(1.2f, 3.4f);

  EXPECT_THAT(v1, EqualsMathfu(v1));

  for (int i = 0; i < 2; ++i) {
    mathfu::vec2 v2(v1);
    v2[i] += kLessThanEpsilon;
    EXPECT_THAT(v1, Not(EqualsMathfu(v2)));
    EXPECT_THAT(v1, NearMathfu(v2, kEpsilon));
  }

  for (int i = 0; i < 2; ++i) {
    mathfu::vec2 v2(v1);
    v2[i] += kMoreThanEpsilon;
    EXPECT_THAT(v1, Not(NearMathfu(v2, kEpsilon)));
  }
}

TEST(MathfuMatchersTest, Vector3) {
  const mathfu::vec3 v1(1.2f, 3.4f, 5.6f);

  EXPECT_THAT(v1, EqualsMathfu(v1));

  for (int i = 0; i < 3; ++i) {
    mathfu::vec3 v2(v1);
    v2[i] += kLessThanEpsilon;
    EXPECT_THAT(v1, Not(EqualsMathfu(v2)));
    EXPECT_THAT(v1, NearMathfu(v2, kEpsilon));
  }

  for (int i = 0; i < 3; ++i) {
    mathfu::vec3 v2(v1);
    v2[i] += kMoreThanEpsilon;
    EXPECT_THAT(v1, Not(NearMathfu(v2, kEpsilon)));
  }
}


TEST(MathfuMatchersTest, Vector4) {
  const mathfu::vec4 v1(1.2f, 3.4f, 5.6f, 7.8f);

  EXPECT_THAT(v1, EqualsMathfu(v1));

  for (int i = 0; i < 4; ++i) {
    mathfu::vec4 v2(v1);
    v2[i] += kLessThanEpsilon;
    EXPECT_THAT(v1, Not(EqualsMathfu(v2)));
    EXPECT_THAT(v1, NearMathfu(v2, kEpsilon));
  }

  for (int i = 0; i < 4; ++i) {
    mathfu::vec4 v2(v1);
    v2[i] += kMoreThanEpsilon;
    EXPECT_THAT(v1, Not(NearMathfu(v2, kEpsilon)));
  }
}

TEST(MathfuMatchersTest, Quaternion) {
  const mathfu::quat q1(1.2f, 3.4f, 5.6f, 7.8f);

  EXPECT_THAT(q1, EqualsMathfu(q1));

  for (int i = 0; i < 4; ++i) {
    mathfu::quat q2(q1);
    q2[i] += kLessThanEpsilon;
    EXPECT_THAT(q1, Not(EqualsMathfu(q2)));
    EXPECT_THAT(q1, NearMathfu(q2, kEpsilon));
  }

  for (int i = 0; i < 4; ++i) {
    mathfu::quat q2(q1);
    q2[i] += kMoreThanEpsilon;
    EXPECT_THAT(q1, Not(NearMathfu(q2, kEpsilon)));
  }
}

TEST(MathfuMatchersTest, Matrix) {
  const mathfu::mat4 m1(0.0f, 0.1f, 0.2f, 0.3f,
                        1.0f, 1.1f, 1.2f, 1.3f,
                        2.0f, 2.1f, 2.2f, 2.3f,
                        3.0f, 3.1f, 3.2f, 3.3f);

  EXPECT_THAT(m1, EqualsMathfu(m1));

  for (int i = 0; i < 16; ++i) {
    mathfu::mat4 m2(m1);
    m2[i] += kLessThanEpsilon;
    EXPECT_THAT(m1, Not(EqualsMathfu(m2)));
    EXPECT_THAT(m1, NearMathfu(m2, kEpsilon));
  }

  for (int i = 0; i < 16; ++i) {
    mathfu::mat4 m2(m1);
    m2[i] += kMoreThanEpsilon;
    EXPECT_THAT(m1, Not(NearMathfu(m2, kEpsilon)));
  }
}

}  // namespace
}  // namespace lull
