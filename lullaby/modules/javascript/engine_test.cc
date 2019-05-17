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

#include "lullaby/modules/javascript/engine.h"

#include "ion/base/logchecker.h"
#include "gtest/gtest.h"
#include "lullaby/util/built_in_functions.h"
#include "lullaby/util/time.h"
#include "lullaby/util/entity.h"

namespace lull {
namespace script {
namespace javascript {

enum EnumTest {
  ZEROTH,
  FIRST,
  SECOND,
};

class JsEngineTest : public testing::Test {
 public:
  Engine js_;
  ion::base::LogChecker log_checker_;

  void SetUp() override { RegisterBuiltInFunctions(&js_); }
};

TEST_F(JsEngineTest, SimpleScript) {
  uint64_t id = js_.LoadScript("y = (3 + x) * 2 + 1", "SimpleScript");
  js_.SetValue(id, "x", 10);
  js_.RunScript(id);
  int value;
  EXPECT_TRUE(js_.GetValue(id, "y", &value));
  EXPECT_EQ(27, value);
  EXPECT_FALSE(log_checker_.HasAnyMessages());
}

TEST_F(JsEngineTest, RegisterFunction) {
  int x = 17;
  js_.RegisterFunction("Blah", [x](int y) { return x + y; });
  js_.RegisterFunction("lull.Blah", [x](int y) { return x - y; });
  js_.RegisterFunction("lull.a.Blah", [x](int y) { return x * y; });
  js_.RegisterFunction("lull.a.b.Blah", [x](int y) { return x / y; });
  js_.RegisterFunction("lull.a.b.c.Blah", [x](int y) { return x % y; });
  uint64_t id = js_.LoadScript(
      R"(
        a = Blah(7);
        b = lull.Blah(11);
        c = lull.a.Blah(2);
        d = lull.a.b.Blah(4);
        e = lull.a.b.c.Blah(6);
      )",
      "RegisterFunctionScript");
  js_.RunScript(id);
  int value;
  EXPECT_TRUE(js_.GetValue(id, "a", &value));
  EXPECT_EQ(24, value);
  EXPECT_TRUE(js_.GetValue(id, "b", &value));
  EXPECT_EQ(6, value);
  EXPECT_TRUE(js_.GetValue(id, "c", &value));
  EXPECT_EQ(34, value);
  EXPECT_TRUE(js_.GetValue(id, "d", &value));
  EXPECT_EQ(4, value);
  EXPECT_TRUE(js_.GetValue(id, "e", &value));
  EXPECT_EQ(5, value);
  EXPECT_FALSE(log_checker_.HasAnyMessages());
}

TEST_F(JsEngineTest, CallbackFunction) {
  js_.RegisterFunction(
      "BlahVoid",
      [](int i, std::function<void(int, int)>& callback) { callback(i, 7); });
  js_.RegisterFunction("Blah",
                       [](int i, std::function<int(int, int)>& callback) {
                         return callback(i, 6);
                       });
  js_.RegisterFunction("BlahZero",
                       [](std::function<void()>& callback) { callback(); });
  // Every time BlahRecurse is invoked, it creates a new std::function ("i + 5")
  // and hands it to the vm. This test checks that the vm does not leak it.
  js_.RegisterFunction(
      "BlahRecurse",
      [](std::function<void(std::function<int(int)>)>& callback) {
        callback([](int i) { return i + 5; });
      });
  uint64_t id = js_.LoadScript(
      R"(
      var a = 0;
      BlahVoid(6, function(i, j) { a = i + j; });

      var b = Blah(6, function(a, b) { return a + b; } );

      var c = 0;
      BlahZero(function() { c = 14; });

      var d = 0;
      BlahRecurse(function(fn) { d = fn(4);});
      )",
      "CallbackScript");
  js_.RunScript(id);
  int value;
  EXPECT_TRUE(js_.GetValue(id, "a", &value));
  EXPECT_EQ(13, value);
  EXPECT_TRUE(js_.GetValue(id, "b", &value));
  EXPECT_EQ(12, value);
  EXPECT_TRUE(js_.GetValue(id, "c", &value));
  EXPECT_EQ(14, value);
  EXPECT_TRUE(js_.GetValue(id, "d", &value));
  EXPECT_EQ(9, value);
  EXPECT_FALSE(log_checker_.HasAnyMessages());
}

TEST_F(JsEngineTest, RegisterFunctionWithTwoArgs) {
  int x = 5;
  js_.RegisterFunction("Blah", [x](int y) { return x + y; });
  js_.RegisterFunction("Blah2", [x](int y, int z) { return x + y + z; });
  uint64_t id =
      js_.LoadScript("a = Blah2(7, 8); b = Blah(7);", "RegisterFunctionScript");
  js_.RunScript(id);
  int value;
  EXPECT_TRUE(js_.GetValue(id, "a", &value));
  EXPECT_EQ(20, value);
  EXPECT_FALSE(log_checker_.HasAnyMessages());
  EXPECT_TRUE(js_.GetValue(id, "b", &value));
  EXPECT_EQ(12, value);
  EXPECT_FALSE(log_checker_.HasAnyMessages());
}

TEST_F(JsEngineTest, Converter) {
  js_.RegisterFunction("FlipBool", [](bool x) { return !x; });
  js_.RegisterFunction("AddEnum", [](EnumTest x) -> EnumTest {
    return static_cast<EnumTest>(x + 1);
  });
  js_.RegisterFunction("AddInt16", [](int16_t x) -> int16_t { return x + 1; });
  js_.RegisterFunction("AddInt32", [](int32_t x) -> int32_t { return x + 1; });
  js_.RegisterFunction("AddInt64", [](int64_t x) -> int64_t { return x + 1; });
  js_.RegisterFunction("AddUint16",
                       [](uint16_t x) -> uint16_t { return x + 1; });
  js_.RegisterFunction("AddUint32",
                       [](uint32_t x) -> uint32_t { return x + 1; });
  js_.RegisterFunction("AddUint64",
                       [](uint64_t x) -> uint64_t { return x + 1; });
  js_.RegisterFunction("AddFloat", [](float x) -> float { return x + 1; });
  js_.RegisterFunction("AddDouble", [](double x) -> double { return x + 1; });
  js_.RegisterFunction("AddDuration", [](Clock::duration x) -> Clock::duration {
    return x + DurationFromSeconds(1);
  });
  js_.RegisterFunction("AddEntity", [](lull::Entity x) -> lull::Entity {
    return x.AsUint32() + 1;
  });
  js_.RegisterFunction("RepeatString",
                       [](std::string s) -> std::string { return s + s; });
  js_.RegisterFunction("RotateVec2",
                       [](mathfu::vec2 v) { return mathfu::vec2(v.y, v.x); });
  js_.RegisterFunction(
      "RotateVec3", [](mathfu::vec3 v) { return mathfu::vec3(v.y, v.z, v.x); });
  js_.RegisterFunction("RotateVec4", [](mathfu::vec4 v) {
    return mathfu::vec4(v.y, v.z, v.w, v.x);
  });
  js_.RegisterFunction("RotateQuat", [](mathfu::quat v) {
    return mathfu::quat(v.vector().x, v.vector().y, v.vector().z, v.scalar());
  });
  js_.RegisterFunction("ReverseVector", [](const std::vector<int>& v) {
    std::vector<int> vv(v);
    for (size_t i = 0; i < vv.size() / 2; ++i) {
      std::swap(vv[i], vv[vv.size() - 1 - i]);
    }
    return vv;
  });
  js_.RegisterFunction("ReverseInt64Vector", [](const std::vector<int64_t>& v) {
    std::vector<int64_t> vv(v);
    for (size_t i = 0; i < vv.size() / 2; ++i) {
      std::swap(vv[i], vv[vv.size() - 1 - i]);
    }
    return vv;
  });
  js_.RegisterFunction("ReverseBoolVector", [](const std::vector<bool>& v) {
    std::vector<bool> vv(v);
    for (size_t i = 0; i < vv.size() / 2; ++i) {
      std::swap(vv[i], vv[vv.size() - 1 - i]);
    }
    return vv;
  });
  js_.RegisterFunction("ReverseStringVector",
                       [](const std::vector<std::string>& v) {
                         std::vector<std::string> vv(v);
                         for (size_t i = 0; i < vv.size() / 2; ++i) {
                           std::swap(vv[i], vv[vv.size() - 1 - i]);
                         }
                         return vv;
                       });
  js_.RegisterFunction("FlipVec2Vector",
                       [](const std::vector<mathfu::vec2>& v) {
                         std::vector<mathfu::vec2> vv(v);
                         for (auto& a : vv) {
                           float x = a.x;
                           a.x = a.y;
                           a.y = x;
                         }
                         return vv;
                       });
  js_.RegisterFunction("InvertMap", [](const std::map<std::string, int>& m) {
    std::map<int, std::string> n;
    for (const auto& kv : m) {
      n[kv.second] = kv.first;
    }
    return n;
  });
  js_.RegisterFunction("VectorizeStrings",
                       [](const std::unordered_map<int, std::string>& m) {
                         std::unordered_map<int, std::vector<std::string>> n;
                         for (const auto& kv : m) {
                           for (char c : kv.second) {
                             n[kv.first].emplace_back(1, c);
                           }
                         }
                         return n;
                       });
  js_.RegisterFunction("TransposeMatrix",
                       [](const mathfu::mat4& m) { return m.Transpose(); });
  js_.RegisterFunction("FlipAabb", [](const lull::Aabb& aabb) {
    return lull::Aabb(aabb.max, aabb.min);
  });
  js_.RegisterFunction("FlipRect", [](const mathfu::rectf& rect) {
    return mathfu::rectf(rect.size, rect.pos);
  });
  js_.RegisterFunction("DoubleOptional",
                       [](const lull::Optional<float>& optional) {
    if (optional) {
      return lull::Optional<float>(*optional * 2.f);
    } else {
      return lull::Optional<float>();
    }
  });
  uint64_t id = js_.LoadScript(
      R"(
        bool = FlipBool(true);
        enum_test = AddEnum(1);
        int16 = AddInt16(123);
        int32 = AddInt32(123);
        int64 = AddInt64(123);
        uint16 = AddUint16(123);
        uint32 = AddUint32(123);
        uint64 = AddUint64(123);
        float = AddFloat(123);
        double = AddDouble(123);
        duration = AddDuration(durationFromMilliseconds(234));
        entity = AddEntity(123);
        str = RepeatString("abc");
        vec2 = RotateVec2({x:1, y:2});
        vec3 = RotateVec3({x:1, y:2, z:3});
        vec4 = RotateVec4({x:1, y:2, z:3, w:4});
        quat = RotateQuat({x:1, y:2, z:3, s:4});
        vector = ReverseVector(new Int32Array([1, 2, 3, 4, 5]));
        vector64 = ReverseInt64Vector([1, 2, 3, 4, 5]);
        array = ReverseVector([1, 2, 3, 4, 5]);
        bool_array = ReverseBoolVector(new Uint8Array([true, false]));
        str_array = ReverseStringVector(["a", "b", "c"]);
        vec2v = FlipVec2Vector([{x:1, y:2}, {x:3, y:4}, {x:5, y:6}]);
        map = InvertMap({a:1, b:2, c:3, d:4});
        umap = VectorizeStrings({1:'abc', 2:'defg'});
        mat4 = TransposeMatrix({
          c0: {x:1, y:2, z:3, w:4},
          c1: {x:5, y:6, z:7, w:8},
          c2: {x:9, y:10, z:11, w:12},
          c3: {x:13, y:14, z:15, w:16},
        });
        aabb = FlipAabb({min:{x:1, y:2, z:3}, max:{x:4, y:5, z:6}});
        rect = FlipRect({pos:{x:1, y:2}, size:{x:3, y:4}});
        optional = DoubleOptional(4);
        optional_null = DoubleOptional(null);
      )",
      "ConverterScript");
  js_.RunScript(id);

