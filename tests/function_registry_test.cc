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

#include "lullaby/util/function_registry.h"

#include <string>

#include "ion/base/logchecker.h"
#include "gtest/gtest.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/util/variant.h"

namespace lull {

class FunctionRegistryTest : public testing::Test {
 public:
  std::unique_ptr<Registry> registry_;
  FunctionRegistry* fn_registry_;
  FunctionBinder* fn_binder_;
  std::unique_ptr<ion::base::LogChecker> log_checker_;

  void SetUp() override {
    registry_.reset(new Registry());
    log_checker_.reset(new ion::base::LogChecker());
    fn_registry_ = registry_->Create<FunctionRegistry>();
    fn_binder_ = registry_->Create<FunctionBinder>(registry_.get());
  }
};

TEST_F(FunctionRegistryTest, BasicUsage) {
  fn_binder_->RegisterFunction(
      "Concat", [](std::string a, std::string b) { return a + b; });
  std::string a("abc");
  std::string b("def");
  Variant result = fn_registry_->Call("Concat", a, b);
  EXPECT_EQ("abcdef", *result.Get<std::string>());
  EXPECT_FALSE(log_checker_->HasAnyMessages());
}

TEST_F(FunctionRegistryTest, Vectors) {
  fn_binder_->RegisterFunction("IntsToStrings", [](const std::vector<int>& v) {
    std::vector<std::string> out;
    for (int i : v) {
      out.emplace_back(std::to_string(i));
    }
    return out;
  });
  std::vector<int> v = {1, 2, 3};
  Variant result = fn_registry_->Call("IntsToStrings", v);
  const VariantArray* rv = result.Get<VariantArray>();
  EXPECT_EQ(3u, rv->size());
  EXPECT_EQ("1", *(*rv)[0].Get<std::string>());
  EXPECT_EQ("2", *(*rv)[1].Get<std::string>());
  EXPECT_EQ("3", *(*rv)[2].Get<std::string>());
  EXPECT_FALSE(log_checker_->HasAnyMessages());
}

TEST_F(FunctionRegistryTest, Maps) {
  fn_binder_->RegisterFunction("RepeatStrings",
                               [](const std::map<HashValue, std::string>& m) {
                                 std::map<HashValue, std::string> out;
                                 for (const auto& kv : m) {
                                   out[kv.first] = kv.second + kv.second;
                                 }
                                 return out;
                               });
  std::map<HashValue, std::string> m = {{0, "abc"}, {1, "def"}, {2, "ghi"}};
  Variant result = fn_registry_->Call("RepeatStrings", m);
  const VariantMap* rm = result.Get<VariantMap>();
  EXPECT_EQ(3u, rm->size());
  EXPECT_EQ("abcabc", *rm->find(0)->second.Get<std::string>());
  EXPECT_EQ("defdef", *rm->find(1)->second.Get<std::string>());
  EXPECT_EQ("ghighi", *rm->find(2)->second.Get<std::string>());
  EXPECT_FALSE(log_checker_->HasAnyMessages());
}

TEST_F(FunctionRegistryTest, UnorderedMaps) {
  fn_binder_->RegisterFunction(
      "RepeatStrings", [](const std::unordered_map<HashValue, std::string>& m) {
        std::unordered_map<HashValue, std::string> out;
        for (const auto& kv : m) {
          out[kv.first] = kv.second + kv.second;
        }
        return out;
      });
  std::unordered_map<HashValue, std::string> m = {
      {0, "abc"}, {1, "def"}, {2, "ghi"}};
  Variant result = fn_registry_->Call("RepeatStrings", m);
  const VariantMap* rm = result.Get<VariantMap>();
  EXPECT_EQ(3u, rm->size());
  EXPECT_EQ("abcabc", *rm->find(0)->second.Get<std::string>());
  EXPECT_EQ("defdef", *rm->find(1)->second.Get<std::string>());
  EXPECT_EQ("ghighi", *rm->find(2)->second.Get<std::string>());
  EXPECT_FALSE(log_checker_->HasAnyMessages());
}

TEST_F(FunctionRegistryTest, Optionals) {
  fn_binder_->RegisterFunction(
      "DoubleOptionals", [](const Optional<float>& o) {
        Optional<float> out;
        if (o) {
          out = *o * 2.f;
        }
        return out;
      });
  Optional<float> o1(4.f);
  Optional<float> o2;
  Variant r1 = fn_registry_->Call("DoubleOptionals", o1);
  Variant r2 = fn_registry_->Call("DoubleOptionals", o2);
  EXPECT_EQ(8.f, *r1.Get<float>());
  EXPECT_EQ(nullptr, r2.Get<float>());
  EXPECT_EQ(true, r2.Empty());
  EXPECT_FALSE(log_checker_->HasAnyMessages());
}

TEST_F(FunctionRegistryTest, WrongNumberOfArgsError) {
  fn_binder_->RegisterFunction(
      "Concat", [](std::string a, std::string b) { return a + b; });
  std::string a("abc");
  Variant result = fn_registry_->Call("Concat", a);
  EXPECT_TRUE(result.Empty());
  EXPECT_TRUE(
      log_checker_->HasMessage("ERROR", "Concat expects 2 args, but got 1"));
}

TEST_F(FunctionRegistryTest, WrongArgTypeError) {
  fn_binder_->RegisterFunction("ExpectStrings",
                               [](std::string, std::string) {});
  fn_binder_->RegisterFunction("ExpectVector", [](std::vector<std::string>) {});
  fn_binder_->RegisterFunction("ExpectMap", [](std::map<HashValue, double>) {});
  fn_binder_->RegisterFunction(
      "ExpectUnorderedMap", [](std::unordered_map<HashValue, mathfu::vec3>) {});
  fn_binder_->RegisterFunction("ExpectOptional", [](Optional<float>) {});

  Variant result = fn_registry_->Call("ExpectStrings", std::string("abc"), 123);
  EXPECT_TRUE(result.Empty());
  EXPECT_TRUE(log_checker_->HasMessage(
      "ERROR", "ExpectStrings expects the type of arg 2 to be std::string"));

  result = fn_registry_->Call("ExpectVector", 123);
  EXPECT_TRUE(result.Empty());
  EXPECT_TRUE(log_checker_->HasMessage(
      "ERROR",
      "ExpectVector expects the type of arg 1 to be std::vector<std::string>"));

  result = fn_registry_->Call("ExpectMap", 123);
  EXPECT_TRUE(result.Empty());
  EXPECT_TRUE(
      log_checker_->HasMessage("ERROR",
                               "ExpectMap expects the type of arg 1 to be "
                               "std::map<lull::HashValue, double>"));

  result = fn_registry_->Call("ExpectUnorderedMap", 123);
  EXPECT_TRUE(result.Empty());
  EXPECT_TRUE(log_checker_->HasMessage(
      "ERROR",
      "ExpectUnorderedMap expects the type of arg 1 to be "
      "std::unordered_map<lull::HashValue, "
      "mathfu::vec3>"));

  result = fn_registry_->Call("ExpectOptional", 123);
  EXPECT_TRUE(result.Empty());
  EXPECT_TRUE(log_checker_->HasMessage(
      "ERROR",
      "ExpectOptional expects the type of arg 1 to be "
      "lull::Optional<float>"));
}

TEST_F(FunctionRegistryTest, UnregisteredFunctionError) {
  fn_binder_->RegisterFunction(
      "Concat", [](std::string a, std::string b) { return a + b; });
  fn_binder_->UnregisterFunction("Concat");
  std::string a("abc");
  std::string b("def");
  Variant result = fn_registry_->Call("Concat", a, b);
  EXPECT_TRUE(result.Empty());
  EXPECT_TRUE(log_checker_->HasMessage(
      "ERROR", "Unknown function: Concat"));
}

}  // namespace lull
