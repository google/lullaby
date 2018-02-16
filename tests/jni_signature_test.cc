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

#include "lullaby/util/jni_signature.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

using ::testing::Eq;

TEST(JniSignature, Fields) {
  EXPECT_THAT(GetJniFieldSignature<jboolean>().name, Eq("Z"));
  EXPECT_THAT(GetJniFieldSignature<jint>().name, Eq("I"));
  EXPECT_THAT(GetJniFieldSignature<jlong>().name, Eq("J"));
  EXPECT_THAT(GetJniFieldSignature<jfloat>().name, Eq("F"));
  EXPECT_THAT(GetJniFieldSignature<jdouble>().name, Eq("D"));
  EXPECT_THAT(GetJniFieldSignature<jobject>().name, Eq("Ljava/lang/Object;"));
  EXPECT_THAT(GetJniFieldSignature<jstring>().name, Eq("Ljava/lang/String;"));
}

TEST(JniSignature, Methods) {
  const auto& s1 = GetJniMethodSignature<void>();
  EXPECT_THAT(s1.name, Eq("()V"));

  const auto& s2 = GetJniMethodSignature<void, jint>();
  EXPECT_THAT(s2.name, Eq("(I)V"));

  const auto& s3 = GetJniMethodSignature<jint, jint>();
  EXPECT_THAT(s3.name, Eq("(I)I"));

  const auto& s4 = GetJniMethodSignature<jint, jfloat, jfloat>();
  EXPECT_THAT(s4.name, Eq("(FF)I"));

  const auto& s5 = GetJniMethodSignature<jstring, jobject>();
  EXPECT_THAT(s5.name, Eq("(Ljava/lang/Object;)Ljava/lang/String;"));
}

}  // namespace
}  // namespace lull
