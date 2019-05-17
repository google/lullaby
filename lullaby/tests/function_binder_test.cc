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

#include "lullaby/modules/script/function_binder.h"

#include <string>
#include "gtest/gtest.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/dispatcher/event_wrapper.h"
#include "lullaby/util/variant.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

TEST(FunctionBinderTest, BasicUsage) {
  Registry registry;
  FunctionBinder binder(&registry);

  binder.RegisterFunction("Concat",
                            [](std::string a, std::string b) { return a + b; });
  std::string a("abc");
  std::string b("def");
  Variant result = binder.Call("Concat", a, b);
  EXPECT_EQ("abcdef", *result.Get<std::string>());
}

TEST(FunctionBinderTest, Vectors) {
  Registry registry;
  FunctionBinder binder(&registry);

  binder.RegisterFunction("IntsToStrings", [](const std::vector<int>& v) {
    std::vector<std::string> out;
    for (int i : v) {
      out.emplace_back(std::to_string(i));
    }
    return out;
  });
  std::vector<int> v = {1, 2, 3};
  Variant result = binder.Call("IntsToStrings", v);
  const VariantArray* rv = result.Get<VariantArray>();
  EXPECT_EQ(3u, rv->size());
  EXPECT_EQ("1", *(*rv)[0].Get<std::string>());
  EXPECT_EQ("2", *(*rv)[1].Get<std::string>());
  EXPECT_EQ("3", *(*rv)[2].Get<std::string>());
}

TEST(FunctionBinderTest, Maps) {
  Registry registry;
  FunctionBinder binder(&registry);

  binder.RegisterFunction("RepeatStrings",
                          [](const std::map<HashValue, std::string>& m) {
                            std::map<HashValue, std::string> out;
                            for (const auto& kv : m) {
                              out[kv.first] = kv.second + kv.second;
                            }
                            return out;
                          });
  std::map<HashValue, std::string> m = {{0, "abc"}, {1, "def"}, {2, "ghi"}};
  Variant result = binder.Call("RepeatStrings", m);
  const VariantMap* rm = result.Get<VariantMap>();
  EXPECT_EQ(3u, rm->size());
  EXPECT_EQ("abcabc", *rm->find(0)->second.Get<std::string>());
  EXPECT_EQ("defdef", *rm->find(1)->second.Get<std::string>());
  EXPECT_EQ("ghighi", *rm->find(2)->second.Get<std::string>());
}

TEST(FunctionBinderTest, UnorderedMaps) {
  Registry registry;
  FunctionBinder binder(&registry);

  binder.RegisterFunction(
      "RepeatStrings", [](const std::unordered_map<HashValue, std::string>& m) {
        std::unordered_map<HashValue, std::string> out;
        for (const auto& kv : m) {
          out[kv.first] = kv.second + kv.second;
        }
        return out;
      });
  std::unordered_map<HashValue, std::string> m = {
      {0, "abc"}, {1, "def"}, {2, "ghi"}};
  Variant result = binder.Call("RepeatStrings", m);
  const VariantMap* rm = result.Get<VariantMap>();
  EXPECT_EQ(3u, rm->size());
  EXPECT_EQ("abcabc", *rm->find(0)->second.Get<std::string>());
  EXPECT_EQ("defdef", *rm->find(1)->second.Get<std::string>());
  EXPECT_EQ("ghighi", *rm->find(2)->second.Get<std::string>());
}

TEST(FunctionBinderTest, Optionals) {
  Registry registry;
  FunctionBinder binder(&registry);

  binder.RegisterFunction("DoubleOptionals", [](const Optional<float>& o) {
    Optional<float> out;
    if (o) {
      out = *o * 2.f;
    }
    return out;
  });
  Optional<float> o1(4.f);
  Optional<float> o2;
  Variant r1 = binder.Call("DoubleOptionals", o1);
  Variant r2 = binder.Call("DoubleOptionals", o2);
  EXPECT_EQ(8.f, *r1.Get<float>());
  EXPECT_EQ(nullptr, r2.Get<float>());
  EXPECT_EQ(true, r2.Empty());
}

