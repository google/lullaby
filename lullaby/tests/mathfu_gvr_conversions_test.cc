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

#include "lullaby/modules/gvr/mathfu_gvr_conversions.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

TEST(MathfuGvrConversions, GvrMatFromMathfu) {
  // GVR data is row-major; mathfu is column major.
  mathfu::mat4 mathfuMat(0, 1, 2, 3, 10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32,
                         33);
  gvr::Mat4f gvrMat = GvrMatFromMathfu(mathfuMat);
  EXPECT_EQ(0,  gvrMat.m[0][0]);
  EXPECT_EQ(10, gvrMat.m[0][1]);
  EXPECT_EQ(20, gvrMat.m[0][2]);
  EXPECT_EQ(30, gvrMat.m[0][3]);
  EXPECT_EQ(1,  gvrMat.m[1][0]);
  EXPECT_EQ(11, gvrMat.m[1][1]);
  EXPECT_EQ(21, gvrMat.m[1][2]);
  EXPECT_EQ(31, gvrMat.m[1][3]);
  EXPECT_EQ(2,  gvrMat.m[2][0]);
  EXPECT_EQ(12, gvrMat.m[2][1]);
  EXPECT_EQ(22, gvrMat.m[2][2]);
  EXPECT_EQ(32, gvrMat.m[2][3]);
  EXPECT_EQ(3,  gvrMat.m[3][0]);
  EXPECT_EQ(13, gvrMat.m[3][1]);
  EXPECT_EQ(23, gvrMat.m[3][2]);
  EXPECT_EQ(33, gvrMat.m[3][3]);
}

TEST(MathfuGvrConversions, MathfuMatFromGvr) {
  // GVR data is row-major; mathfu is column major.
  gvr::Mat4f gvrMat;
  gvrMat.m[0][0] = 0;
  gvrMat.m[0][1] = 10;
  gvrMat.m[0][2] = 20;
  gvrMat.m[0][3] = 30;
  gvrMat.m[1][0] = 1;
  gvrMat.m[1][1] = 11;
  gvrMat.m[1][2] = 21;
  gvrMat.m[1][3] = 31;
  gvrMat.m[2][0] = 2;
  gvrMat.m[2][1] = 12;
  gvrMat.m[2][2] = 22;
  gvrMat.m[2][3] = 32;
  gvrMat.m[3][0] = 3;
  gvrMat.m[3][1] = 13;
  gvrMat.m[3][2] = 23;
  gvrMat.m[3][3] = 33;

  mathfu::mat4 mathfuMat = MathfuMatFromGvr(gvrMat);
  EXPECT_EQ(0, mathfuMat(0, 0));
  EXPECT_EQ(10, mathfuMat(0, 1));
  EXPECT_EQ(20, mathfuMat(0, 2));
  EXPECT_EQ(30, mathfuMat(0, 3));
  EXPECT_EQ(1, mathfuMat(1, 0));
  EXPECT_EQ(11, mathfuMat(1, 1));
  EXPECT_EQ(21, mathfuMat(1, 2));
  EXPECT_EQ(31, mathfuMat(1, 3));
  EXPECT_EQ(2, mathfuMat(2, 0));
  EXPECT_EQ(12, mathfuMat(2, 1));
  EXPECT_EQ(22, mathfuMat(2, 2));
  EXPECT_EQ(32, mathfuMat(2, 3));
  EXPECT_EQ(3, mathfuMat(3, 0));
  EXPECT_EQ(13, mathfuMat(3, 1));
  EXPECT_EQ(23, mathfuMat(3, 2));
  EXPECT_EQ(33, mathfuMat(3, 3));
}

TEST(MathfuGvrConversions, GvrQuatFromMathfu) {
  // In mathfu, scalar is first; in GVR, it is last.
  mathfu::quat mathfuQuat(1, 2, 3, 4);
  gvr::Quatf gvrQuat = GvrQuatFromMathfu(mathfuQuat);
  EXPECT_EQ(2, gvrQuat.qx);
  EXPECT_EQ(3, gvrQuat.qy);
  EXPECT_EQ(4, gvrQuat.qz);
  EXPECT_EQ(1, gvrQuat.qw);
}

}  // namespace
}  // namespace lull