  bool bool_value;
  EXPECT_TRUE(js_.GetValue(id, "bool", &bool_value));
  EXPECT_EQ(false, bool_value);
  EnumTest enum_value;
  EXPECT_TRUE(js_.GetValue(id, "enum_test", &enum_value));
  EXPECT_EQ(SECOND, enum_value);
  int16_t int16_value;
  EXPECT_TRUE(js_.GetValue(id, "int16", &int16_value));
  EXPECT_EQ(124, int16_value);
  int32_t int32_value;
  EXPECT_TRUE(js_.GetValue(id, "int32", &int32_value));
  EXPECT_EQ(124, int32_value);
  int64_t int64_value;
  EXPECT_TRUE(js_.GetValue(id, "int64", &int64_value));
  EXPECT_EQ(124, int64_value);
  uint16_t uint16_value;
  EXPECT_TRUE(js_.GetValue(id, "uint16", &uint16_value));
  EXPECT_EQ(124u, uint16_value);
  uint32_t uint32_value;
  EXPECT_TRUE(js_.GetValue(id, "uint32", &uint32_value));
  EXPECT_EQ(124u, uint32_value);
  uint64_t uint64_value;
  EXPECT_TRUE(js_.GetValue(id, "uint64", &uint64_value));
  EXPECT_EQ(124u, uint64_value);
  float float_value;
  EXPECT_TRUE(js_.GetValue(id, "float", &float_value));
  EXPECT_EQ(124.0f, float_value);
  double double_value;
  EXPECT_TRUE(js_.GetValue(id, "double", &double_value));
  EXPECT_EQ(124.0, double_value);
  Clock::duration duration_value;
  EXPECT_TRUE(js_.GetValue(id, "duration", &duration_value));
  EXPECT_EQ(duration_value, DurationFromMilliseconds(1234));
  lull::Entity entity_value;
  EXPECT_TRUE(js_.GetValue(id, "entity", &entity_value));
  EXPECT_EQ(entity_value, 124u);
  std::string str_value;
  EXPECT_TRUE(js_.GetValue(id, "str", &str_value));
  EXPECT_EQ("abcabc", str_value);

