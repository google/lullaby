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

#include "lullaby/modules/jni/jni_convert.h"

#include <jni.h>

#include "lullaby/util/entity.h"

namespace lull {
namespace {

// Reminder: Java templates use type erasure, so signatures for all generic
// return values or arguments should use the jni type jobject, representing
// java.lang.Object.

// Convert to native types

bool ConvertToNativeBool(JniContext* ctx, jobject jobj) {
  return static_cast<bool>(ctx->CallJniMethod<jboolean>(jobj, "booleanValue"));
}

int32_t ConvertToNativeInt(JniContext* ctx, jobject jobj) {
  return static_cast<int32_t>(ctx->CallJniMethod<jint>(jobj, "intValue"));
}

int64_t ConvertToNativeLong(JniContext* ctx, jobject jobj) {
  return static_cast<int64_t>(ctx->CallJniMethod<jlong>(jobj, "longValue"));
}

float ConvertToNativeFloat(JniContext* ctx, jobject jobj) {
  return static_cast<float>(ctx->CallJniMethod<jfloat>(jobj, "floatValue"));
}

double ConvertToNativeDouble(JniContext* ctx, jobject jobj) {
  return static_cast<double>(ctx->CallJniMethod<jdouble>(jobj, "doubleValue"));
}

std::string ConvertToNativeString(JniContext* ctx, jstring jstr) {
  std::string out;
  JNIEnv* env = ctx->GetJniEnv();
  const char* data = env->GetStringUTFChars(jstr, nullptr);
  if (data) {
    const size_t size = env->GetStringUTFLength(jstr);
    out.assign(data, size);
    env->ReleaseStringUTFChars(jstr, data);
  }
  return out;
}

mathfu::vec2 ConvertToNativeVec2(JniContext* ctx, jobject jobj) {
  mathfu::vec2 out;
  out.x = static_cast<float>(ctx->CallJniMethod<jfloat>(jobj, "getX"));
  out.y = static_cast<float>(ctx->CallJniMethod<jfloat>(jobj, "getY"));
  return out;
}

mathfu::vec3 ConvertToNativeVec3(JniContext* ctx, jobject jobj) {
  mathfu::vec3 out;
  out.x = static_cast<float>(ctx->CallJniMethod<jfloat>(jobj, "getX"));
  out.y = static_cast<float>(ctx->CallJniMethod<jfloat>(jobj, "getY"));
  out.z = static_cast<float>(ctx->CallJniMethod<jfloat>(jobj, "getZ"));
  return out;
}

mathfu::vec4 ConvertToNativeVec4(JniContext* ctx, jobject jobj) {
  mathfu::vec4 out;
  out.x = static_cast<float>(ctx->CallJniMethod<jfloat>(jobj, "getX"));
  out.y = static_cast<float>(ctx->CallJniMethod<jfloat>(jobj, "getY"));
  out.z = static_cast<float>(ctx->CallJniMethod<jfloat>(jobj, "getZ"));
  out.w = static_cast<float>(ctx->CallJniMethod<jfloat>(jobj, "getW"));
  return out;
}

mathfu::quat ConvertToNativeQuat(JniContext* ctx, jobject jobj) {
  // Note: vecmath.Quat4f is (x, y, z, s), whereas mathfu::quat is (s, x, y, z).
  mathfu::vec3 out;
  out.x = static_cast<float>(ctx->CallJniMethod<jfloat>(jobj, "getX"));
  out.y = static_cast<float>(ctx->CallJniMethod<jfloat>(jobj, "getY"));
  out.z = static_cast<float>(ctx->CallJniMethod<jfloat>(jobj, "getZ"));
  float s = static_cast<float>(ctx->CallJniMethod<jfloat>(jobj, "getW"));
  return mathfu::quat(s, out);
}

mathfu::mat4 ConvertToNativeMat4(JniContext* ctx, jobject jobj) {
  mathfu::mat4 out;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      out(i, j) = static_cast<float>(ctx->CallJniMethod<jfloat>(
          jobj, "getElement", static_cast<jint>(i), static_cast<jint>(j)));
    }
  }
  return out;
}

