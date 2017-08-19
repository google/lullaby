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

#include "lullaby/util/unordered_vector_map.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

struct TestClass {
  TestClass(int key, int value) : key(key), value(value) {}
  int key;
  int value;
};

struct TestKeyFn {
  int operator()(const TestClass& t) const {
    return t.key;
  }
};

struct TestLookupHash {
  size_t operator()(const int key) const {
    return static_cast<size_t>(key + 1);
  }
};

using TestUnorderedVectorMap =
    UnorderedVectorMap<int, TestClass, TestKeyFn, TestLookupHash>;

TEST(UnorderedVectorMap, Empty) {
  TestUnorderedVectorMap map(32);
  EXPECT_EQ(static_cast<int>(map.Size()), 0);
}

TEST(UnorderedVectorMap, Add) {
  TestUnorderedVectorMap map(32);
  auto obj = map.Emplace(1, 10);
  ASSERT_NE(obj, nullptr);
  EXPECT_EQ(obj->key, 1);
  EXPECT_EQ(obj->value, 10);
  EXPECT_EQ(static_cast<int>(map.Size()), 1);
}

TEST(UnorderedVectorMap, Get) {
  TestUnorderedVectorMap map(32);

  auto obj = map.Get(1);
  EXPECT_EQ(obj, nullptr);

  map.Emplace(1, 10);
  obj = map.Get(1);
  ASSERT_NE(obj, nullptr);
  EXPECT_EQ(obj->key, 1);
  EXPECT_EQ(obj->value, 10);
  EXPECT_EQ(static_cast<int>(map.Size()), 1);
}

TEST(UnorderedVectorMap, Duplicates) {
  TestUnorderedVectorMap map(32);

  map.Emplace(1, 10);
  map.Emplace(1, 100);
  EXPECT_EQ(static_cast<int>(map.Size()), 1);

  // Make sure it the first obj in the map, not the second.
  auto obj = map.Get(1);
  ASSERT_NE(obj, nullptr);
  EXPECT_EQ(obj->key, 1);
  EXPECT_EQ(obj->value, 10);
}

TEST(UnorderedVectorMap, Multipage) {
  TestUnorderedVectorMap map(32);

  for (int i = 0; i < 128; ++i) {
    map.Emplace(i, 10 * i);
  }
  EXPECT_EQ(static_cast<int>(map.Size()), 128);
}

TEST(UnorderedVectorMap, ForEach) {
  TestUnorderedVectorMap map(32);

  int check = 0;
  for (int i = 0; i < 128; ++i) {
    const int value = 10 * i;
    map.Emplace(i, value);
    check += value;
  }
  EXPECT_EQ(static_cast<int>(map.Size()), 128);

  int sum = 0;
  map.ForEach([&](TestClass& t) {
    sum += t.value;
  });
  EXPECT_EQ(sum, check);
}

TEST(UnorderedVectorMap, RangeBasedFor) {
  TestUnorderedVectorMap map(32);

  int check = 0;
  for (int i = 0; i < 128; ++i) {
    const int value = 10 * i;
    map.Emplace(i, value);
    check += value;
  }
  EXPECT_EQ(static_cast<int>(map.Size()), 128);

  int sum = 0;
  for (TestClass& t : map) {
    sum += t.value;
  }
  EXPECT_EQ(sum, check);

  sum = 0;
  const TestUnorderedVectorMap& const_map = map;
  for (const TestClass& t : const_map) {
    sum += t.value;
  }
  EXPECT_EQ(sum, check);
}

TEST(UnorderedVectorMap, AddRemove) {
  TestUnorderedVectorMap map(32);

  int check = 0;
  for (int i = 0; i < 128; ++i) {
    const int value = 10 * i;
    map.Emplace(i, value);
    check += value;
  }
  EXPECT_EQ(static_cast<int>(map.Size()), 128);

  for (int i = 55; i < 101; ++i) {
    const int value = 10 * i;
    map.Destroy(i);
    check -= value;
  }

  int sum1 = 0;
  map.ForEach([&](TestClass& t) { sum1 += t.value; });

  int sum2 = 0;
  for (TestClass& t : map) {
    sum2 += t.value;
  }

  EXPECT_EQ(sum1, check);
  EXPECT_EQ(sum2, check);
  EXPECT_EQ(static_cast<int>(map.Size()), 128 - 101 + 55);
}