  mathfu::vec2 vec2_value;
  EXPECT_TRUE(js_.GetValue(id, "vec2", &vec2_value));
  EXPECT_EQ(mathfu::vec2(2, 1), vec2_value);
  mathfu::vec3 vec3_value;
  EXPECT_TRUE(js_.GetValue(id, "vec3", &vec3_value));
  EXPECT_EQ(mathfu::vec3(2, 3, 1), vec3_value);
  mathfu::vec4 vec4_value;
  EXPECT_TRUE(js_.GetValue(id, "vec4", &vec4_value));
  EXPECT_EQ(mathfu::vec4(2, 3, 4, 1), vec4_value);

  mathfu::quat quat;
  EXPECT_TRUE(js_.GetValue(id, "quat", &quat));
  EXPECT_EQ(mathfu::vec3(2, 3, 4), quat.vector());
  EXPECT_EQ(1, quat.scalar());

  std::vector<int> vect;
  EXPECT_TRUE(js_.GetValue(id, "vector", &vect));
  EXPECT_EQ(5, vect[0]);
  EXPECT_EQ(4, vect[1]);
  EXPECT_EQ(3, vect[2]);
  EXPECT_EQ(2, vect[3]);
  EXPECT_EQ(1, vect[4]);

  std::vector<int64_t> vect64;
  EXPECT_TRUE(js_.GetValue(id, "vector64", &vect64));
  EXPECT_EQ(5, vect64[0]);
  EXPECT_EQ(4, vect64[1]);
  EXPECT_EQ(3, vect64[2]);
  EXPECT_EQ(2, vect64[3]);
  EXPECT_EQ(1, vect64[4]);