VariantArray ConvertToNativeArray(JniContext* ctx,
                                  const JavaUtilArrayList& jarray) {
  VariantArray array;
  const jint size = ctx->CallJniMethod<jint>(jarray, "size");
  for (jint i = 0; i < size; ++i) {
    auto jobj = ctx->CallJniMethod<jobject>(jarray, "get", i);
    array.emplace_back(ConvertToNativeObject(ctx, jobj));
  }
  return array;
}

VariantMap ConvertToNativeMap(JniContext* ctx, const JavaUtilHashMap& jmap) {
  VariantMap map;
  auto jset = ctx->CallJniMethod<JavaUtilSet>(jmap, "entrySet");
  auto jiter = ctx->CallJniMethod<JavaUtilIterator>(jset, "iterator");
  while (ctx->CallJniMethod<jboolean>(jiter, "hasNext")) {
    auto jentry = ctx->CallJniMethod<jobject>(jiter, "next");
    auto jkey = ctx->CallJniMethod<jobject>(jentry, "getKey");
    auto jvalue = ctx->CallJniMethod<jobject>(jentry, "getValue");
    auto key = static_cast<HashValue>(ConvertToNativeLong(ctx, jkey));
    auto value = ConvertToNativeObject(ctx, jvalue);
    map.emplace(key, std::move(value));
  }
  return map;
}

Entity ConvertToNativeEntity(JniContext* ctx, ComGoogleLullabyEntity jentity) {
  return Entity(
      static_cast<uint32_t>(ctx->CallJniMethod<jlong>(jentity, "getNativeId")));
}

EventWrapper ConvertToNativeEvent(JniContext* ctx,
                                  const ComGoogleLullabyEvent& jevent) {
  const auto jtype = ctx->CallJniMethod<jlong>(jevent, "getType");
  const auto jmap = ctx->CallJniMethod<JavaUtilHashMap>(jevent, "getValues");
  EventWrapper event_wrapper(static_cast<TypeId>(jtype));
  event_wrapper.SetValues(ConvertToNativeMap(ctx, jmap));
  return event_wrapper;
}

std::string GetClassName(JniContext* ctx, jobject jobj) {
  const jclass jcls = ctx->GetJniEnv()->GetObjectClass(jobj);
  if (!jcls) {
    return "";
  }
  const jstring jname = ctx->CallJniMethod<jstring>(jcls, "getName");
  if (!jname) {
    return "";
  }
  return ConvertToNativeString(ctx, jname);
}

// Convert to jni types

JavaLangBoolean ConvertToJniBool(JniContext* ctx, bool value) {
  return ctx->CallJniStaticMethod<JavaLangBoolean>(
      "java/lang/Boolean", "valueOf", static_cast<jboolean>(value));
}

JavaLangInteger ConvertToJniInt(JniContext* ctx, int32_t value) {
  return ctx->CallJniStaticMethod<JavaLangInteger>(
      "java/lang/Integer", "valueOf", static_cast<jint>(value));
}

JavaLangLong ConvertToJniLong(JniContext* ctx, int64_t value) {
  return ctx->CallJniStaticMethod<JavaLangLong>("java/lang/Long", "valueOf",
                                                static_cast<jlong>(value));
}

JavaLangFloat ConvertToJniFloat(JniContext* ctx, float value) {
  return ctx->CallJniStaticMethod<JavaLangFloat>("java/lang/Float", "valueOf",
                                                 static_cast<float>(value));
}

JavaLangDouble ConvertToJniDouble(JniContext* ctx, double value) {
  return ctx->CallJniStaticMethod<JavaLangDouble>("java/lang/Double", "valueOf",
                                                  static_cast<double>(value));
}

jstring ConvertToJniString(JniContext* ctx, const std::string& value) {
  return ctx->GetJniEnv()->NewStringUTF(value.c_str());
}

JavaxVecmathVector2f ConvertToJniVec2(JniContext* ctx,
                                      const mathfu::vec2& value) {
  return ctx->NewJniObject<JavaxVecmathVector2f>(
      "javax/vecmath/Vector2f", static_cast<jfloat>(value.x),
      static_cast<jfloat>(value.y));
}

