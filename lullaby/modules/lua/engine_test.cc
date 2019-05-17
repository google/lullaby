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

#include "lullaby/modules/lua/engine.h"

#include "ion/base/logchecker.h"
#include "gtest/gtest.h"
#include "lullaby/util/built_in_functions.h"
#include "lullaby/util/time.h"
#include "lullaby/util/entity.h"

namespace lull {
namespace script {
namespace lua {

enum EnumTest {
  ZEROTH,
  FIRST,
  SECOND,
};

class LuaEngineTest : public testing::Test {
 public:
  Engine lua_;
  std::unique_ptr<ion::base::LogChecker> log_checker_;

  void SetUp() override {
    RegisterBuiltInFunctions(&lua_);
    log_checker_.reset(new ion::base::LogChecker());
  }
};

TEST_F(LuaEngineTest, SimpleScript) {
  uint64_t id = lua_.LoadScript("y = (3 + x) * 2 + 1", "SimpleScript");
  lua_.SetValue(id, "x", 10);
  lua_.RunScript(id);
  int value;
  EXPECT_TRUE(lua_.GetValue(id, "y", &value));
  EXPECT_EQ(27, value);
  EXPECT_FALSE(log_checker_->HasAnyMessages());
}

TEST_F(LuaEngineTest, RegisterFunction) {
  int x = 17;
  lua_.RegisterFunction("Blah", [x](int y) { return x + y; });
  lua_.RegisterFunction("lull.Blah", [x](int y) { return x - y; });
  lua_.RegisterFunction("lull.a.Blah", [x](int y) { return x * y; });
  lua_.RegisterFunction("lull.a.b.Blah", [x](int y) { return x / y; });
  lua_.RegisterFunction("lull.a.b.c.Blah", [x](int y) { return x % y; });
  uint64_t id = lua_.LoadScript(
      R"(
        a = Blah(7)
        b = lull.Blah(11)
        c = lull.a.Blah(2)
        d = lull.a.b.Blah(4)
        e = lull.a.b.c.Blah(6)
      )",
      "RegisterFunctionScript");
  lua_.RunScript(id);
  int value;
  EXPECT_TRUE(lua_.GetValue(id, "a", &value));
  EXPECT_EQ(24, value);
  EXPECT_TRUE(lua_.GetValue(id, "b", &value));
  EXPECT_EQ(6, value);
  EXPECT_TRUE(lua_.GetValue(id, "c", &value));
  EXPECT_EQ(34, value);
  EXPECT_TRUE(lua_.GetValue(id, "d", &value));
  EXPECT_EQ(4, value);
  EXPECT_TRUE(lua_.GetValue(id, "e", &value));
  EXPECT_EQ(5, value);
  EXPECT_FALSE(log_checker_->HasAnyMessages());
}

TEST_F(LuaEngineTest, Converter) {
  lua_.RegisterFunction("FlipBool", [](bool x) { return !x; });
  lua_.RegisterFunction("AddEnum", [](EnumTest x) -> EnumTest {
    return static_cast<EnumTest>(x + 1);
  });
  lua_.RegisterFunction("AddInt16", [](int16_t x) -> int16_t { return ++x; });
  lua_.RegisterFunction("AddInt32", [](int32_t x) -> int32_t { return ++x; });
  lua_.RegisterFunction("AddInt64", [](int64_t x) -> int64_t { return ++x; });
  lua_.RegisterFunction("AddUint16",
                        [](uint16_t x) -> uint16_t { return ++x; });
  lua_.RegisterFunction("AddUint32",
                        [](uint32_t x) -> uint32_t { return ++x; });
  lua_.RegisterFunction("AddUint64",
                        [](uint64_t x) -> uint64_t { return ++x; });
  lua_.RegisterFunction("AddDuration",
                        [](Clock::duration x) -> Clock::duration {
                          return x + DurationFromSeconds(1);
                        });
  lua_.RegisterFunction("AddEntity", [](lull::Entity x) -> lull::Entity {
    return x.AsUint32() + 1;
  });
  lua_.RegisterFunction("RepeatString",
                        [](std::string s) -> std::string { return s + s; });
  lua_.RegisterFunction("RotateVec2",
                        [](mathfu::vec2 v) { return mathfu::vec2(v.y, v.x); });
  lua_.RegisterFunction(
      "RotateVec3", [](mathfu::vec3 v) { return mathfu::vec3(v.y, v.z, v.x); });
  lua_.RegisterFunction("RotateVec4", [](mathfu::vec4 v) {
    return mathfu::vec4(v.y, v.z, v.w, v.x);
  });
  lua_.RegisterFunction("RotateQuat", [](mathfu::quat v) {
    return mathfu::quat(v.vector().x, v.vector().y, v.vector().z, v.scalar());
  });
  lua_.RegisterFunction("ReverseVector", [](const std::vector<int>& v) {
    std::vector<int> vv(v);
    for (size_t i = 0; i < vv.size() / 2; ++i) {
      std::swap(vv[i], vv[vv.size() - 1 - i]);
    }
    return vv;
  });
  lua_.RegisterFunction("FlipVec2Vector",
                        [](const std::vector<mathfu::vec2>& v) {
                          std::vector<mathfu::vec2> vv(v);
                          for (auto& a : vv) {
                            float x = a.x;
                            a.x = a.y;
                            a.y = x;
                          }
                          return vv;
                        });
  lua_.RegisterFunction("InvertMap", [](const std::map<std::string, int>& m) {
    std::map<int, std::string> n;
    for (const auto& kv : m) {
      n[kv.second] = kv.first;
    }
    return n;
  });
  lua_.RegisterFunction("VectorizeStrings",
                        [](const std::unordered_map<int, std::string>& m) {
                          std::unordered_map<int, std::vector<std::string>> n;
                          for (const auto& kv : m) {
                            for (char c : kv.second) {
                              n[kv.first].emplace_back(1, c);
                            }
                          }
                          return n;
                        });
  lua_.RegisterFunction("TransposeMatrix",
                        [](const mathfu::mat4 m) { return m.Transpose(); });
  lua_.RegisterFunction("FlipAabb", [](const lull::Aabb aabb) {
    return lull::Aabb(aabb.max, aabb.min);
  });
  lua_.RegisterFunction("FlipRect", [](const mathfu::rectf rect) {
    return mathfu::rectf(rect.size, rect.pos);
  });
  lua_.RegisterFunction("DoubleOptional",
                        [](const lull::Optional<float> optional) {
                          if (optional) {
                            return lull::Optional<float>(*optional * 2.f);
                          } else {
                            return lull::Optional<float>();
                          }
                        });

  uint64_t id = lua_.LoadScript(
      R"(
        bool = FlipBool(true)
        enum = AddEnum(1)
        int16 = AddInt16(123)
        int32 = AddInt32(123)
        int64 = AddInt64(123)
        uint16 = AddUint16(123)
        uint32 = AddUint32(123)
        uint64 = AddUint64(123)
        duration = AddDuration(durationFromMilliseconds(234))
        entity = AddEntity(123)
        str = RepeatString("abc")
        vec2 = RotateVec2({x=1, y=2})
        vec3 = RotateVec3({x=1, y=2, z=3})
        vec4 = RotateVec4({x=1, y=2, z=3, w=4})
        quat = RotateQuat({x=1, y=2, z=3, s=4})
        mat4 = TransposeMatrix({
          c0 = {x=1, y=2, z=3, w=4},
          c1 = {x=5, y=6, z=7, w=8},
          c2 = {x=9, y=10, z=11, w=12},
          c3 = {x=13, y=14, z=15, w=16},
        })
        vector = ReverseVector({1, 2, 3, 4, 5})
        vec2v = FlipVec2Vector({{x=1, y=2}, {x=3, y=4}, {x=5, y=6}})
        map = InvertMap({a=1, b=2, c=3, d=4})
        umap = VectorizeStrings({'abc', 'defg'})
        aabb = FlipAabb({min={x=1, y=2, z=3}, max={x=4, y=5, z=6}})
        rect = FlipRect({pos={x=1, y=2}, size={x=3, y=4}})
        optional = DoubleOptional(4);
        optional_nil = DoubleOptional(nil);
      )",
      "ConverterScript");