TEST(UnorderedVectorMap, IteratorTraits) {
  using IteratorTraits = std::iterator_traits<TestUnorderedVectorMap::iterator>;
  static_assert(std::is_same<IteratorTraits::iterator_category,
                             std::forward_iterator_tag>(),
                "Non-const iterator must be a forward iterator.");
  static_assert(std::is_same<IteratorTraits::difference_type, std::ptrdiff_t>(),
                "Non-const iterator difference_type should be ptrdiff_t");
  static_assert(std::is_same<IteratorTraits::value_type, TestClass>(),
                "Non-const iterator value_type should be TestClass");
  static_assert(std::is_same<IteratorTraits::reference, TestClass&>(),
                "Non-const iterator reference should be TestClass&");
  static_assert(std::is_same<IteratorTraits::pointer, TestClass*>(),
                "Non-const iterator pointer should be TestClass*");

  using ConstIteratorTraits =
      std::iterator_traits<TestUnorderedVectorMap::const_iterator>;
  static_assert(std::is_same<ConstIteratorTraits::iterator_category,
                             std::forward_iterator_tag>(),
                "Const iterator must be a forward iterator.");
  static_assert(
      std::is_same<ConstIteratorTraits::difference_type, std::ptrdiff_t>(),
      "Const iterator difference_type should be ptrdiff_t");
  static_assert(std::is_same<ConstIteratorTraits::value_type, TestClass>(),
                "Const iterator value_type should be TestClass");
  static_assert(
      std::is_same<ConstIteratorTraits::reference, const TestClass&>(),
      "Const iterator reference should be const TestClass&");
  static_assert(std::is_same<ConstIteratorTraits::pointer, const TestClass*>(),
                "Const iterator pointer should be const TestClass*");
}

TEST(UnorderedVectorMap, NonConstIteration) {
  TestUnorderedVectorMap map(32);

  int check = 0;
  for (int i = 0; i < 128; ++i) {
    const int value = 10 * i;
    map.Emplace(i, value);
    check += value;
  }
  EXPECT_EQ(static_cast<int>(map.Size()), 128);

  auto it = map.begin();
  static_assert(std::is_same<decltype(it), TestUnorderedVectorMap::iterator>(),
                "Non-const map should return a non-const iterator.");
  static_assert(std::is_same<decltype(*it), TestClass&>(),
                "Non-const iterator should return to a non-const reference.");

  int sum1 = 0;
  int sum2 = 0;
  while (it != map.end()) {
    sum1 += it->value;
    sum2 += (*it).value;
    ++it;
  }
  EXPECT_EQ(sum1, check);
  EXPECT_EQ(sum2, check);

  int sum3 = 0;
  it = map.begin();
  while (it != map.end()) {
    sum3 += (it++)->value;
  }
  EXPECT_EQ(sum3, check);
}

TEST(UnorderedVectorMap, ConstIteration) {
  TestUnorderedVectorMap map(32);
  const TestUnorderedVectorMap& const_map = map;

  int check = 0;
  for (int i = 0; i < 128; ++i) {
    const int value = 10 * i;
    map.Emplace(i, value);
    check += value;
  }
  EXPECT_EQ(static_cast<int>(map.Size()), 128);

  auto it = const_map.begin();
  static_assert(
      std::is_same<decltype(it), TestUnorderedVectorMap::const_iterator>(),
      "Const map should return a const iterator.");
  static_assert(std::is_same<decltype(*it), const TestClass&>(),
                "Const iterator should return to a const reference.");

  int sum1 = 0;
  int sum2 = 0;
  while (it != const_map.end()) {
    sum1 += it->value;
    sum2 += (*it).value;
    ++it;
  }
  EXPECT_EQ(sum1, check);
  EXPECT_EQ(sum2, check);

  int sum3 = 0;
  it = const_map.begin();
  while (it != const_map.end()) {
    sum3 += (it++)->value;
  }
  EXPECT_EQ(sum3, check);
}

TEST(UnorderedVectorMap, MixAndMatchConstIterators) {
  TestUnorderedVectorMap map(32);
  const TestUnorderedVectorMap& const_map = map;

  int check = 0;
  for (int i = 0; i < 128; ++i) {
    const int value = 10 * i;
    map.Emplace(i, value);
    check += value;
  }
  EXPECT_EQ(static_cast<int>(map.Size()), 128);

  // Equality and inequality between non-const and const iterators.
  EXPECT_EQ(map.begin(), const_map.begin());
  EXPECT_EQ(const_map.end(), map.end());
  EXPECT_NE(map.begin(), const_map.end());
  EXPECT_NE(const_map.end(), map.begin());

  // Allows implicit conversion to const from non-const.
  static_assert(
      std::is_convertible<TestUnorderedVectorMap::iterator,
                          TestUnorderedVectorMap::const_iterator>::value,
      "Non-const iterator should convert to a const iterator.");
  static_assert(!std::is_convertible<TestUnorderedVectorMap::const_iterator,
                                     TestUnorderedVectorMap::iterator>::value,
                "Const iterator should not convert to a non-const iterator.");

  int sum = 0;
  TestUnorderedVectorMap::const_iterator it = map.begin();
  while (it != map.end()) {
    sum += it->value;
    ++it;
  }
  EXPECT_EQ(sum, check);
}

}  // namespace
}  // namespace lull
