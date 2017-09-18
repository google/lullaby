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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/dispatcher/event_wrapper.h"
#include "lullaby/modules/script/lull/script_env.h"

namespace lull {
namespace {

using ::testing::Eq;
using ::testing::Ne;

TEST(ScriptFunctionsEventTest, Event) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(make-event :foo {(1u 'a') (2u 123) (4u 'd')})");
  EXPECT_THAT(res.Is<EventWrapper>(), Eq(true));
  EXPECT_THAT(res.Get<EventWrapper>()->GetTypeId(), Eq(ConstHash("foo")));
  const auto* values = res.Get<EventWrapper>()->GetValues();
  EXPECT_THAT(values, Ne(nullptr));
  EXPECT_THAT(values->size(), Eq(static_cast<size_t>(3)));
  EXPECT_THAT(values->count(1), Eq(static_cast<size_t>(1)));
  EXPECT_THAT(values->count(2), Eq(static_cast<size_t>(1)));
  EXPECT_THAT(values->count(3), Eq(static_cast<size_t>(0)));
  EXPECT_THAT(values->count(4), Eq(static_cast<size_t>(1)));

  auto iter = values->find(1);
  EXPECT_THAT(iter->second.GetTypeId(), Eq(GetTypeId<std::string>()));
  EXPECT_THAT(*iter->second.Get<std::string>(), Eq("a"));

  iter = values->find(2);
  EXPECT_THAT(iter->second.GetTypeId(), Eq(GetTypeId<int>()));
  EXPECT_THAT(*iter->second.Get<int>(), Eq(123));

  iter = values->find(4);
  EXPECT_THAT(iter->second.GetTypeId(), Eq(GetTypeId<std::string>()));
  EXPECT_THAT(*iter->second.Get<std::string>(), Eq("d"));
}

TEST(ScriptFunctionsEventTest, EventType) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(event-type (make-event :foo))");
  EXPECT_THAT(res.Is<TypeId>(), Eq(true));
  EXPECT_THAT(*res.Get<TypeId>(), Eq(ConstHash("foo")));

  res = env.Exec("(event-type (make-event :bar {(1u 'a')}))");
  EXPECT_THAT(res.Is<TypeId>(), Eq(true));
  EXPECT_THAT(*res.Get<TypeId>(), Eq(ConstHash("bar")));

  res = env.Exec("(event-type (make-event :baz {(1u 'a') (2u 123) (4u 'd')}))");
  EXPECT_THAT(res.Is<TypeId>(), Eq(true));
  EXPECT_THAT(*res.Get<TypeId>(), Eq(ConstHash("baz")));
}

TEST(ScriptFunctionsEventTest, EventSize) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(event-size (make-event :foo))");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(0));

  res = env.Exec("(event-size (make-event :bar {(1u 'a')}))");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(1));

  res = env.Exec("(event-size (make-event :baz {(1u 'a') (2u 123) (4u 'd')}))");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(3));
}

TEST(ScriptFunctionsEventTest, EventEmpty) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(event-empty (make-event :foo))");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res =
      env.Exec("(event-empty (make-event :bar {(1u 'a') (2u 123) (4 u'd')}))");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));
}

TEST(ScriptFunctionsEventTest, EventGet) {
  ScriptEnv env;
  ScriptValue res;

  res =
      env.Exec("(event-get (make-event :foo {(1u 'a') (2u 123) (4u 'd')}) 2u)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(123));
}

TEST(ScriptFunctionsEventTest, EventWrapper) {
  ScriptEnv env;
  ScriptValue res;

  EventWrapper event(ConstHash("dummy-event"));
  event.SetValue(ConstHash("field1"), true);
  event.SetValue(ConstHash("field2"), 123);

  env.SetValue(ConstHash("event"), ScriptValue::Create(event));

  res = env.Exec("(event-type event)");
  EXPECT_THAT(res.Is<TypeId>(), Eq(true));
  EXPECT_THAT(*res.Get<TypeId>(), Eq(ConstHash("dummy-event")));

  res = env.Exec("(event-size event)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(2));

  res = env.Exec("(event-empty event)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(event-get event :field1)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(event-get event :field2)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(123));
}

}  // namespace
}  // namespace lull