  lua_.RunScript(id);

  bool bool_value;
  EXPECT_TRUE(lua_.GetValue(id, "bool", &bool_value));
  EXPECT_EQ(false, bool_value);
  EnumTest enum_value;
  EXPECT_TRUE(lua_.GetValue(id, "enum", &enum_value));
  EXPECT_EQ(SECOND, enum_value);
  int16_t int16_value;
  EXPECT_TRUE(lua_.GetValue(id, "int16", &int16_value));
  EXPECT_EQ(124, int16_value);
  int32_t int32_value;
  EXPECT_TRUE(lua_.GetValue(id, "int32", &int32_value));
  EXPECT_EQ(124, int32_value);
  int64_t int64_value;
  EXPECT_TRUE(lua_.GetValue(id, "int64", &int64_value));
  EXPECT_EQ(124, int64_value);
  uint16_t uint16_value;
  EXPECT_TRUE(lua_.GetValue(id, "uint16", &uint16_value));
  EXPECT_EQ(124u, uint16_value);
  uint32_t uint32_value;
  EXPECT_TRUE(lua_.GetValue(id, "uint32", &uint32_value));
  EXPECT_EQ(124u, uint32_value);
  uint64_t uint64_value;
  EXPECT_TRUE(lua_.GetValue(id, "uint64", &uint64_value));
  EXPECT_EQ(124u, uint64_value);
  Clock::duration duration_value;
  EXPECT_TRUE(lua_.GetValue(id, "duration", &duration_value));
  EXPECT_EQ(duration_value, DurationFromMilliseconds(1234));
  lull::Entity entity_value;
  EXPECT_TRUE(lua_.GetValue(id, "entity", &entity_value));
  EXPECT_EQ(entity_value, 124u);
  std::string str_value;
  EXPECT_TRUE(lua_.GetValue(id, "str", &str_value));
  EXPECT_EQ("abcabc", str_value);