TEST(FunctionBinderTest, EventHandlerArgument) {
  Registry registry;
  FunctionBinder binder(&registry);
  int count = 0;
  int handled_count = 0;

  binder.RegisterFunction(
      "EventHandlerArgument",
      [&count](const Dispatcher::EventHandler& handler) {
        ++count;
        ASSERT_TRUE(handler);
        EventWrapper event(Hash("myEvent"));
        event.SetValue(Hash("myInt"), 999);
        handler(event);
      });
  Dispatcher::EventHandler handler =
      [&handled_count](const EventWrapper& event) {
        ++handled_count;
        EXPECT_EQ(event.GetTypeId(), Hash("myEvent"));
        auto* ptr = event.GetValue<int>(Hash("myInt"));
        EXPECT_NE(ptr, nullptr);
        EXPECT_EQ(*ptr, 999);
      };

  EXPECT_EQ(count, 0);
  EXPECT_EQ(handled_count, 0);
  binder.Call("EventHandlerArgument", handler);
  EXPECT_EQ(count, 1);
  EXPECT_EQ(handled_count, 1);
}

TEST(FunctionBinderTest, EventHandlerReturn) {
  Registry registry;
  FunctionBinder binder(&registry);
  int count = 0;
  int handled_count = 0;

  binder.RegisterFunction(
      "EventHandlerReturn",
      [&count, &handled_count]() -> Dispatcher::EventHandler {
        ++count;
        return [&handled_count](const EventWrapper& event) {
          ++handled_count;
          EXPECT_EQ(event.GetTypeId(), Hash("myEvent"));
          auto* ptr = event.GetValue<int>(Hash("myInt"));
          EXPECT_NE(ptr, nullptr);
          EXPECT_EQ(*ptr, 999);
        };
      });

  EXPECT_EQ(count, 0);
  EXPECT_EQ(handled_count, 0);
  Variant result = binder.Call("EventHandlerReturn");
  EXPECT_EQ(count, 1);
  EXPECT_EQ(handled_count, 0);
  auto* handler = result.Get<Dispatcher::EventHandler>();
  EXPECT_NE(handler, nullptr);

  EventWrapper event(Hash("myEvent"));
  event.SetValue(Hash("myInt"), 999);
  (*handler)(event);
  EXPECT_EQ(count, 1);
  EXPECT_EQ(handled_count, 1);
}

TEST(FunctionBinderDeathTest, WrongNumberOfArgsError) {
  Registry registry;
  FunctionBinder binder(&registry);

  binder.RegisterFunction("Concat",
                            [](std::string a, std::string b) { return a + b; });
  std::string a("abc");
  Variant result;
  PORT_EXPECT_DEBUG_DEATH(result = binder.Call("Concat", a), "");
  EXPECT_TRUE(result.Empty());
}

TEST(FunctionBinderDeathTest, WrongArgTypeError) {
  Registry registry;
  FunctionBinder binder(&registry);

  binder.RegisterFunction("ExpectStrings", [](std::string, std::string) {});
  binder.RegisterFunction("ExpectVector", [](std::vector<std::string>) {});
  binder.RegisterFunction("ExpectMap", [](std::map<HashValue, double>) {});
  binder.RegisterFunction("ExpectUnorderedMap",
                            [](std::unordered_map<HashValue, mathfu::vec3>) {});
  binder.RegisterFunction("ExpectOptional", [](Optional<float>) {});

  Variant result;
  PORT_EXPECT_DEBUG_DEATH(
      result = binder.Call("ExpectStrings", std::string("abc"), 123), "");
  EXPECT_TRUE(result.Empty());

  PORT_EXPECT_DEBUG_DEATH(result = binder.Call("ExpectVector", 123), "");
  EXPECT_TRUE(result.Empty());

  PORT_EXPECT_DEBUG_DEATH(result = binder.Call("ExpectMap", 123), "");
  EXPECT_TRUE(result.Empty());

  PORT_EXPECT_DEBUG_DEATH(result = binder.Call("ExpectUnorderedMap", 123), "");
  EXPECT_TRUE(result.Empty());

  PORT_EXPECT_DEBUG_DEATH(result = binder.Call("ExpectOptional", 123), "");
  EXPECT_TRUE(result.Empty());
}

TEST(FunctionBinderDeathTest, UnregisteredFunctionError) {
  Registry registry;
  FunctionBinder binder(&registry);

  binder.RegisterFunction("Concat",
                            [](std::string a, std::string b) { return a + b; });
  binder.UnregisterFunction("Concat");

  std::string a("abc");
  std::string b("def");
  Variant result;
  PORT_EXPECT_DEBUG_DEATH(result = binder.Call("Concat", a, b), "");
  EXPECT_TRUE(result.Empty());
}

}  // namespace
}  // namespace lull
