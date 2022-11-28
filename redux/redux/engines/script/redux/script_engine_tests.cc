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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/engines/script/script_engine.h"

namespace redux {
enum class TestOp {
  Add,
  Mul,
};
}  // namespace redux

REDUX_SETUP_TYPEID(redux::TestOp);

namespace redux {
namespace {

using ::testing::Eq;

static int Add(int x, int y) { return x + y; }

static int Mul(int x, int y) { return x * y; }

static int Apply(TestOp op, int x, int y) {
  switch (op) {
    case TestOp::Add:
      return Add(x, y);
    case TestOp::Mul:
      return Mul(x, y);
  }
}

class ScriptEngineTests : public ::testing::Test {
 protected:
  void SetUp() override {
    // We assume the redux script engine for our tests.
    ScriptEngine::Create(&registry_);
    engine_ = registry_.Get<ScriptEngine>();
  }

  Registry registry_;
  ScriptEngine* engine_ = nullptr;
};

TEST_F(ScriptEngineTests, RunNow) {
  Var res = engine_->RunNow("(do 123)");
  EXPECT_TRUE(res.Is<int>());
  EXPECT_THAT(res.ValueOr(0), Eq(123));
}

TEST_F(ScriptEngineTests, RegisterFunction) {
  engine_->RegisterFunction("add", Add);

  Var res = engine_->RunNow("(add 12 34)");
  EXPECT_TRUE(res.Is<int>());
  EXPECT_THAT(res.ValueOr(0), Eq(46));
}

TEST_F(ScriptEngineTests, RegisterEnum) {
  engine_->RegisterEnum<TestOp>("TestOp");
  engine_->RegisterFunction("apply", Apply);

  Var res = engine_->RunNow("(apply TestOp.Mul 12 34)");
  EXPECT_TRUE(res.Is<int>());
  EXPECT_THAT(res.ValueOr(0), Eq(408));
}

TEST_F(ScriptEngineTests, RegisterEnumReturn) {
  engine_->RegisterFunction("return_enum", []() { return TestOp::Mul; });

  Var res = engine_->RunNow("(return_enum)");
  EXPECT_TRUE(res.Is<TestOp>());
  EXPECT_THAT(res.ValueOr(TestOp::Add), Eq(TestOp::Mul));
}

}  // namespace
}  // namespace redux