  std::vector<int> arr;
  EXPECT_TRUE(js_.GetValue(id, "array", &arr));
  EXPECT_EQ(5, arr[0]);
  EXPECT_EQ(4, arr[1]);
  EXPECT_EQ(3, arr[2]);
  EXPECT_EQ(2, arr[3]);
  EXPECT_EQ(1, arr[4]);

  std::vector<bool> bool_arr;
  EXPECT_TRUE(js_.GetValue(id, "bool_array", &bool_arr));
  EXPECT_FALSE(bool_arr[0]);
  EXPECT_TRUE(bool_arr[1]);

  std::vector<std::string> str_array;
  EXPECT_TRUE(js_.GetValue(id, "str_array", &str_array));
  EXPECT_EQ("c", str_array[0]);
  EXPECT_EQ("b", str_array[1]);
  EXPECT_EQ("a", str_array[2]);

  std::vector<mathfu::vec2> vec2v;
  EXPECT_TRUE(js_.GetValue(id, "vec2v", &vec2v));
  EXPECT_EQ(mathfu::vec2(2, 1), vec2v[0]);
  EXPECT_EQ(mathfu::vec2(4, 3), vec2v[1]);
  EXPECT_EQ(mathfu::vec2(6, 5), vec2v[2]);

  std::map<int, std::string> m;
  EXPECT_TRUE(js_.GetValue(id, "map", &m));
  EXPECT_EQ(4u, m.size());
  EXPECT_EQ("b", m[2]);
  EXPECT_EQ("c", m[3]);
  EXPECT_EQ("d", m[4]);