  mathfu::vec2 vec2_value;
  EXPECT_TRUE(lua_.GetValue(id, "vec2", &vec2_value));
  EXPECT_EQ(mathfu::vec2(2, 1), vec2_value);
  mathfu::vec3 vec3_value;
  EXPECT_TRUE(lua_.GetValue(id, "vec3", &vec3_value));
  EXPECT_EQ(mathfu::vec3(2, 3, 1), vec3_value);
  mathfu::vec4 vec4_value;
  EXPECT_TRUE(lua_.GetValue(id, "vec4", &vec4_value));
  EXPECT_EQ(mathfu::vec4(2, 3, 4, 1), vec4_value);

  mathfu::quat quat;
  EXPECT_TRUE(lua_.GetValue(id, "quat", &quat));
  EXPECT_EQ(mathfu::vec3(2, 3, 4), quat.vector());
  EXPECT_EQ(1, quat.scalar());

  mathfu::mat4 mat;
  EXPECT_TRUE(lua_.GetValue(id, "mat4", &mat));
  EXPECT_EQ(mathfu::vec4(1, 5, 9, 13), mat.GetColumn(0));
  EXPECT_EQ(mathfu::vec4(2, 6, 10, 14), mat.GetColumn(1));
  EXPECT_EQ(mathfu::vec4(3, 7, 11, 15), mat.GetColumn(2));
  EXPECT_EQ(mathfu::vec4(4, 8, 12, 16), mat.GetColumn(3));

  std::vector<int> vect;
  EXPECT_TRUE(lua_.GetValue(id, "vector", &vect));
  EXPECT_EQ(5, vect[0]);
  EXPECT_EQ(4, vect[1]);
  EXPECT_EQ(3, vect[2]);
  EXPECT_EQ(2, vect[3]);
  EXPECT_EQ(1, vect[4]);

  std::vector<mathfu::vec2> vec2v;
  EXPECT_TRUE(lua_.GetValue(id, "vec2v", &vec2v));
  EXPECT_EQ(mathfu::vec2(2, 1), vec2v[0]);
  EXPECT_EQ(mathfu::vec2(4, 3), vec2v[1]);
  EXPECT_EQ(mathfu::vec2(6, 5), vec2v[2]);

  std::map<int, std::string> m;
  EXPECT_TRUE(lua_.GetValue(id, "map", &m));
  EXPECT_EQ(4u, m.size());
  EXPECT_EQ("b", m[2]);
  EXPECT_EQ("c", m[3]);
  EXPECT_EQ("d", m[4]);

  std::unordered_map<int, std::vector<std::string>> um;
  EXPECT_TRUE(lua_.GetValue(id, "umap", &um));
  EXPECT_EQ(2u, um.size());
  EXPECT_EQ(3u, um[1].size());
  EXPECT_EQ("a", um[1][0]);
  EXPECT_EQ("b", um[1][1]);
  EXPECT_EQ("c", um[1][2]);
  EXPECT_EQ(4u, um[2].size());
  EXPECT_EQ("d", um[2][0]);
  EXPECT_EQ("e", um[2][1]);
  EXPECT_EQ("f", um[2][2]);
  EXPECT_EQ("g", um[2][3]);

  lull::Aabb aabb;
  EXPECT_TRUE(lua_.GetValue(id, "aabb", &aabb));
  EXPECT_EQ(mathfu::vec3(4, 5, 6), aabb.min);
  EXPECT_EQ(mathfu::vec3(1, 2, 3), aabb.max);

  mathfu::rectf rect;
  EXPECT_TRUE(lua_.GetValue(id, "rect", &rect));
  EXPECT_EQ(mathfu::vec2(3, 4), rect.pos);
  EXPECT_EQ(mathfu::vec2(1, 2), rect.size);

  lull::Optional<float> optional;
  EXPECT_TRUE(lua_.GetValue(id, "optional", &optional));
  EXPECT_EQ(true, static_cast<bool>(optional));
  EXPECT_EQ(8.f, *optional);

  lull::Optional<float> optional_nil;
  EXPECT_TRUE(lua_.GetValue(id, "optional_nil", &optional_nil));
  EXPECT_EQ(false, static_cast<bool>(optional_nil));

  EXPECT_FALSE(log_checker_->HasAnyMessages());
}

struct Foo {
  std::string name;
  int x, y;
  Foo() {}
  Foo(std::string name, int x, int y) : name(name), x(x), y(y) {}
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&name, Hash("name"));
    archive(&x, Hash("x"));
    archive(&y, Hash("y"));
  }
};

