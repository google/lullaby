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
#include "redux/modules/flatbuffers/var.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/vector.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::FloatNear;
static constexpr float kEps = 1.0e-7f;

template <typename Fn>
flatbuffers::FlatBufferBuilder Build(Fn fn) {
  flatbuffers::FlatBufferBuilder fbb;
  fbb.Finish(fn(fbb));
  return fbb;
}

const void* GetRoot(const flatbuffers::FlatBufferBuilder& fbb) {
  return flatbuffers::GetRoot<flatbuffers::Table>(fbb.GetBufferPointer());
}

TEST(FbsVar, DataBool) {
  auto fbb = Build([](flatbuffers::FlatBufferBuilder& fbb) {
    return fbs::CreateDataBool(fbb, true);
  });

  Var var;

  ASSERT_TRUE(TryReadFbs(fbs::VarDef::DataBool, GetRoot(fbb), &var));
  EXPECT_TRUE(var.Is<bool>());
  EXPECT_THAT(*var.Get<bool>(), Eq(true));
}

TEST(FbsVar, DataInt) {
  auto fbb = Build([](flatbuffers::FlatBufferBuilder& fbb) {
    return fbs::CreateDataInt(fbb, 123);
  });

  Var var;
  auto res = TryReadFbs(fbs::VarDef::DataInt, GetRoot(fbb), &var);

  EXPECT_TRUE(res);
  EXPECT_TRUE(var.Is<int>());
  EXPECT_THAT(*var.Get<int>(), Eq(123));
}

TEST(FbsVar, DataFloat) {
  auto fbb = Build([](flatbuffers::FlatBufferBuilder& fbb) {
    return fbs::CreateDataFloat(fbb, 123.45f);
  });

  Var var;
  auto res = TryReadFbs(fbs::VarDef::DataFloat, GetRoot(fbb), &var);

  EXPECT_TRUE(res);
  EXPECT_TRUE(var.Is<float>());
  EXPECT_THAT(*var.Get<float>(), Eq(123.45f));
}

TEST(FbsVar, DataVec2i) {
  auto fbb = Build([](flatbuffers::FlatBufferBuilder& fbb) {
    fbs::Vec2i v(1, 2);
    return fbs::CreateDataVec2i(fbb, &v);
  });

  Var var;
  auto res = TryReadFbs(fbs::VarDef::DataVec2i, GetRoot(fbb), &var);

  EXPECT_TRUE(res);
  EXPECT_TRUE(var.Is<vec2i>());
  EXPECT_THAT(var.Get<vec2i>()->x, Eq(1));
  EXPECT_THAT(var.Get<vec2i>()->y, Eq(2));
}

TEST(FbsVar, DataVec2f) {
  auto fbb = Build([](flatbuffers::FlatBufferBuilder& fbb) {
    fbs::Vec2f v(1.1f, 2.2f);
    return fbs::CreateDataVec2f(fbb, &v);
  });

  Var var;
  auto res = TryReadFbs(fbs::VarDef::DataVec2f, GetRoot(fbb), &var);

  EXPECT_TRUE(res);
  EXPECT_TRUE(var.Is<vec2>());
  EXPECT_THAT(var.Get<vec2>()->x, Eq(1.1f));
  EXPECT_THAT(var.Get<vec2>()->y, Eq(2.2f));
}

TEST(FbsVar, DataVec3i) {
  auto fbb = Build([](flatbuffers::FlatBufferBuilder& fbb) {
    fbs::Vec3i v(1, 2, 3);
    return fbs::CreateDataVec3i(fbb, &v);
  });

  Var var;
  auto res = TryReadFbs(fbs::VarDef::DataVec3i, GetRoot(fbb), &var);

  EXPECT_TRUE(res);
  EXPECT_TRUE(var.Is<vec3i>());
  EXPECT_THAT(var.Get<vec3i>()->x, Eq(1));
  EXPECT_THAT(var.Get<vec3i>()->y, Eq(2));
  EXPECT_THAT(var.Get<vec3i>()->z, Eq(3));
}

