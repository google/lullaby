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
#include "redux/modules/dispatcher/message.h"

namespace redux {
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
    archive(number, kNumberHash);
    archive(word, kWordHash);
  }

  int number = 0;
  std::string word;
};
}  // namespace
}  // namespace redux

// We need to step out of the namespace to setup the TypeId before we start
// using it.
REDUX_SETUP_TYPEID(redux::Event);

namespace redux {
namespace {

TEST(Message, ConcreteToConcrete) {
  const Event event(123, "hello");
  Message msg(event);

  EXPECT_THAT(msg.GetTypeId(), Eq(GetTypeId<Event>()));
  EXPECT_THAT(msg.Get<int>(), IsNull());
  EXPECT_THAT(msg.Get<Event>(), Not(IsNull()));

  const Event* ptr = msg.Get<Event>();
  EXPECT_THAT(event.number, Eq(ptr->number));
  EXPECT_THAT(event.word, Eq(ptr->word));
}

TEST(Message, RuntimeToRuntime) {
  Message msg(GetTypeId<Event>());
  msg.SetValue(kWordHash, std::string("hello"));
  msg.SetValue(kNumberHash, 123);

  EXPECT_THAT(msg.GetTypeId(), Eq(GetTypeId<Event>()));
  EXPECT_THAT(msg.ValueOr(kNumberHash, 0), Eq(123));
  EXPECT_THAT(msg.ValueOr(kWordHash, std::string()), Eq("hello"));
  EXPECT_THAT(msg.ValueOr(kNumberBadHash, 0), Eq(0));
  EXPECT_THAT(msg.ValueOr(kWordBadHash, std::string()), Eq(""));
  EXPECT_THAT(msg.ValueOr(kNumberBadHash, 123), Eq(123));
  EXPECT_THAT(msg.ValueOr(kWordBadHash, std::string("hello")), Eq("hello"));
}

TEST(Message, ConcreteToRuntime) {
  const Event event(123, "hello");
  Message msg(event);

  EXPECT_THAT(msg.GetTypeId(), Eq(GetTypeId<Event>()));
  EXPECT_THAT(msg.Get<int>(), IsNull());
  EXPECT_THAT(msg.Get<Event>(), Not(IsNull()));

  EXPECT_THAT(msg.ValueOr(kNumberHash, 0), Eq(123));
  EXPECT_THAT(msg.ValueOr(kWordHash, std::string()), Eq("hello"));
  EXPECT_THAT(msg.ValueOr(kNumberBadHash, 0), Eq(0));
  EXPECT_THAT(msg.ValueOr(kWordBadHash, std::string()), Eq(""));
  EXPECT_THAT(msg.ValueOr(kNumberBadHash, 123), Eq(123));
  EXPECT_THAT(msg.ValueOr(kWordBadHash, std::string("hello")), Eq("hello"));
}

TEST(Message, RuntimeToConcrete) {
  Message msg(GetTypeId<Event>());
  msg.SetValue(kWordHash, std::string("hello"));
  msg.SetValue(kNumberHash, 123);

  EXPECT_THAT(msg.GetTypeId(), Eq(GetTypeId<Event>()));

  const Event* ptr = msg.Get<Event>();
  EXPECT_THAT(ptr, Not(IsNull()));
  EXPECT_THAT(ptr->number, Eq(123));
  EXPECT_THAT(ptr->word, Eq("hello"));
}

TEST(Message, ConcreteLocked) {
  const Event event(123, "hello");
  Message msg(event);
  EXPECT_DEATH(msg.SetValue(kNumberHash, 456), "");
}

TEST(Message, RuntimeToConcreteLocked) {
  Message msg(GetTypeId<Event>());
  msg.SetValue(kWordHash, std::string("hello"));
  msg.SetValue(kNumberHash, 123);

  EXPECT_THAT(msg.ValueOr(kNumberHash, 0), Eq(123));

  const Event* ptr = msg.Get<Event>();
  EXPECT_THAT(ptr, Not(IsNull()));
  EXPECT_DEATH(msg.SetValue(kNumberHash, 456), "");
}

TEST(Message, VarTable) {
  VarTable tbl;
  tbl[kWordHash] = std::string("hello");
  tbl[kNumberHash] = 123;
  Message msg(GetTypeId<Event>(), std::move(tbl));

  const Event* ptr = msg.Get<Event>();
  EXPECT_THAT(ptr, Not(IsNull()));
  EXPECT_THAT(ptr->number, Eq(123));
  EXPECT_THAT(ptr->word, Eq("hello"));

  const VarTable* other = msg.Get<VarTable>();
  EXPECT_THAT(other, Not(IsNull()));
  EXPECT_THAT(other->ValueOr(kNumberHash, 0), Eq(123));
  EXPECT_THAT(other->ValueOr(kWordHash, std::string()), Eq("hello"));
}

}  // namespace
}  // namespace redux