struct Bar {
  std::string name;
  int z;
  Bar() {}
  Bar(std::string name, int z) : name(name), z(z) {}
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&name, Hash("name"));
    archive(&z, Hash("z"));
  }
};

struct Baz {
  Foo foo;
  Bar bar;
  Baz() {}
  Baz(Foo foo, Bar bar) : foo(foo), bar(bar) {}
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&foo, Hash("foo"));
    archive(&bar, Hash("bar"));
  }
};

TEST_F(LuaEngineTest, Serializables) {
  lua_.RegisterFunction("ConvertFooToBar", [](const Foo& foo) {
    return Bar(foo.name, foo.x + foo.y);
  });
  lua_.RegisterFunction(
      "ConvertFooBarToBaz",
      [](const Foo& foo, const Bar& bar) { return Baz(foo, bar); });

  uint64_t id = lua_.LoadScript(
      R"(
        bar1 = ConvertFooToBar(foo1)
        bar2 = ConvertFooToBar({name = "lua foo", x = -8, y = 4})
        bar3 = {name = "bar from " .. bar1.name, z = bar1.z + 10}
        bar1.z = 35
        baz1 = ConvertFooBarToBaz(foo1, bar3)
      )",
      "ConverterScript");

  lua_.SetValue(id, "foo1", Foo("native foo", 5, 3));

  lua_.RunScript(id);

  Bar bar1, bar2, bar3;
  EXPECT_TRUE(lua_.GetValue(id, "bar1", &bar1));
  EXPECT_EQ("native foo", bar1.name);
  EXPECT_EQ(35, bar1.z);

  EXPECT_TRUE(lua_.GetValue(id, "bar2", &bar2));
  EXPECT_EQ("lua foo", bar2.name);
  EXPECT_EQ(-4, bar2.z);

  EXPECT_TRUE(lua_.GetValue(id, "bar3", &bar3));
  EXPECT_EQ("bar from native foo", bar3.name);
  EXPECT_EQ(18, bar3.z);

  Baz baz1;
  EXPECT_TRUE(lua_.GetValue(id, "baz1", &baz1));
  EXPECT_EQ("native foo", baz1.foo.name);
  EXPECT_EQ(5, baz1.foo.x);
  EXPECT_EQ(3, baz1.foo.y);
  EXPECT_EQ("bar from native foo", baz1.bar.name);
  EXPECT_EQ(18, baz1.bar.z);

  EXPECT_FALSE(log_checker_->HasAnyMessages());
}

