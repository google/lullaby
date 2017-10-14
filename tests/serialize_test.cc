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

#include <string>
#include <unordered_map>
#include <vector>

#include "gtest/gtest.h"
#include "lullaby/modules/serialize/buffer_serializer.h"
#include "lullaby/modules/serialize/serialize.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/optional.h"
#include "tests/portable_test_macros.h"

namespace lull {
namespace {

enum SerializeEnum {
  kEnumFoo,
  kEnumBar,
  kEnumBaz,
};

struct SerializeBase {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&base_value, Hash("base_value"));
  }

  int base_value = 0;
};

struct SerializeDerived : public SerializeBase {
  template <typename Archive>
  void Serialize(Archive archive) {
    SerializeBase::Serialize(archive);
    archive(&derived_value, Hash("derived_value"));
  }

  int derived_value = 0;
};

struct SerializeCompound {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&int_value, Hash("int_value"));
    archive(&float_value, Hash("float_value"));
    archive(&enum_value, Hash("enum_value"));
    archive(&string_value, Hash("string_value"));
    archive(&string_vec, Hash("string_vec"));
    archive(&dictionary, Hash("dictionary"));
    archive(&optional, Hash("optional"));
    archive(&optional, Hash("optional_unset"));
    archive(&base, Hash("base"));
    archive(&derived, Hash("derived"));
  }

  int int_value = 0;
  float float_value = 0.f;
  SerializeEnum enum_value = kEnumFoo;
  std::string string_value;
  std::vector<std::string> string_vec;
  std::unordered_map<int, std::string> dictionary;
  Optional<float> optional;
  Optional<float> optional_unset;
  SerializeBase base;
  SerializeDerived derived;
};

TEST(Serialize, SaveLoad) {
  SerializeCompound obj1;
  obj1.int_value = 1;
  obj1.float_value = 2.f;
  obj1.enum_value = kEnumBaz;
  obj1.string_value = "hello";
  obj1.string_vec.emplace_back("how");
  obj1.string_vec.emplace_back("are");
  obj1.string_vec.emplace_back("you");
  obj1.dictionary[123] = "123";
  obj1.dictionary[456] = "456";
  obj1.dictionary[789] = "789";
  obj1.optional = 3.f;
  obj1.base.base_value = 4;
  obj1.derived.base_value = 5;
  obj1.derived.derived_value = 6;

  std::vector<uint8_t> buffer;

  SaveToBuffer saver(&buffer);
  Serialize(&saver, &obj1, 0);

  SerializeCompound obj2;
  LoadFromBuffer loader(&buffer);
  Serialize(&loader, &obj2, 0);

  EXPECT_EQ(obj1.int_value, obj2.int_value);
  EXPECT_EQ(obj1.float_value, obj2.float_value);
  EXPECT_EQ(obj1.enum_value, obj2.enum_value);
  EXPECT_EQ(obj1.string_value, obj2.string_value);
  EXPECT_EQ(obj1.string_vec, obj2.string_vec);
  EXPECT_EQ(obj1.dictionary, obj2.dictionary);
  EXPECT_EQ(obj1.optional, obj2.optional);
  EXPECT_EQ(obj1.optional_unset, obj2.optional_unset);
  EXPECT_EQ(obj1.base.base_value, obj2.base.base_value);
  EXPECT_EQ(obj1.derived.base_value, obj2.derived.base_value);
  EXPECT_EQ(obj1.derived.derived_value, obj2.derived.derived_value);
}

TEST(SerializeDeathTest, LoadOutOfBounds) {
  std::vector<uint8_t> buffer;

  uint8_t small = 0;
  SaveToBuffer saver(&buffer);
  saver(&small, 0);

  uint32_t big = 0;
  LoadFromBuffer loader(&buffer);
  PORT_EXPECT_DEBUG_DEATH(loader(&big, 0), "");
}

}  // namespace
}  // namespace lull
