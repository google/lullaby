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

#include "lullaby/modules/dispatcher/event_wrapper.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/util/common_types.h"

namespace lull {
namespace {

using ::testing::Eq;
using ::testing::IsNull;
using ::testing::Not;

const HashValue kNumberHash = Hash("number");
const HashValue kWordHash = Hash("word");
const HashValue kNumberBadHash = Hash("number_bad");
const HashValue kWordBadHash = Hash("word_bad");

struct Event {
  Event() {}
  Event(int number, std::string word) : number(number), word(std::move(word)) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&number, kNumberHash);
    archive(&word, kWordHash);
  }

  int number = 0;
  std::string word;
};

struct UnserializableEvent {
  UnserializableEvent() {}
  UnserializableEvent(int number, std::string word)
      : number(number), word(std::move(word)) {}

  int number = 0;
  std::string word;
};

}  // namespace
}  // namespace lull

// We need to step out of the namespace to setup the TypeId before we start
// using it.
LULLABY_SETUP_TYPEID(lull::Event);
LULLABY_SETUP_TYPEID(lull::UnserializableEvent);

namespace lull {
namespace {

TEST(EventWrapper, ConcreteToConcrete) {
  const Event event(123, "hello");
  EventWrapper wrapper(event);

  EXPECT_THAT(wrapper.GetTypeId(), Eq(GetTypeId<Event>()));
  EXPECT_THAT(wrapper.IsRuntimeEvent(), Eq(false));
  EXPECT_THAT(wrapper.Get<int>(), IsNull());
  EXPECT_THAT(wrapper.Get<Event>(), Not(IsNull()));

  const Event* ptr = wrapper.Get<Event>();
  EXPECT_THAT(event.number, Eq(ptr->number));
  EXPECT_THAT(event.word, Eq(ptr->word));
}

TEST(EventWrapper, RuntimeToRuntime) {
  EventWrapper wrapper(GetTypeId<Event>());
  wrapper.SetValue(kWordHash, std::string("hello"));
  wrapper.SetValue(kNumberHash, 123);

  EXPECT_THAT(wrapper.GetTypeId(), Eq(GetTypeId<Event>()));
  EXPECT_THAT(wrapper.IsRuntimeEvent(), Eq(true));
  EXPECT_THAT(*wrapper.GetValue<int>(kNumberHash), Eq(123));
  EXPECT_THAT(*wrapper.GetValue<std::string>(kWordHash), Eq("hello"));
  EXPECT_THAT(wrapper.GetValueWithDefault(kNumberHash, 0), Eq(123));
  EXPECT_THAT(wrapper.GetValueWithDefault(kWordHash, std::string()),
              Eq("hello"));
  EXPECT_THAT(wrapper.GetValueWithDefault(kNumberBadHash, 0), Eq(0));
  EXPECT_THAT(wrapper.GetValueWithDefault(kWordBadHash, std::string()),
              Eq(""));
  EXPECT_THAT(wrapper.GetValueWithDefault(kNumberBadHash, 123), Eq(123));
  EXPECT_THAT(wrapper.GetValueWithDefault(kWordBadHash, std::string("hello")),
              Eq("hello"));
}

TEST(EventWrapper, ConcreteToRuntime) {
  const Event event(123, "hello");
  EventWrapper wrapper(event);

  EXPECT_THAT(wrapper.GetTypeId(), Eq(GetTypeId<Event>()));
  EXPECT_THAT(wrapper.IsRuntimeEvent(), Eq(false));
  EXPECT_THAT(wrapper.Get<int>(), IsNull());
  EXPECT_THAT(wrapper.Get<Event>(), Not(IsNull()));

  EXPECT_THAT(*wrapper.GetValue<int>(kNumberHash), Eq(123));
  EXPECT_THAT(*wrapper.GetValue<std::string>(kWordHash), Eq("hello"));
  EXPECT_THAT(wrapper.GetValueWithDefault(kNumberHash, 0), Eq(123));
  EXPECT_THAT(wrapper.GetValueWithDefault(kWordHash, std::string()),
              Eq("hello"));
  EXPECT_THAT(wrapper.GetValueWithDefault(kNumberBadHash, 0), Eq(0));
  EXPECT_THAT(wrapper.GetValueWithDefault(kWordBadHash, std::string()),
              Eq(""));
  EXPECT_THAT(wrapper.GetValueWithDefault(kNumberBadHash, 123), Eq(123));
  EXPECT_THAT(wrapper.GetValueWithDefault(kWordBadHash, std::string("hello")),
              Eq("hello"));
}

TEST(EventWrapper, RuntimeToConcrete) {
  EventWrapper wrapper(GetTypeId<Event>());
  wrapper.SetValue(kWordHash, std::string("hello"));
  wrapper.SetValue(kNumberHash, 123);

  EXPECT_THAT(wrapper.GetTypeId(), Eq(GetTypeId<Event>()));
  EXPECT_THAT(wrapper.IsRuntimeEvent(), Eq(true));

  const Event* ptr = wrapper.Get<Event>();
  EXPECT_THAT(ptr->number, Eq(123));
  EXPECT_THAT(ptr->word, Eq("hello"));
}

TEST(EventWrapper, RuntimeToConcreteLocked) {
  EventWrapper wrapper(GetTypeId<Event>());
  wrapper.SetValue(kWordHash, std::string("hello"));
  wrapper.SetValue(kNumberHash, 123);

  EXPECT_THAT(*wrapper.GetValue<int>(kNumberHash), Eq(123));

  const Event* ptr = wrapper.Get<Event>();
  EXPECT_THAT(ptr, Not(IsNull()));

  wrapper.SetValue(kNumberHash, 456);
  EXPECT_THAT(*wrapper.GetValue<int>(kNumberHash), Eq(123));
}


TEST(EventWrapper, RuntimeMap) {
  VariantMap map;
  map[kWordHash] = std::string("hello");
  map[kNumberHash] = 123;

  EventWrapper wrapper(GetTypeId<Event>());
  wrapper.SetValues(map);

  const VariantMap* other = wrapper.GetValues();

  const Variant& var1 = map.find(kWordHash)->second;
  const Variant& var2 = other->find(kWordHash)->second;
  EXPECT_THAT(*var1.Get<std::string>(), Eq(*var2.Get<std::string>()));
}

TEST(EventWrapper, IsSerializable) {
  EventWrapper wrapper(UnserializableEvent(123, "hello"));
  EXPECT_THAT(wrapper.IsSerializable(), false);

  EventWrapper wrapper2(Event(123, "hello"));
  EXPECT_THAT(wrapper2.IsSerializable(), true);
}

#if LULLABY_TRACK_EVENT_NAMES
TEST(EventWrapper, GetName) {
  EventWrapper wrapper(UnserializableEvent(123, "hello"));
  EXPECT_THAT(wrapper.GetName(), "lull::UnserializableEvent");

  EventWrapper wrapper2(Event(123, "hello"));
  EXPECT_THAT(wrapper2.GetName(), "lull::Event");

  EventWrapper wrapper3(Hash("TestEventName"), "TestEventName");
  EXPECT_THAT(wrapper3.GetName(), "TestEventName");
}
#endif

}  // namespace
}  // namespace lull