TEST_F(LuaEngineTest, Mathfu) {
  uint64_t id = lua_.LoadScript(
      R"(
        v2b = -vec2(1, 4) + (1 + vec2(-3, 2) + 5) - (2 - vec2(-1, 5) - 1) *
            (2 * vec2(14, 2) * -3) / (42 / v2a / 3)
        v2eq = (vec2(1, 3) == vec2(1, 3)) and (vec2(4, 2) ~= vec2(6, 2))
        v3b = -vec3(1, 4, 5) + (1 + vec3(-3, 2, -4) + 5) -
            (2 - vec3(-1, 5, -6) - 1) * (2 * vec3(14, 2, 7) * -3) /
            (42 / v3a / 3)
        v3eq = (vec3(1, 3, 8) == vec3(1, 3, 8)) and
            (vec3(4, 2, 5) ~= vec3(6, 2, 7))
        v4b = -vec4(1, 4, 5, -10) + (1 + vec4(-3, 2, -4, 7) + 5) -
            (2 - vec4(-1, 5, -6, 3) - 1) * (2 * vec4(14, 2, 7, 28) * -3) /
            (42 / v4a / 3)
        v4eq = (vec4(1, 3, 8, 4) == vec4(1, 3, 8, 4)) and
            (vec4(4, 2, 5, 0) ~= vec4(6, 2, 7, -1))
        qb = qa * quat(5, 6, 7, 8)
        v3c = qa * vec3(3, -1, 2)
        qc = qa * 5
        qd = 7 * qa
      )",
      "MathfuScript");

  lua_.SetValue(id, "v2a", mathfu::vec2(2, 7));
  lua_.SetValue(id, "v3a", mathfu::vec3(2, 7, -2));
  lua_.SetValue(id, "v4a", mathfu::vec4(2, 7, -2, 1));
  lua_.SetValue(id, "qa", mathfu::quat(1, 2, 3, 4));

  lua_.RunScript(id);

  mathfu::vec2 v2b;
  EXPECT_TRUE(lua_.GetValue(id, "v2b", &v2b));
  EXPECT_EQ(26, v2b.x);
  EXPECT_EQ(-20, v2b.y);

  bool v2eq;
  EXPECT_TRUE(lua_.GetValue(id, "v2eq", &v2eq));
  EXPECT_TRUE(v2eq);

  mathfu::vec3 v3b;
  EXPECT_TRUE(lua_.GetValue(id, "v3b", &v3b));
  EXPECT_EQ(26, v3b.x);
  EXPECT_EQ(-20, v3b.y);
  EXPECT_EQ(-45, v3b.z);

  bool v3eq;
  EXPECT_TRUE(lua_.GetValue(id, "v3eq", &v3eq));
  EXPECT_TRUE(v3eq);

  mathfu::vec4 v4b;
  EXPECT_TRUE(lua_.GetValue(id, "v4b", &v4b));
  EXPECT_EQ(26, v4b.x);
  EXPECT_EQ(-20, v4b.y);
  EXPECT_EQ(-45, v4b.z);
  EXPECT_EQ(-1, v4b.w);

  bool v4eq;
  EXPECT_TRUE(lua_.GetValue(id, "v4eq", &v4eq));
  EXPECT_TRUE(v4eq);

  mathfu::quat qb;
  EXPECT_TRUE(lua_.GetValue(id, "qb", &qb));
  EXPECT_EQ(-60, qb.scalar());
  EXPECT_EQ(12, qb.vector().x);
  EXPECT_EQ(30, qb.vector().y);
  EXPECT_EQ(24, qb.vector().z);

  mathfu::vec3 v3c;
  EXPECT_TRUE(lua_.GetValue(id, "v3c", &v3c));
  EXPECT_EQ(67, v3c.x);
  EXPECT_EQ(81, v3c.y);
  EXPECT_EQ(68, v3c.z);

  mathfu::quat qc;
  mathfu::quat expectedQc = mathfu::quat(1, 2, 3, 4) * 5.0;
  EXPECT_TRUE(lua_.GetValue(id, "qc", &qc));
  EXPECT_NEAR(expectedQc.scalar(), qc.scalar(), 1e-6);
  EXPECT_NEAR(expectedQc.vector().x, qc.vector().x, 1e-6);
  EXPECT_NEAR(expectedQc.vector().y, qc.vector().y, 1e-6);
  EXPECT_NEAR(expectedQc.vector().z, qc.vector().z, 1e-6);

  mathfu::quat qd;
  mathfu::quat expectedQd = mathfu::quat(1, 2, 3, 4) * 7.0;
  EXPECT_TRUE(lua_.GetValue(id, "qd", &qd));
  EXPECT_NEAR(expectedQd.scalar(), qd.scalar(), 1e-6);
  EXPECT_NEAR(expectedQd.vector().x, qd.vector().x, 1e-6);
  EXPECT_NEAR(expectedQd.vector().y, qd.vector().y, 1e-6);
  EXPECT_NEAR(expectedQd.vector().z, qd.vector().z, 1e-6);

  EXPECT_FALSE(log_checker_->HasAnyMessages());
}

TEST_F(LuaEngineTest, Duration) {
  uint64_t id = lua_.LoadScript(
      R"(
        eq0 = (ms05 == sec1)
        eq1 = (ms1 == sec1)
        eq2 = (ms1 == (ms05 + sec05))
        eq3 = (sec1 == durationFromSeconds(1))
        eq4 = (ms05 == durationFromMilliseconds(500))
        eq5 = (0.5 == secondsFromDuration(sec05))
        eq6 = (1000 == millisecondsFromDuration(ms1))
      )",
      "DurationScript");

  lua_.SetValue(id, "ms1", DurationFromMilliseconds(1000));
  lua_.SetValue(id, "ms05", DurationFromMilliseconds(500));
  lua_.SetValue(id, "sec1", DurationFromSeconds(1));
  lua_.SetValue(id, "sec05", DurationFromSeconds(0.5f));

  lua_.RunScript(id);

  bool eq0, eq1, eq2, eq3, eq4, eq5, eq6;
  EXPECT_TRUE(lua_.GetValue(id, "eq0", &eq0));
  EXPECT_TRUE(lua_.GetValue(id, "eq1", &eq1));
  EXPECT_TRUE(lua_.GetValue(id, "eq2", &eq2));
  EXPECT_TRUE(lua_.GetValue(id, "eq3", &eq3));
  EXPECT_TRUE(lua_.GetValue(id, "eq4", &eq4));
  EXPECT_TRUE(lua_.GetValue(id, "eq5", &eq5));
  EXPECT_TRUE(lua_.GetValue(id, "eq6", &eq6));
  EXPECT_FALSE(eq0);
  EXPECT_TRUE(eq1);
  EXPECT_TRUE(eq2);
  EXPECT_TRUE(eq3);
  EXPECT_TRUE(eq4);
  EXPECT_TRUE(eq5);
  EXPECT_TRUE(eq6);

  EXPECT_FALSE(log_checker_->HasAnyMessages());
}