JavaxVecmathVector3f ConvertToJniVec3(JniContext* ctx,
                                      const mathfu::vec3& value) {
  return ctx->NewJniObject<JavaxVecmathVector3f>(
      "javax/vecmath/Vector3f", static_cast<jfloat>(value.x),
      static_cast<jfloat>(value.y), static_cast<jfloat>(value.z));
}

JavaxVecmathVector4f ConvertToJniVec4(JniContext* ctx,
                                      const mathfu::vec4& value) {
  return ctx->NewJniObject<JavaxVecmathVector4f>(
      "javax/vecmath/Vector4f", static_cast<jfloat>(value.x),
      static_cast<jfloat>(value.y), static_cast<jfloat>(value.z),
      static_cast<jfloat>(value.w));
}

JavaxVecmathQuat4f ConvertToJniQuat(JniContext* ctx,
                                    const mathfu::quat& value) {
  // Note: vecmath.Quat4f is (x, y, z, s), whereas mathfu::quat is (s, x, y, z).
  return ctx->NewJniObject<JavaxVecmathQuat4f>(
      "javax/vecmath/Quat4f", static_cast<jfloat>(value.vector().x),
      static_cast<jfloat>(value.vector().y),
      static_cast<jfloat>(value.vector().z),
      static_cast<jfloat>(value.scalar()));
}

JavaxVecmathMatrix4f ConvertToJniMat4(JniContext* ctx,
                                      const mathfu::mat4& value) {
  auto jmat = ctx->NewJniObject<JavaxVecmathMatrix4f>("javax/vecmath/Matrix4f");
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      ctx->CallJniMethod<void>(jmat, "setElement", static_cast<jint>(i),
                               static_cast<jint>(j),
                               static_cast<jfloat>(value(i, j)));
    }
  }
  return jmat;
}

JavaUtilArrayList ConvertToJniArray(JniContext* ctx,
                                    const VariantArray& array) {
  auto jarray = ctx->NewJniObject<JavaUtilArrayList>("java/util/ArrayList");
  for (const auto& element : array) {
    ctx->CallJniMethod<jboolean>(jarray, "add",
                                 ConvertToJniObject(ctx, element));
  }
  return jarray;
}

JavaUtilHashMap ConvertToJniMap(JniContext* ctx, const VariantMap& map) {
  auto jmap = ctx->NewJniObject<JavaUtilHashMap>("java/util/HashMap");
  for (const auto& pair : map) {
    ctx->CallJniMethod<jobject>(
        jmap, "put",
        ConvertToJniLong(ctx, static_cast<int64_t>(pair.first)).jobj,
        ConvertToJniObject(ctx, pair.second));
  }
  return jmap;
}

ComGoogleLullabyEntity ConvertToJniEntity(JniContext* ctx, Entity value) {
  if (value != kNullEntity) {
    return ctx->CallJniStaticMethod<ComGoogleLullabyEntity>(
        "com/google/lullaby/Entity", "create", static_cast<jlong>(value));
  } else {
    return nullptr;
  }
}

}  // namespace

Variant ConvertToNativeObject(JniContext* ctx, jobject jobj) {
  Variant out;
  if (!jobj) {
    return out;
  }

  std::string class_name = GetClassName(ctx, jobj);
  if (class_name == "java.lang.Boolean") {
    out = ConvertToNativeBool(ctx, jobj);
  } else if (class_name == "java.lang.Integer") {
    out = ConvertToNativeInt(ctx, jobj);
  } else if (class_name == "java.lang.Long") {
    out = ConvertToNativeLong(ctx, jobj);
  } else if (class_name == "java.lang.Float") {
    out = ConvertToNativeFloat(ctx, jobj);
  } else if (class_name == "java.lang.Double") {
    out = ConvertToNativeDouble(ctx, jobj);
  } else if (class_name == "java.lang.String") {
    out = ConvertToNativeString(ctx, static_cast<jstring>(jobj));
  } else if (class_name == "javax.vecmath.Vector2f") {
    out = ConvertToNativeVec2(ctx, jobj);
  } else if (class_name == "javax.vecmath.Vector3f") {
    out = ConvertToNativeVec3(ctx, jobj);
  } else if (class_name == "javax.vecmath.Vector4f") {
    out = ConvertToNativeVec4(ctx, jobj);
  } else if (class_name == "javax.vecmath.Quat4f") {
    out = ConvertToNativeQuat(ctx, jobj);
  } else if (class_name == "javax.vecmath.Matrix4f") {
    out = ConvertToNativeMat4(ctx, jobj);
  } else if (class_name == "java.util.ArrayList") {
    out = ConvertToNativeArray(ctx, jobj);
  } else if (class_name == "java.util.HashMap") {
    out = ConvertToNativeMap(ctx, jobj);
  } else if (class_name == "com.google.lullaby.Entity") {
    out = ConvertToNativeEntity(ctx, jobj);
  } else if (class_name == "com.google.lullaby.Event") {
    out = ConvertToNativeEvent(ctx, jobj);
  } else if (!class_name.empty()) {
    LOG(WARNING) << "Unknown class name for jni: " << class_name;
  }
  // Null is a valid value, such as from an unset Optional.
  return out;
}