  std::unordered_map<int, std::vector<std::string>> um;
  EXPECT_TRUE(js_.GetValue(id, "umap", &um));
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

  mathfu::mat4 mat;
  EXPECT_TRUE(js_.GetValue(id, "mat4", &mat));
  EXPECT_EQ(mathfu::vec4(1, 5, 9, 13), mat.GetColumn(0));
  EXPECT_EQ(mathfu::vec4(2, 6, 10, 14), mat.GetColumn(1));
  EXPECT_EQ(mathfu::vec4(3, 7, 11, 15), mat.GetColumn(2));
  EXPECT_EQ(mathfu::vec4(4, 8, 12, 16), mat.GetColumn(3));

  lull::Aabb aabb;
  EXPECT_TRUE(js_.GetValue(id, "aabb", &aabb));
  EXPECT_EQ(mathfu::vec3(4, 5, 6), aabb.min);
  EXPECT_EQ(mathfu::vec3(1, 2, 3), aabb.max);

  mathfu::rectf rect;
  EXPECT_TRUE(js_.GetValue(id, "rect", &rect));
  EXPECT_EQ(mathfu::vec2(3, 4), rect.pos);
  EXPECT_EQ(mathfu::vec2(1, 2), rect.size);

  lull::Optional<float> optional;
  EXPECT_TRUE(js_.GetValue(id, "optional", &optional));
  EXPECT_EQ(true, static_cast<bool>(optional));
  EXPECT_EQ(8.f, *optional);

  lull::Optional<float> optional_null;
  EXPECT_TRUE(js_.GetValue(id, "optional_null", &optional_null));
  EXPECT_EQ(false, static_cast<bool>(optional_null));

  EXPECT_FALSE(log_checker_.HasAnyMessages());
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

TEST_F(JsEngineTest, Serializables) {
  js_.RegisterFunction("ConvertFooToBar", [](const Foo& foo) {
    return Bar(foo.name, foo.x + foo.y);
  });
  js_.RegisterFunction(
      "ConvertFooBarToBaz",
      [](const Foo& foo, const Bar& bar) { return Baz(foo, bar); });

  uint64_t id = js_.LoadScript(
      R"(
        bar1 = ConvertFooToBar(foo1);
        bar2 = ConvertFooToBar({name: "js foo", x: -8, y: 4});
        bar3 = {name: "bar from " + bar1.name, z: bar1.z + 10};
        bar1.z = 35;
        baz1 = ConvertFooBarToBaz(foo1, bar3);
      )",
      "ConverterScript");
  js_.SetValue(id, "foo1", Foo("native foo", 5, 3));
  js_.RunScript(id);

  Bar bar1, bar2, bar3;
  EXPECT_TRUE(js_.GetValue(id, "bar1", &bar1));
  EXPECT_EQ("native foo", bar1.name);
  EXPECT_EQ(35, bar1.z);

  EXPECT_TRUE(js_.GetValue(id, "bar2", &bar2));
  EXPECT_EQ("js foo", bar2.name);
  EXPECT_EQ(-4, bar2.z);

  EXPECT_TRUE(js_.GetValue(id, "bar3", &bar3));
  EXPECT_EQ("bar from native foo", bar3.name);
  EXPECT_EQ(18, bar3.z);

  Baz baz1;
  EXPECT_TRUE(js_.GetValue(id, "baz1", &baz1));
  EXPECT_EQ("native foo", baz1.foo.name);
  EXPECT_EQ(5, baz1.foo.x);
  EXPECT_EQ(3, baz1.foo.y);
  EXPECT_EQ("bar from native foo", baz1.bar.name);
  EXPECT_EQ(18, baz1.bar.z);

  EXPECT_FALSE(log_checker_.HasAnyMessages());
}