TEST_F(LuaEngineTest, EventWrappers) {
  lua_.RegisterFunction("EventWrapperFooToBar", [](const EventWrapper& foo) {
    Bar b(*foo.GetValue<std::string>(Hash("name")),
          *foo.GetValue<int>(Hash("x")) + *foo.GetValue<int>(Hash("y")));
    EventWrapper e(b), c(e);  // Because reasons.
    return c;
  });

  uint64_t id = lua_.LoadScript(
      R"(
        bar1 = EventWrapperFooToBar(foo1)
        bar2 = EventWrapperFooToBar({
          type = hash("lull::script::lua::Foo"),
          data = {
            name = {type = hash("std::string"), data = "lua foo"},
            x = {type = hash("int32_t"), data = -8},
            y = {type = hash("int32_t"), data = 4}
          }
        })
        bar3 = {
          type = hash("lull::script::lua::Bar"),
          data = {
            name = {type = hash("std::string"), data = bar1.data.name.data},
            z = {type = hash("int32_t"), data = bar1.data.z.data + 10}
          }
        }
        bar1.data.z.data = 35
      )",
      "ConverterScript");

  Foo foo1("native foo", 5, 3);
  lua_.SetValue(id, "foo1", EventWrapper(foo1));

  lua_.RunScript(id);

  EventWrapper bar1, bar2, bar3;
  ASSERT_TRUE(lua_.GetValue(id, "bar1", &bar1));
  ASSERT_NE(nullptr, bar1.GetValue<std::string>(Hash("name")));
  EXPECT_EQ("native foo", *bar1.GetValue<std::string>(Hash("name")));
  ASSERT_NE(nullptr, bar1.GetValue<int>(Hash("z")));
  EXPECT_EQ(35, *bar1.GetValue<int>(Hash("z")));

  ASSERT_TRUE(lua_.GetValue(id, "bar2", &bar2));
  ASSERT_NE(nullptr, bar2.GetValue<std::string>(Hash("name")));
  EXPECT_EQ("lua foo", *bar2.GetValue<std::string>(Hash("name")));
  ASSERT_NE(nullptr, bar2.GetValue<int>(Hash("z")));
  EXPECT_EQ(-4, *bar2.GetValue<int>(Hash("z")));

  ASSERT_TRUE(lua_.GetValue(id, "bar3", &bar3));
  ASSERT_NE(nullptr, bar3.GetValue<std::string>(Hash("name")));
  EXPECT_EQ("native foo", *bar3.GetValue<std::string>(Hash("name")));
  ASSERT_NE(nullptr, bar3.GetValue<int>(Hash("z")));
  EXPECT_EQ(18, *bar3.GetValue<int>(Hash("z")));

  EXPECT_FALSE(log_checker_->HasAnyMessages());
}

TEST_F(LuaEngineTest, FunctionConversions) {
  std::function<int(int, int)> intCallback;
  std::function<void(int, int)> voidCallback;

  lua_.RegisterFunction(
      "SetIntCallback",
      [&intCallback](const std::function<int(int, int)>& func) {
        intCallback = func;
      });
  lua_.RegisterFunction(
      "SetVoidCallback",
      [&voidCallback](const std::function<void(int, int)>& func) {
        voidCallback = func;
      });

  lua_.RegisterFunction("BindStrCatFunction",
                        [](const std::string& a)
                            -> std::function<std::string(const std::string&)> {
                          return [a](const std::string& b) { return a + b; };
                        });

  uint64_t id = lua_.LoadScript(
      R"(
        -- Passing functions as arguments.
        SetIntCallback(function(a, b) return a * b end)
        c = -1
        SetVoidCallback(function(a, b) c = a + b end)

        -- Receiving functions as return values.
        BoundStrCat = BindStrCatFunction("Hello")
        str = BoundStrCat("World")

        -- Calling functions inserted into the environment.
        t = Subtract(29, 34)

        -- Creating functions that can be pulled out of the environment.
        function Divide(a, b) return a / b end
      )",
      "CallbackScript");

  lua_.SetValue(id, "Subtract", std::function<int(int, int)>([](int a, int b) {
                  return a - b;
                }));
  lua_.RunScript(id);

  EXPECT_EQ(8, intCallback(2, 4));
  EXPECT_EQ(65, intCallback(5, 13));

  int c;
  voidCallback(2, 4);
  EXPECT_TRUE(lua_.GetValue(id, "c", &c));
  EXPECT_EQ(6, c);
  voidCallback(5, 13);
  EXPECT_TRUE(lua_.GetValue(id, "c", &c));
  EXPECT_EQ(18, c);

  std::string str;
  EXPECT_TRUE(lua_.GetValue(id, "str", &str));
  EXPECT_EQ("HelloWorld", str);

  int t;
  EXPECT_TRUE(lua_.GetValue(id, "t", &t));
  EXPECT_EQ(-5, t);

  std::function<int(int, int)> divideInt;
  EXPECT_TRUE(lua_.GetValue(id, "Divide", &divideInt));
  EXPECT_EQ(3, divideInt(13, 4));
  std::function<double(double, double)> divideDouble;
  EXPECT_TRUE(lua_.GetValue(id, "Divide", &divideDouble));
  EXPECT_EQ(3.25, divideDouble(13, 4));

  EXPECT_FALSE(log_checker_->HasAnyMessages());
}