jobject ConvertToJniObject(JniContext* ctx, const Variant& value) {
  if (auto v = value.Get<bool>()) {
    return ConvertToJniBool(ctx, *v).jobj;
  }
  if (auto v = value.Get<int32_t>()) {
    return ConvertToJniInt(ctx, *v).jobj;
  }
  if (auto v = value.Get<int64_t>()) {
    return ConvertToJniLong(ctx, *v).jobj;
  }
  if (auto v = value.Get<float>()) {
    return ConvertToJniFloat(ctx, *v).jobj;
  }
  if (auto v = value.Get<double>()) {
    return ConvertToJniDouble(ctx, *v).jobj;
  }
  if (auto v = value.Get<std::string>()) {
    return static_cast<jobject>(ConvertToJniString(ctx, *v));
  }
  // Cast c unsigned ints and longs to java long.
  if (auto v = value.Get<uint32_t>()) {
    return ConvertToJniLong(ctx, static_cast<int64_t>(*v)).jobj;
  }
  if (auto v = value.Get<uint64_t>()) {
    return ConvertToJniLong(ctx, static_cast<int64_t>(*v)).jobj;
  }
  if (auto v = value.Get<mathfu::vec2>()) {
    return ConvertToJniVec2(ctx, *v).jobj;
  }
  if (auto v = value.Get<mathfu::vec3>()) {
    return ConvertToJniVec3(ctx, *v).jobj;
  }
  // This will also capture vec4i, recti, rectf.
  if (auto v = value.ImplicitCast<mathfu::vec4>()) {
    return ConvertToJniVec4(ctx, *v).jobj;
  }
  if (auto v = value.Get<mathfu::quat>()) {
    return ConvertToJniQuat(ctx, *v).jobj;
  }
  if (auto v = value.Get<mathfu::mat4>()) {
    return ConvertToJniMat4(ctx, *v).jobj;
  }
  if (auto v = value.Get<VariantArray>()) {
    return ConvertToJniArray(ctx, *v).jobj;
  }
  if (auto v = value.Get<VariantMap>()) {
    return ConvertToJniMap(ctx, *v).jobj;
  }
  if (auto v = value.Get<Entity>()) {
    return ConvertToJniEntity(ctx, *v).jobj;
  }
  if (auto v = value.Get<EventWrapper>()) {
    return ConvertToJniEvent(ctx, *v).jobj;
  }
  // This captures enums and shorter int types.
  if (auto v = value.ImplicitCast<int32_t>()) {
    return ConvertToJniInt(ctx, *v).jobj;
  }
  if (value.GetTypeId()) {
    LOG(WARNING) << "Unknown variant type for jni: " << value.GetTypeId();
  }
  // Null is a valid value, such as from an unset Optional.
  return nullptr;
}

ComGoogleLullabyEvent ConvertToJniEvent(JniContext* ctx,
                                        const EventWrapper& event_wrapper) {
  Variant v = event_wrapper;
  auto jmap = ConvertToJniMap(ctx, *event_wrapper.GetValues());
  return ctx->CallJniStaticMethod<ComGoogleLullabyEvent>(
      "com/google/lullaby/Event", "createWithData",
      static_cast<jlong>(event_wrapper.GetTypeId()), jmap);
}

}  // namespace lull
