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
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/engines/script/call_native_function.h"

namespace redux {
namespace {

using ::testing::Eq;

struct TestContext {
  absl::StatusCode ArgFromCpp(size_t index, int* out) {
    if (index < args.size()) {
      *out = args[index];
      return absl::StatusCode::kOk;
    }
    return absl::StatusCode::kOutOfRange;
  }

  absl::StatusCode ReturnFromCpp(int value) {
    return_value = value;
    return absl::StatusCode::kOk;
  }

  std::vector<int> args;
  int return_value = 0;
};

static int Add(int x, int y) { return x + y; }

TEST(CallNativeFunction, Function) {
  TestContext context;
  context.args.push_back(1);
  context.args.push_back(2);
  CallNativeFunction(&context, Add);
  EXPECT_THAT(context.return_value, Eq(3));
}

TEST(CallNativeFunction, Lambda) {
  auto lambda = [](int x, int y) { return x + y; };

  TestContext context;
  context.args.push_back(1);
  context.args.push_back(2);
  CallNativeFunction(&context, lambda);
  EXPECT_THAT(context.return_value, Eq(3));
}

TEST(CallNativeFunction, StdFunction) {
  std::function<int(int, int)> add = [](int x, int y) { return x + y; };

  TestContext context;
  context.args.push_back(1);
  context.args.push_back(2);
  CallNativeFunction(&context, add);
  EXPECT_THAT(context.return_value, Eq(3));
}

}  // namespace
}  // namespace redux
