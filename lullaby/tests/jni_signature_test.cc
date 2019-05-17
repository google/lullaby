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

#include "lullaby/modules/jni/jni_signature.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

using ::testing::Eq;
using ::testing::StaticAssertTypeEq;

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

  const auto& s6 = GetJniMethodSignature<JavaLangInteger, JavaxVecmathVector4f,
                                         ComGoogleLullabyEvent>();
  EXPECT_THAT(s6.name, Eq("(Ljavax/vecmath/Vector4f;Lcom/google/lullaby/Event;)"
                          "Ljava/lang/Integer;"));
}

TEST(JniSignature, JniForward) {
  // Derivation used for function arguments.
  jint i1 = 123;
  const jint i2 = 456;
  auto fi1 = JniForward(i1);
  auto fi2 = JniForward(i2);
  auto fi3 = JniForward((jint) 789);
  StaticAssertTypeEq<jint, decltype(fi1)>();
  StaticAssertTypeEq<jint, decltype(fi2)>();
  StaticAssertTypeEq<jint, decltype(fi3)>();
  EXPECT_EQ(static_cast<jint>(123), fi1);
  EXPECT_EQ(static_cast<jint>(456), fi2);
  EXPECT_EQ(static_cast<jint>(789), fi3);

  jobject o1 = reinterpret_cast<jobject>(147);
  jobject o2 = reinterpret_cast<jobject>(258);
  jobject o3 = reinterpret_cast<jobject>(369);
  JavaLangFloat f1(o1);
  const JavaLangFloat f2(o2);
  auto fo1 = JniForward(f1);
  auto fo2 = JniForward(f2);
  auto fo3 = JniForward((JavaLangFloat) o3);
  StaticAssertTypeEq<jobject, decltype(fo1)>();
  StaticAssertTypeEq<jobject, decltype(fo2)>();
  StaticAssertTypeEq<jobject, decltype(fo3)>();
  EXPECT_EQ(reinterpret_cast<jobject>(147), fo1);
  EXPECT_EQ(reinterpret_cast<jobject>(258), fo2);
  EXPECT_EQ(reinterpret_cast<jobject>(369), fo3);

  // Derivation used for return types. decltype does not return the simple type,
  // but a rvalue (&&), so it needs to be decayed.
  using FEvent = decltype(JniForward(std::declval<ComGoogleLullabyEvent>()));
  using DEvent = typename std::decay<FEvent>::type;
  StaticAssertTypeEq<jobject, DEvent>();
}

}  // namespace
}  // namespace lull