TEST_F(JsEngineTest, Duration) {
  uint64_t id = js_.LoadScript(
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

  js_.SetValue(id, "ms1", DurationFromMilliseconds(1000));
  js_.SetValue(id, "ms05", DurationFromMilliseconds(500));
  js_.SetValue(id, "sec1", DurationFromSeconds(1));
  js_.SetValue(id, "sec05", DurationFromSeconds(0.5f));

  js_.RunScript(id);

  bool eq0, eq1, eq2, eq3, eq4, eq5, eq6;
  EXPECT_TRUE(js_.GetValue(id, "eq0", &eq0));
  EXPECT_TRUE(js_.GetValue(id, "eq1", &eq1));
  EXPECT_TRUE(js_.GetValue(id, "eq2", &eq2));
  EXPECT_TRUE(js_.GetValue(id, "eq3", &eq3));
  EXPECT_TRUE(js_.GetValue(id, "eq4", &eq4));
  EXPECT_TRUE(js_.GetValue(id, "eq5", &eq5));
  EXPECT_TRUE(js_.GetValue(id, "eq6", &eq6));
  EXPECT_FALSE(eq0);
  EXPECT_TRUE(eq1);
  EXPECT_TRUE(eq2);
  EXPECT_TRUE(eq3);
  EXPECT_TRUE(eq4);
  EXPECT_TRUE(eq5);
  EXPECT_TRUE(eq6);

  EXPECT_FALSE(log_checker_.HasAnyMessages());
}

TEST_F(JsEngineTest, HashValue) {
  uint64_t id = js_.LoadScript(
      R"(
         y = hash("x");
      )",
      "HashScript");
  js_.RunScript(id);

  HashValue y;
  ASSERT_TRUE(js_.GetValue(id, "y", &y));
  EXPECT_EQ(y, Hash("x"));

  EXPECT_FALSE(log_checker_.HasAnyMessages());
}

TEST_F(JsEngineTest, EventWrappers) {
  js_.RegisterFunction("EventWrapperFooToBar", [](const EventWrapper& foo) {
    Bar b(*foo.GetValue<std::string>(Hash("name")),
          *foo.GetValue<int>(Hash("x")) + *foo.GetValue<int>(Hash("y")));
    EventWrapper e(b), c(e);  // Because reasons.
    return c;
  });

  uint64_t id = js_.LoadScript(
      R"(
        bar1 = EventWrapperFooToBar(foo1);
        bar2 = EventWrapperFooToBar({
          type: hash("lull::script::javascript::Foo"),
          data: {
            name: {type: hash("std::string"), data: "js foo"},
            x: {type: hash("int32_t"), data: -8},
            y: {type: hash("int32_t"), data: 4}
          }
        });
        bar3 = {
          type: hash("lull::script::javascript::Bar"),
          data: {
            name: {type: hash("std::string"), data: bar1.data.name.data},
            z: {type: hash("int32_t"), data: bar1.data.z.data + 10}
          }
        };
        bar1.data.z.data = 35;
      )",
      "ConverterScript");

  Foo foo1("native foo", 5, 3);
  js_.SetValue(id, "foo1", EventWrapper(foo1));

  js_.RunScript(id);

  EventWrapper bar1, bar2, bar3;
  ASSERT_TRUE(js_.GetValue(id, "bar1", &bar1));
  ASSERT_NE(nullptr, bar1.GetValue<std::string>(Hash("name")));
  EXPECT_EQ("native foo", *bar1.GetValue<std::string>(Hash("name")));
  ASSERT_NE(nullptr, bar1.GetValue<int>(Hash("z")));
  EXPECT_EQ(35, *bar1.GetValue<int>(Hash("z")));

  ASSERT_TRUE(js_.GetValue(id, "bar2", &bar2));
  ASSERT_NE(nullptr, bar2.GetValue<std::string>(Hash("name")));
  EXPECT_EQ("js foo", *bar2.GetValue<std::string>(Hash("name")));
  ASSERT_NE(nullptr, bar2.GetValue<int>(Hash("z")));
  EXPECT_EQ(-4, *bar2.GetValue<int>(Hash("z")));

  ASSERT_TRUE(js_.GetValue(id, "bar3", &bar3));
  ASSERT_NE(nullptr, bar3.GetValue<std::string>(Hash("name")));
  EXPECT_EQ("native foo", *bar3.GetValue<std::string>(Hash("name")));
  ASSERT_NE(nullptr, bar3.GetValue<int>(Hash("z")));
  EXPECT_EQ(18, *bar3.GetValue<int>(Hash("z")));

  EXPECT_FALSE(log_checker_.HasAnyMessages());
}