TEST(FbsVar, DataVec3f) {
  auto fbb = Build([](flatbuffers::FlatBufferBuilder& fbb) {
    fbs::Vec3f v(1.1f, 2.2f, 3.3f);
    return fbs::CreateDataVec3f(fbb, &v);
  });

  Var var;
  auto res = TryReadFbs(fbs::VarDef::DataVec3f, GetRoot(fbb), &var);

  EXPECT_TRUE(res);
  EXPECT_TRUE(var.Is<vec3>());
  EXPECT_THAT(var.Get<vec3>()->x, Eq(1.1f));
  EXPECT_THAT(var.Get<vec3>()->y, Eq(2.2f));
  EXPECT_THAT(var.Get<vec3>()->z, Eq(3.3f));
}

TEST(FbsVar, DataVec4i) {
  auto fbb = Build([](flatbuffers::FlatBufferBuilder& fbb) {
    fbs::Vec4i v(1, 2, 3, 4);
    return fbs::CreateDataVec4i(fbb, &v);
  });

  Var var;
  auto res = TryReadFbs(fbs::VarDef::DataVec4i, GetRoot(fbb), &var);

  EXPECT_TRUE(res);
  EXPECT_TRUE(var.Is<vec4i>());
  EXPECT_THAT(var.Get<vec4i>()->x, Eq(1));
  EXPECT_THAT(var.Get<vec4i>()->y, Eq(2));
  EXPECT_THAT(var.Get<vec4i>()->z, Eq(3));
  EXPECT_THAT(var.Get<vec4i>()->w, Eq(4));
}

TEST(FbsVar, DataVec4f) {
  auto fbb = Build([](flatbuffers::FlatBufferBuilder& fbb) {
    fbs::Vec4f v(1.1f, 2.2f, 3.3f, 4.4f);
    return fbs::CreateDataVec4f(fbb, &v);
  });

  Var var;
  auto res = TryReadFbs(fbs::VarDef::DataVec4f, GetRoot(fbb), &var);

  EXPECT_TRUE(res);
  EXPECT_TRUE(var.Is<vec4>());
  EXPECT_THAT(var.Get<vec4>()->x, Eq(1.1f));
  EXPECT_THAT(var.Get<vec4>()->y, Eq(2.2f));
  EXPECT_THAT(var.Get<vec4>()->z, Eq(3.3f));
  EXPECT_THAT(var.Get<vec4>()->w, Eq(4.4f));
}

TEST(FbsVar, DataQuatf) {
  auto fbb = Build([](flatbuffers::FlatBufferBuilder& fbb) {
    fbs::Quatf q(1.1f, 2.2f, 3.3f, 4.4f);
    return fbs::CreateDataQuatf(fbb, &q);
  });

  Var var;
  auto res = TryReadFbs(fbs::VarDef::DataQuatf, GetRoot(fbb), &var);

  EXPECT_TRUE(res);
  EXPECT_TRUE(var.Is<quat>());

  // Quaternions are automatically normalized.
  const auto n = vec4(1.1f, 2.2f, 3.3f, 4.4f).Normalized();
  EXPECT_THAT(var.Get<quat>()->w, FloatNear(n.w, kEps));
  EXPECT_THAT(var.Get<quat>()->x, FloatNear(n.x, kEps));
  EXPECT_THAT(var.Get<quat>()->y, FloatNear(n.y, kEps));
  EXPECT_THAT(var.Get<quat>()->z, FloatNear(n.z, kEps));
}

TEST(FbsVar, DataString) {
  auto fbb = Build([](flatbuffers::FlatBufferBuilder& fbb) {
    return fbs::CreateDataString(fbb, fbb.CreateString("hello"));
  });

  Var var;
  auto res = TryReadFbs(fbs::VarDef::DataString, GetRoot(fbb), &var);

  EXPECT_TRUE(res);
  EXPECT_TRUE(var.Is<std::string>());
  EXPECT_THAT(*var.Get<std::string>(), Eq("hello"));
}

TEST(FbsVar, DataHashVal) {
  auto fbb = Build([](flatbuffers::FlatBufferBuilder& fbb) {
    fbs::HashVal h(12345);
    return fbs::CreateDataHashVal(fbb, &h);
  });

  Var var;
  auto res = TryReadFbs(fbs::VarDef::DataHashVal, GetRoot(fbb), &var);

  EXPECT_TRUE(res);
  EXPECT_TRUE(var.Is<HashValue>());
  EXPECT_THAT(var.Get<HashValue>()->get(), Eq(12345));
}

