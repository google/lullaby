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

#include "lullaby/modules/flatbuffers/variant_fb_conversions.h"
#include "gtest/gtest.h"
#include "lullaby/util/math.h"

namespace lull {
namespace {

TEST(VariantFbConversions, Bool) {
  flatbuffers::FlatBufferBuilder fbb;
  fbb.Finish(CreateDataBool(fbb, true));
  auto ptr = flatbuffers::GetRoot<DataBool>(fbb.GetBufferPointer());

  Variant var;
  const bool res = VariantFromFbVariant(VariantDef_DataBool, ptr, &var);

  EXPECT_TRUE(res);
  EXPECT_EQ(true, *var.Get<bool>());
}

TEST(VariantFbConversions, Int) {
  flatbuffers::FlatBufferBuilder fbb;
  fbb.Finish(CreateDataInt(fbb, 123));
  auto ptr = flatbuffers::GetRoot<DataInt>(fbb.GetBufferPointer());

  Variant var;
  const bool res = VariantFromFbVariant(VariantDef_DataInt, ptr, &var);

  EXPECT_TRUE(res);
  EXPECT_EQ(123, *var.Get<int>());
}

TEST(VariantFbConversions, Float) {
  flatbuffers::FlatBufferBuilder fbb;
  fbb.Finish(CreateDataFloat(fbb, 123.f));
  auto ptr = flatbuffers::GetRoot<DataFloat>(fbb.GetBufferPointer());

  Variant var;
  const bool res = VariantFromFbVariant(VariantDef_DataFloat, ptr, &var);

  EXPECT_TRUE(res);
  EXPECT_EQ(123.f, *var.Get<float>());
}

TEST(VariantFbConversions, Hash) {
  flatbuffers::FlatBufferBuilder fbb;
  fbb.Finish(CreateDataHashValue(fbb, Hash("hello")));
  auto ptr = flatbuffers::GetRoot<DataHashValue>(fbb.GetBufferPointer());

  Variant var;
  const bool res = VariantFromFbVariant(VariantDef_DataHashValue, ptr, &var);

  EXPECT_TRUE(res);
  EXPECT_EQ(Hash("hello"), *var.Get<HashValue>());
}

TEST(VariantFbConversions, String) {
  flatbuffers::FlatBufferBuilder fbb;
  fbb.Finish(CreateDataString(fbb, fbb.CreateString("hello")));
  auto ptr = flatbuffers::GetRoot<DataString>(fbb.GetBufferPointer());

  Variant var;
  const bool res = VariantFromFbVariant(VariantDef_DataString, ptr, &var);

  EXPECT_TRUE(res);
  EXPECT_EQ("hello", *var.Get<std::string>());
}

TEST(VariantFbConversions, Vec2) {
  flatbuffers::FlatBufferBuilder fbb;
  Vec2 value(1.f, 2.f);
  fbb.Finish(CreateDataVec2(fbb, &value));
  auto ptr = flatbuffers::GetRoot<DataVec2>(fbb.GetBufferPointer());

  Variant var;
  const bool res = VariantFromFbVariant(VariantDef_DataVec2, ptr, &var);

  EXPECT_TRUE(res);
  EXPECT_EQ(mathfu::vec2(1.f, 2.f), *var.Get<mathfu::vec2>());
}

TEST(VariantFbConversions, Vec3) {
  flatbuffers::FlatBufferBuilder fbb;
  Vec3 value(1.f, 2.f, 3.f);
  fbb.Finish(CreateDataVec3(fbb, &value));
  auto ptr = flatbuffers::GetRoot<DataVec3>(fbb.GetBufferPointer());

  Variant var;
  const bool res = VariantFromFbVariant(VariantDef_DataVec3, ptr, &var);

  EXPECT_TRUE(res);
  EXPECT_EQ(mathfu::vec3(1.f, 2.f, 3.f), *var.Get<mathfu::vec3>());
}

TEST(VariantFbConversions, Vec4) {
  flatbuffers::FlatBufferBuilder fbb;
  Vec4 value(1.f, 2.f, 3.f, 4.f);
  fbb.Finish(CreateDataVec4(fbb, &value));
  auto ptr = flatbuffers::GetRoot<DataVec4>(fbb.GetBufferPointer());

  Variant var;
  const bool res = VariantFromFbVariant(VariantDef_DataVec4, ptr, &var);

  EXPECT_TRUE(res);
  EXPECT_EQ(mathfu::vec4(1.f, 2.f, 3.f, 4.f), *var.Get<mathfu::vec4>());
}

TEST(VariantFbConversions, Quat) {
  flatbuffers::FlatBufferBuilder fbb;
  Quat value(1.f, 2.f, 3.f, 4.f);
  fbb.Finish(CreateDataQuat(fbb, &value));
  auto ptr = flatbuffers::GetRoot<DataQuat>(fbb.GetBufferPointer());

  Variant var;
  const bool res = VariantFromFbVariant(VariantDef_DataQuat, ptr, &var);
  EXPECT_TRUE(res);

  const mathfu::quat quat = mathfu::quat(4.f, 1.f, 2.f, 3.f).Normalized();
  EXPECT_EQ(quat.scalar(), var.Get<mathfu::quat>()->scalar());
  EXPECT_EQ(quat.vector(), var.Get<mathfu::quat>()->vector());
}

TEST(VariantFbConversions, Bytes) {
  flatbuffers::FlatBufferBuilder fbb;
  std::vector<uint8_t> bytes = {1, 2, 3, 4, 5, 6, 7, 8};
  fbb.Finish(CreateDataBytes(fbb, fbb.CreateVector(bytes)));
  auto ptr = flatbuffers::GetRoot<DataBytes>(fbb.GetBufferPointer());

  Variant var;
  const bool res = VariantFromFbVariant(VariantDef_DataBytes, ptr, &var);
  EXPECT_TRUE(res);

  EXPECT_EQ(var.Get<ByteArray>()->size(), size_t(8));
  EXPECT_EQ(var.Get<ByteArray>()->front(), uint8_t(1));
  EXPECT_EQ(var.Get<ByteArray>()->back(), uint8_t(8));
}

TEST(VariantFbConversions, Array) {
  flatbuffers::FlatBufferBuilder fbb;

  std::vector<flatbuffers::Offset<VariantArrayDefImpl>> values;
  values.push_back(CreateVariantArrayDefImpl(fbb, VariantDef_DataInt,
                                             CreateDataInt(fbb, 123).Union()));
  values.push_back(CreateVariantArrayDefImpl(
      fbb, VariantDef_DataFloat, CreateDataFloat(fbb, 456.f).Union()));
  values.push_back(
      CreateVariantArrayDefImpl(fbb, VariantDef_DataString,
                                CreateDataStringDirect(fbb, "hello").Union()));

  fbb.Finish(CreateVariantArrayDefDirect(fbb, &values));
  auto ptr = flatbuffers::GetRoot<VariantArrayDef>(fbb.GetBufferPointer());

  VariantArray arr;
  const bool res = VariantArrayFromFbVariantArray(ptr, &arr);
  EXPECT_TRUE(res);
  EXPECT_EQ(arr.size(), size_t(3));
  EXPECT_EQ(123, *arr[0].Get<int>());
  EXPECT_EQ(456.f, *arr[1].Get<float>());
  EXPECT_EQ("hello", *arr[2].Get<std::string>());
}

TEST(VariantFbConversions, Map) {
  flatbuffers::FlatBufferBuilder fbb;

  std::vector<flatbuffers::Offset<KeyVariantPairDef>> values;
  values.push_back(CreateKeyVariantPairDefDirect(
      fbb, nullptr, 1, VariantDef_DataInt, CreateDataInt(fbb, 123).Union()));
  values.push_back(
      CreateKeyVariantPairDefDirect(fbb, nullptr, 2, VariantDef_DataFloat,
                                    CreateDataFloat(fbb, 456.f).Union()));
  values.push_back(CreateKeyVariantPairDefDirect(
      fbb, nullptr, 3, VariantDef_DataString,
      CreateDataStringDirect(fbb, "hello").Union()));

  fbb.Finish(CreateVariantMapDefDirect(fbb, &values));
  auto ptr = flatbuffers::GetRoot<VariantMapDef>(fbb.GetBufferPointer());

  VariantMap map;
  const bool res = VariantMapFromFbVariantMap(ptr, &map);
  EXPECT_TRUE(res);
  EXPECT_EQ(map.size(), size_t(3));
  EXPECT_EQ(123, *map[1].Get<int>());
  EXPECT_EQ(456.f, *map[2].Get<float>());
  EXPECT_EQ("hello", *map[3].Get<std::string>());
}

TEST(VariantFbConversions, VariantArray) {
  flatbuffers::FlatBufferBuilder fbb;

  std::vector<flatbuffers::Offset<VariantArrayDefImpl>> values;
  values.push_back(CreateVariantArrayDefImpl(fbb, VariantDef_DataInt,
                                             CreateDataInt(fbb, 123).Union()));
  values.push_back(CreateVariantArrayDefImpl(
      fbb, VariantDef_DataFloat, CreateDataFloat(fbb, 456.f).Union()));
  values.push_back(
      CreateVariantArrayDefImpl(fbb, VariantDef_DataString,
                                CreateDataStringDirect(fbb, "hello").Union()));
  fbb.Finish(CreateVariantArrayDefDirect(fbb, &values));
  auto ptr = flatbuffers::GetRoot<VariantArrayDef>(fbb.GetBufferPointer());

  Variant var;
  const bool res = VariantFromFbVariant(VariantDef_VariantArrayDef, ptr, &var);

  EXPECT_TRUE(res);
  auto* arr = var.Get<VariantArray>();

  ASSERT_NE(arr, nullptr);
  EXPECT_EQ(arr->size(), size_t(3));
  EXPECT_EQ(123, *arr->at(0).Get<int>());
  EXPECT_EQ(456.f, *arr->at(1).Get<float>());
  EXPECT_EQ("hello", *arr->at(2).Get<std::string>());
}

TEST(VariantFbConversions, VariantMap) {
  flatbuffers::FlatBufferBuilder fbb;

  std::vector<flatbuffers::Offset<KeyVariantPairDef>> values;
  values.push_back(CreateKeyVariantPairDefDirect(
      fbb, nullptr, 1, VariantDef_DataInt, CreateDataInt(fbb, 123).Union()));
  values.push_back(
      CreateKeyVariantPairDefDirect(fbb, nullptr, 2, VariantDef_DataFloat,
                                    CreateDataFloat(fbb, 456.f).Union()));
  values.push_back(CreateKeyVariantPairDefDirect(
      fbb, nullptr, 3, VariantDef_DataString,
      CreateDataStringDirect(fbb, "hello").Union()));

  fbb.Finish(CreateVariantMapDefDirect(fbb, &values));
  auto ptr = flatbuffers::GetRoot<VariantMapDef>(fbb.GetBufferPointer());

  Variant var;
  const bool res = VariantFromFbVariant(VariantDef_VariantMapDef, ptr, &var);

  EXPECT_TRUE(res);
  auto* map = var.Get<VariantMap>();

  ASSERT_NE(map, nullptr);
  EXPECT_EQ(map->size(), size_t(3));
  EXPECT_EQ(123, *map->at(1).Get<int>());
  EXPECT_EQ(456.f, *map->at(2).Get<float>());
  EXPECT_EQ("hello", *map->at(3).Get<std::string>());
}

}  // namespace
}  // namespace lull