TEST_F(JsEngineTest, Environment) {
  uint64_t id = js_.LoadScript("x = x + 1", "EnvironmentScript");
  js_.SetValue(id, "x", 10);
  js_.RunScript(id);
  int value;
  EXPECT_TRUE(js_.GetValue(id, "x", &value));
  EXPECT_EQ(11, value);
  js_.RunScript(id);
  EXPECT_TRUE(js_.GetValue(id, "x", &value));
  EXPECT_EQ(12, value);
  js_.RunScript(id);
  EXPECT_TRUE(js_.GetValue(id, "x", &value));
  EXPECT_EQ(13, value);
  js_.RunScript(id);
  EXPECT_TRUE(js_.GetValue(id, "x", &value));
  EXPECT_EQ(14, value);
  uint64_t id2 = js_.LoadScript("x = x + 1", "EnvironmentScript");
  js_.SetValue(id2, "x", 3);
  js_.RunScript(id2);
  EXPECT_TRUE(js_.GetValue(id2, "x", &value));
  EXPECT_EQ(4, value);
  js_.RunScript(id2);
  js_.RunScript(id);
  EXPECT_TRUE(js_.GetValue(id, "x", &value));
  EXPECT_EQ(15, value);
  EXPECT_FALSE(log_checker_.HasAnyMessages());
}

TEST_F(JsEngineTest, Reload) {
  uint64_t id = js_.LoadScript("x = 5", "ReloadScript");
  js_.RunScript(id);
  int value;
  EXPECT_TRUE(js_.GetValue(id, "x", &value));
  EXPECT_EQ(5, value);
  js_.ReloadScript(id, "y = x * 2");
  js_.RunScript(id);
  EXPECT_TRUE(js_.GetValue(id, "y", &value));
  EXPECT_EQ(10, value);
  EXPECT_FALSE(log_checker_.HasAnyMessages());
}

TEST_F(JsEngineTest, IncludeFunction) {
  js_.SetLoadFileFunction([](const char* filename, std::string* out) {
    EXPECT_EQ(filename, std::string("thing.js"));
    *out = R"(
          thing = 0;

          function GetThing() {
            return thing;
          }

          function SetThing(t) {
            thing = t;
          }
        )";
    return true;
  });

  uint64_t id1 = js_.LoadScript(
      R"(
        thing = include('thing.js');
        thing.SetThing(1234);
      )",
      "ImportScript1");
  uint64_t id2 = js_.LoadScript(
      R"(
        thing = include('thing.js');
        t = thing.GetThing();
      )",
      "ImportScript2");

  js_.RunScript(id1);
  js_.RunScript(id2);

  int t = -1;
  EXPECT_TRUE(js_.GetValue(id2, "t", &t));
  EXPECT_EQ(1234, t);
}

TEST_F(JsEngineTest, UnloadScript) {
  uint64_t id = js_.LoadScript("y = (3 + x) * 2 + 1;", "SimpleScript");
  js_.SetValue(id, "x", 10);
  js_.RunScript(id);
  int value;
  EXPECT_TRUE(js_.GetValue(id, "y", &value));
  EXPECT_EQ(27, value);

  size_t total_scripts = js_.GetTotalScripts();
  js_.UnloadScript(id);
  js_.RunScript(id);

  EXPECT_TRUE(log_checker_.HasMessage("ERROR", "Script not found"));
  EXPECT_EQ(total_scripts - 1, js_.GetTotalScripts());
}

TEST_F(JsEngineTest, ConsoleLogFunction) {
  uint64_t id =
      js_.LoadScript("console.log('abc', 123, true, 'xyz');", "LogScript");
  js_.RunScript(id);
  EXPECT_TRUE(log_checker_.HasMessage("INFO", "abc 123 true xyz"));
}

TEST_F(JsEngineTest, GetValueError) {
  uint64_t id = js_.LoadScript("", "GetValueErrorScript");
  int intx;
  float floatx;
  std::vector<std::string> vectorx;
  js_.SetValue(id, "x", 123);
  EXPECT_TRUE(js_.GetValue(id, "x", &intx));
  EXPECT_EQ(123, intx);
  EXPECT_TRUE(js_.GetValue(id, "x", &floatx));
  EXPECT_EQ(123, floatx);
  EXPECT_FALSE(js_.GetValue(id, "x", &vectorx));
  EXPECT_FALSE(js_.GetValue(id, "y", &intx));
  EXPECT_FALSE(log_checker_.HasAnyMessages());
}