TEST(FbsVar, DataBytes) {
  auto fbb = Build([](flatbuffers::FlatBufferBuilder& fbb) {
    std::vector<uint8_t> bytes = {1, 2, 3, 4, 5, 6, 7, 8};
    return fbs::CreateDataBytes(fbb, fbb.CreateVector(bytes));
  });

  Var var;
  auto res = TryReadFbs(fbs::VarDef::DataBytes, GetRoot(fbb), &var);

  EXPECT_TRUE(res);
  EXPECT_TRUE(var.Is<ByteVector>());

  auto vec = var.Get<ByteVector>();
  EXPECT_THAT(vec->size(), Eq(8));
  for (int i = 0; i < 8; ++i) {
    EXPECT_THAT(static_cast<int>((*vec)[i]), Eq(i + 1));
  }
}

TEST(FbsVar, VarArray) {
  auto fbb = Build([&](flatbuffers::FlatBufferBuilder& fbb) {
    std::vector<flatbuffers::Offset<fbs::VarArrayDefImpl>> values;
    auto insert = [&](auto type, const auto& value) {
      values.push_back(CreateVarArrayDefImpl(fbb, type, value.Union()));
    };

    insert(fbs::VarDef::DataInt, fbs::CreateDataInt(fbb, 123));
    insert(fbs::VarDef::DataFloat, fbs::CreateDataFloat(fbb, 456.f));
    insert(fbs::VarDef::DataString, fbs::CreateDataStringDirect(fbb, "hey"));
    return CreateVarArrayDefDirect(fbb, &values);
  });

  Var var;
  auto res = TryReadFbs(fbs::VarDef::VarArrayDef, GetRoot(fbb), &var);

  EXPECT_TRUE(res);
  EXPECT_TRUE(var.Is<VarArray>());
  EXPECT_THAT(var.Count(), Eq(3));
  EXPECT_TRUE(var[0].Is<int>());
  EXPECT_THAT(*var[0].Get<int>(), Eq(123));
  EXPECT_TRUE(var[1].Is<float>());
  EXPECT_THAT(*var[1].Get<float>(), Eq(456.f));
  EXPECT_TRUE(var[2].Is<std::string>());
  EXPECT_THAT(*var[2].Get<std::string>(), Eq("hey"));
}

TEST(FbsVar, VarTable) {
  const char* key0 = "key0";
  const char* key1 = "key1";
  const char* key2 = "key2";

  auto fbb = Build([&](flatbuffers::FlatBufferBuilder& fbb) {
    std::vector<flatbuffers::Offset<fbs::KeyVarPairDef>> values;
    auto insert = [&](const char* key, auto type, const auto& value) {
      const auto k = fbs::CreateHashStringDirect(fbb, key, Hash(key).get());
      auto pair = fbs::CreateKeyVarPairDef(fbb, k, type, value.Union());
      values.push_back(pair);
    };

    insert(key0, fbs::VarDef::DataInt, fbs::CreateDataInt(fbb, 123));
    insert(key1, fbs::VarDef::DataFloat, fbs::CreateDataFloat(fbb, 456.f));
    insert(key2, fbs::VarDef::DataString,
           fbs::CreateDataStringDirect(fbb, "hello"));
    return CreateVarTableDefDirect(fbb, &values);
  });

  Var var;
  auto res = TryReadFbs(fbs::VarDef::VarTableDef, GetRoot(fbb), &var);

  EXPECT_TRUE(res);
  EXPECT_TRUE(var.Is<VarTable>());
  EXPECT_THAT(var.Count(), Eq(3));
  EXPECT_TRUE(var[Hash(key0)].Is<int>());
  EXPECT_THAT(*var[Hash(key0)].Get<int>(), Eq(123));
  EXPECT_TRUE(var[Hash(key1)].Is<float>());
  EXPECT_THAT(*var[Hash(key1)].Get<float>(), Eq(456.f));
  EXPECT_TRUE(var[Hash(key2)].Is<std::string>());
  EXPECT_THAT(*var[Hash(key2)].Get<std::string>(), Eq("hello"));
}

}  // namespace
}  // namespace redux
