/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#include <functional>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/modules/base/function_traits.h"

namespace redux {
namespace {

int FunctionTraitsTestFunction(float) { return 0; }

struct FunctionTraitsTestClass {
  int Member(float) { return 0; }
  int ConstMember(float) const { return 0; }
  static int StaticFunction(float) { return 0; }
};

TEST(FunctionTraits, Function) {
  using Traits = FunctionTraits<decltype(FunctionTraitsTestFunction)>;
  static_assert(Traits::kNumArgs == 1);
  static_assert(std::is_same_v<typename Traits::ReturnType, int>);
  static_assert(std::is_same_v<typename Traits::Arg<0>::Type, float>);
}

TEST(FunctionTraits, FunctionPointer) {
  using Traits = FunctionTraits<decltype(&FunctionTraitsTestFunction)>;
  static_assert(Traits::kNumArgs == 1);
  static_assert(std::is_same_v<typename Traits::ReturnType, int>);
  static_assert(std::is_same_v<typename Traits::Arg<0>::Type, float>);
}

TEST(FunctionTraits, Member) {
  using Traits = FunctionTraits<decltype(&FunctionTraitsTestClass::Member)>;
  static_assert(Traits::kNumArgs == 1);
  static_assert(std::is_same_v<typename Traits::ReturnType, int>);
  static_assert(std::is_same_v<typename Traits::Arg<0>::Type, float>);
}

TEST(FunctionTraits, ConstMember) {
  using Traits =
      FunctionTraits<decltype(&FunctionTraitsTestClass::ConstMember)>;
  static_assert(Traits::kNumArgs == 1);
  static_assert(std::is_same_v<typename Traits::ReturnType, int>);
  static_assert(std::is_same_v<typename Traits::Arg<0>::Type, float>);
}

TEST(FunctionTraits, ClassStaticFunction) {
  using Traits =
      FunctionTraits<decltype(FunctionTraitsTestClass::StaticFunction)>;
  static_assert(Traits::kNumArgs == 1);
  static_assert(std::is_same_v<typename Traits::ReturnType, int>);
  static_assert(std::is_same_v<typename Traits::Arg<0>::Type, float>);
}

TEST(FunctionTraits, ClassStaticFunctionPointer) {
  using Traits =
      FunctionTraits<decltype(&FunctionTraitsTestClass::StaticFunction)>;
  static_assert(Traits::kNumArgs == 1);
  static_assert(std::is_same_v<typename Traits::ReturnType, int>);
  static_assert(std::is_same_v<typename Traits::Arg<0>::Type, float>);
}

TEST(FunctionTraits, Lambda) {
  auto fn = [](float) -> int { return 0; };

  using Traits = FunctionTraits<decltype(fn)>;
  static_assert(Traits::kNumArgs == 1);
  static_assert(std::is_same_v<typename Traits::ReturnType, int>);
  static_assert(std::is_same_v<typename Traits::Arg<0>::Type, float>);
}

TEST(FunctionTraits, StdFunction) {
  std::function<int(float)> fn;
  using Traits = FunctionTraits<decltype(fn)>;

  static_assert(Traits::kNumArgs == 1);
  static_assert(std::is_same_v<typename Traits::ReturnType, int>);
  static_assert(std::is_same_v<typename Traits::Arg<0>::Type, float>);
}

}  // namespace
}  // namespace redux