TEST_F(JsEngineTest, RecursiveScripts) {
  uint64_t id1 = js_.LoadScript("x = 5", "InnerScript");
  js_.RunScript(id1);
  js_.RegisterFunction("GetX", [this, id1] () {
    int x = -1;
    EXPECT_TRUE(js_.GetValue(id1, "x", &x));
    return x;
  });
  uint64_t id2 = js_.LoadScript("y = GetX()", "OuterScript");
  js_.RunScript(id2);
  int y = -1;
  EXPECT_TRUE(js_.GetValue(id2, "y", &y));
  EXPECT_EQ(5, y);
  EXPECT_FALSE(log_checker_.HasAnyMessages());
}

TEST_F(JsEngineTest, ConvertError) {
  int y = 4;
  js_.RegisterFunction("FooInt", [y](int x) {});
  js_.RegisterFunction("FooStrings", [](std::string x, std::string y) {});
  js_.RegisterFunction("FooVecVecBool",
                       [](const std::vector<std::vector<bool>>& x) {});
  uint64_t id = js_.LoadScript("FooInt('abc')", "FooIntScript");

  js_.RunScript(id);
  EXPECT_TRUE(log_checker_.HasMessage(
      "ERROR", "FooInt expects the type of arg 0 to be number"));

  id = js_.LoadScript("FooStrings('abc')", "FooStringsScript");
  js_.RunScript(id);
  EXPECT_TRUE(
      log_checker_.HasMessage("ERROR", "FooStrings expects 2 args, but got 1"));

  id = js_.LoadScript("FooStrings('abc', {})", "FooStringsScript2");
  js_.RunScript(id);
  EXPECT_TRUE(log_checker_.HasMessage(
      "ERROR", "FooStrings expects the type of arg 1 to be string"));

  id = js_.LoadScript("FooVecVecBool('abc')", "FooVecVecBoolScript");
  js_.RunScript(id);
  EXPECT_TRUE(log_checker_.HasMessage("ERROR",
                                      "FooVecVecBool expects the type of arg 0 "
                                      "to be array of array of boolean"));

  id = js_.LoadScript("FooVecVecBool([[false, true], [true], []])",
                      "FooVecVecBoolScript");
  js_.RunScript(id);
  EXPECT_FALSE(log_checker_.HasAnyMessages());
}

TEST_F(JsEngineTest, SyntaxError) {
  js_.LoadScript("syntax error", "SyntaxErrorScript");
  EXPECT_TRUE(log_checker_.HasMessage(
      "ERROR", "SyntaxErrorScript: SyntaxError: Unexpected identifier"));
}

TEST_F(JsEngineTest, RuntimeError) {
  uint64_t id = js_.LoadScript("not_a_func()", "RuntimeErrorScript");
  js_.SetValue(id, "not_a_func", 123);
  js_.RunScript(id);
  EXPECT_TRUE(
      log_checker_.HasMessage("ERROR",
                              "RuntimeErrorScript: "
                              "TypeError: not_a_func is not a function"));
}

TEST_F(JsEngineTest, ThrownError) {
  uint64_t id = js_.LoadScript("throw 'oh no!'", "ThrownErrorScript");
  js_.RunScript(id);
  EXPECT_TRUE(log_checker_.HasMessage("ERROR", "ThrownErrorScript: oh no!"));
}

TEST_F(JsEngineTest, UnregisteredFunctionError) {
  js_.RegisterFunction("func1", []() {});
  uint64_t id = js_.LoadScript("func1()", "UnregisteredFunctionErrorScript");
  js_.UnregisterFunction("func1");
  js_.RunScript(id);
  EXPECT_TRUE(log_checker_.HasMessage(
      "ERROR",
      "UnregisteredFunctionErrorScript: ReferenceError: func1 is not defined"));
}

TEST_F(JsEngineTest, ReloadError) {
  uint64_t id = js_.LoadScript("x = 5", "ReloadErrorScript");
  EXPECT_FALSE(log_checker_.HasAnyMessages());
  js_.ReloadScript(id, "syntax error");
  EXPECT_TRUE(log_checker_.HasMessage(
      "ERROR", "ReloadErrorScript: SyntaxError: Unexpected identifier"));
}

}  // namespace javascript
}  // namespace script
}  // namespace lull

LULLABY_SETUP_TYPEID(lull::script::javascript::Foo);
LULLABY_SETUP_TYPEID(lull::script::javascript::Bar);
LULLABY_SETUP_TYPEID(lull::script::javascript::Baz);