TEST_F(LuaEngineTest, Environment) {
  uint64_t id = lua_.LoadScript("x = x + 1", "EnvironmentScript");
  lua_.SetValue(id, "x", 10);
  lua_.RunScript(id);
  int value;
  EXPECT_TRUE(lua_.GetValue(id, "x", &value));
  EXPECT_EQ(11, value);
  lua_.RunScript(id);
  EXPECT_TRUE(lua_.GetValue(id, "x", &value));
  EXPECT_EQ(12, value);
  lua_.RunScript(id);
  EXPECT_TRUE(lua_.GetValue(id, "x", &value));
  EXPECT_EQ(13, value);
  lua_.RunScript(id);
  EXPECT_TRUE(lua_.GetValue(id, "x", &value));
  EXPECT_EQ(14, value);
  uint64_t id2 = lua_.LoadScript("x = x + 1", "EnvironmentScript");
  lua_.SetValue(id2, "x", 3);
  lua_.RunScript(id2);
  EXPECT_TRUE(lua_.GetValue(id2, "x", &value));
  EXPECT_EQ(4, value);
  lua_.RunScript(id2);
  lua_.RunScript(id);
  EXPECT_TRUE(lua_.GetValue(id, "x", &value));
  EXPECT_EQ(15, value);
  EXPECT_FALSE(log_checker_->HasAnyMessages());
}

TEST_F(LuaEngineTest, Reload) {
  uint64_t id = lua_.LoadScript("x = 5", "ReloadScript");
  lua_.RunScript(id);
  int value;
  EXPECT_TRUE(lua_.GetValue(id, "x", &value));
  EXPECT_EQ(5, value);
  lua_.ReloadScript(id, "y = x * 2");
  lua_.RunScript(id);
  EXPECT_TRUE(lua_.GetValue(id, "y", &value));
  EXPECT_EQ(10, value);
  EXPECT_FALSE(log_checker_->HasAnyMessages());
}

TEST_F(LuaEngineTest, PrintFunction) {
  uint64_t id =
      lua_.LoadScript("print('abc', 123, true, 'xyz')", "PrintScript");
  lua_.RunScript(id);
  EXPECT_TRUE(log_checker_->HasMessage("INFO", "abc\t123\ttrue\txyz"));
}

TEST_F(LuaEngineTest, RecursiveScripts) {
  uint64_t id1 = lua_.LoadScript("x = 5", "InnerScript");
  lua_.RunScript(id1);
  lua_.RegisterFunction("GetX", [this, id1]() {
    int x = -1;
    EXPECT_TRUE(lua_.GetValue(id1, "x", &x));
    return x;
  });
  uint64_t id2 = lua_.LoadScript("y = GetX()", "OuterScript");
  lua_.RunScript(id2);
  int y = -1;
  EXPECT_TRUE(lua_.GetValue(id2, "y", &y));
  EXPECT_EQ(5, y);
  EXPECT_FALSE(log_checker_->HasAnyMessages());
}

TEST_F(LuaEngineTest, IncludeFunction) {
  lua_.SetLoadFileFunction([](const char* filename, std::string* out) {
    EXPECT_EQ(filename, std::string("thing.lua"));
    *out = R"(
          thing = 0

          function GetThing()
            return thing
          end

          function SetThing(t)
            thing = t
          end
        )";
    return true;
  });

  uint64_t id1 = lua_.LoadScript(
      R"(
        thing = include('thing.lua')
        thing.SetThing(1234)
      )",
      "IncludeScript1");
  uint64_t id2 = lua_.LoadScript(
      R"(
        thing = include('thing.lua')
        t = thing.GetThing()
      )",
      "IncludeScript2");

  lua_.RunScript(id1);
  lua_.RunScript(id2);

  int t = -1;
  EXPECT_TRUE(lua_.GetValue(id2, "t", &t));
  EXPECT_EQ(1234, t);
}

TEST_F(LuaEngineTest, UnloadScript) {
  uint64_t id = lua_.LoadScript("y = (3 + x) * 2 + 1", "SimpleScript");
  lua_.SetValue(id, "x", 10);
  lua_.RunScript(id);
  int value;
  EXPECT_TRUE(lua_.GetValue(id, "y", &value));
  EXPECT_EQ(27, value);

  size_t total_scripts = lua_.GetTotalScripts();
  lua_.UnloadScript(id);
  lua_.RunScript(id);

  EXPECT_TRUE(log_checker_->HasMessage(
      "ERROR", "Script error: attempt to call a nil value"));
  EXPECT_EQ(total_scripts - 1, lua_.GetTotalScripts());
}

TEST_F(LuaEngineTest, ConvertError) {
  int y = 4;
  lua_.RegisterFunction("FooInt", [y](int x) {});
  lua_.RegisterFunction("FooStrings", [](std::string x, std::string y) {});
  lua_.RegisterFunction("FooVecVecBool",
                        [](const std::vector<std::vector<bool>>& x) {});
  uint64_t id = lua_.LoadScript("FooInt('abc')", "FooIntScript");

  lua_.RunScript(id);
  EXPECT_TRUE(log_checker_->HasMessage(
      "ERROR",
      "Script error: [string \"FooIntScript\"]:1: FooInt expects the type "
      "of arg 1 to be number"));

  id = lua_.LoadScript("FooStrings('abc')", "FooStringsScript");
  lua_.RunScript(id);
  EXPECT_TRUE(log_checker_->HasMessage("ERROR",
                                       "Script error: [string "
                                       "\"FooStringsScript\"]:1: FooStrings "
                                       "expects 2 args, but got 1"));

  id = lua_.LoadScript("FooStrings('abc', {})", "FooStringsScript2");
  lua_.RunScript(id);
  EXPECT_TRUE(
      log_checker_->HasMessage("ERROR",
                               "Script error: [string "
                               "\"FooStringsScript2\"]:1: FooStrings "
                               "expects the type of arg 2 to be string"));

  id = lua_.LoadScript("FooVecVecBool('abc')", "FooVecVecBoolScript");
  lua_.RunScript(id);
  EXPECT_TRUE(log_checker_->HasMessage(
      "ERROR",
      "Script error: [string \"FooVecVecBoolScript\"]:1: FooVecVecBool expects "
      "the type of arg 1 to be table of table of boolean"));

  id = lua_.LoadScript("FooVecVecBool({{false, true}, {true}, {}})",
                       "FooVecVecBoolScript");
  lua_.RunScript(id);
  EXPECT_FALSE(log_checker_->HasAnyMessages());
}

TEST_F(LuaEngineTest, GetValueError) {
  uint64_t id = lua_.LoadScript("", "GetValueErrorScript");
  int intx;
  float floatx;
  std::vector<std::string> vectorx;
  lua_.SetValue(id, "x", 123);
  EXPECT_TRUE(lua_.GetValue(id, "x", &intx));
  EXPECT_EQ(123, intx);
  EXPECT_TRUE(lua_.GetValue(id, "x", &floatx));
  EXPECT_EQ(123, floatx);
  EXPECT_FALSE(lua_.GetValue(id, "x", &vectorx));
  EXPECT_FALSE(lua_.GetValue(id, "y", &intx));
  EXPECT_FALSE(log_checker_->HasAnyMessages());
}

TEST_F(LuaEngineTest, SyntaxError) {
  lua_.LoadScript("syntax error", "SyntaxErrorScript");
  EXPECT_TRUE(log_checker_->HasMessage("ERROR",
                                       "Error loading script: [string "
                                       "\"SyntaxErrorScript\"]:1: syntax error "
                                       "near 'error'"));
}

TEST_F(LuaEngineTest, RuntimeError) {
  uint64_t id = lua_.LoadScript("not_a_func()", "RuntimeErrorScript");
  lua_.SetValue(id, "not_a_func", 123);
  lua_.RunScript(id);
  EXPECT_TRUE(log_checker_->HasMessage(
      "ERROR",
      "Script error: [string \"RuntimeErrorScript\"]:1: attempt to call global "
      "'not_a_func' (a number value)"));
}

TEST_F(LuaEngineTest, ThrownError) {
  uint64_t id = lua_.LoadScript("error('oh no!')", "ThrownErrorScript");
  lua_.RunScript(id);
  EXPECT_TRUE(log_checker_->HasMessage(
      "ERROR", "Script error: [string \"ThrownErrorScript\"]:1: oh no!"));
}

TEST_F(LuaEngineTest, UnregisteredFunctionError) {
  lua_.RegisterFunction("func", []() {});
  uint64_t id = lua_.LoadScript("func()", "UnregisteredFunctionErrorScript");
  lua_.UnregisterFunction("func");
  lua_.RunScript(id);
  EXPECT_TRUE(log_checker_->HasMessage(
      "ERROR",
      "Script error: [string \"UnregisteredFunctionErrorScript\"]:1: Tried to "
      "call an unregistered function: func"));
}

TEST_F(LuaEngineTest, ReloadError) {
  uint64_t id = lua_.LoadScript("x = 5", "ReloadErrorScript");
  EXPECT_FALSE(log_checker_->HasAnyMessages());
  lua_.ReloadScript(id, "syntax error");
  EXPECT_TRUE(log_checker_->HasMessage("ERROR",
                                       "Error reloading script: [string "
                                       "\"ReloadErrorScript\"]:1: syntax error "
                                       "near 'error'"));
}

}  // namespace lua
}  // namespace script
}  // namespace lull

LULLABY_SETUP_TYPEID(lull::script::lua::Foo);
LULLABY_SETUP_TYPEID(lull::script::lua::Bar);
LULLABY_SETUP_TYPEID(lull::script::lua::Baz);
