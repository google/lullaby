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

#ifndef LULLABY_MODULES_JNI_JNI_UTIL_H_
#define LULLABY_MODULES_JNI_JNI_UTIL_H_

#include <jni.h>

#include "lullaby/modules/jni/jni_method.h"
#include "lullaby/modules/jni/scoped_java_exception_guard.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/string_view.h"

namespace lull {
namespace detail {

// Specializations of this class are used to help call JNI functions.
template <typename Type>
struct JniHelper;

// Specializations of JniHelper for common types.
#define LULLABY_JNI_HELPER(NativeType_, JavaType_)                             \
  template <>                                                                  \
  struct JniHelper<NativeType_> {                                              \
    template <typename... Args>                                                \
    static NativeType_ CallMethod(JNIEnv* env, jobject obj, jmethodID id,      \
                                  Args&&... args) {                            \
      ScopedJavaExceptionGuard guard(env);                                     \
      return (NativeType_)env->Call##JavaType_##Method(                        \
          obj, id, std::forward<Args>(args)...);                               \
    }                                                                          \
                                                                               \
    template <typename... Args>                                                \
    static NativeType_ CallStaticMethod(JNIEnv* env, jclass cls, jmethodID id, \
                                        Args&&... args) {                      \
      ScopedJavaExceptionGuard guard(env);                                     \
      return (NativeType_)env->CallStatic##JavaType_##Method(                  \
          cls, id, std::forward<Args>(args)...);                               \
    }                                                                          \
                                                                               \
    static NativeType_ GetFieldValue(JNIEnv* env, jobject obj, jfieldID id) {  \
      ScopedJavaExceptionGuard guard(env);                                     \
      return (NativeType_)env->Get##JavaType_##Field(obj, id);                 \
    }                                                                          \
  };

LULLABY_JNI_HELPER(jboolean, Boolean);
LULLABY_JNI_HELPER(jint, Int);
LULLABY_JNI_HELPER(jlong, Long);
LULLABY_JNI_HELPER(jfloat, Float);
LULLABY_JNI_HELPER(jdouble, Double);
LULLABY_JNI_HELPER(jbooleanArray, Object);
LULLABY_JNI_HELPER(jintArray, Object);
LULLABY_JNI_HELPER(jlongArray, Object);
LULLABY_JNI_HELPER(jfloatArray, Object);
LULLABY_JNI_HELPER(jdoubleArray, Object);
LULLABY_JNI_HELPER(jstring, Object);
LULLABY_JNI_HELPER(jclass, Object);
LULLABY_JNI_HELPER(jobject, Object);
LULLABY_JNI_HELPER(jobjectArray, Object);
#undef LULLABY_JNI_HELPER

// An explicit specialization of JavaTypeHelper for void.  We can't use the
// macro specialization because there's no such thing as GetVoidField.
template <>
struct JniHelper<void> {
  template <typename... Args>
  static void CallMethod(JNIEnv* env, jobject obj, jmethodID id,
                         Args&&... args) {
    ScopedJavaExceptionGuard guard(env);
    env->CallVoidMethod(obj, id, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void CallStaticMethod(JNIEnv* env, jclass cls, jmethodID id,
                               Args&&... args) {
    ScopedJavaExceptionGuard guard(env);
    env->CallStaticVoidMethod(cls, id, std::forward<Args>(args)...);
  }
};

}  // namespace detail

// In the following functions, all |Type|, |ReturnType|, |ObjectType|, or |Args|
// should be a JNI type such as jint, or one of the custom structs defined in
// jni_signature.h such as JavaLangInteger. These types are used to generate all
// Java method signatures at compile time automatically.

// TODO(b/80553784) Fields are not implemented for now.
// Returns the value of the member variable field |id| in the |obj|. It is
// assumed that the type of the member variables matches |Type|.
// template <typename Type, typename ObjectType>
// Type GetJniField(JNIEnv* env, ObjectType obj, jfieldID id) {
//   using JniType = decltype(JniForward(std::declval<Type>()));
//   using DecayedType = typename std::decay<JniType>::type;
//   const auto jobj = JniForward(obj);
//   if (env && jobj && id) {
//     return detail::JniHelper<DecayedType>::GetFieldValue(env, jobj, id);
//   } else {
//     LOG(DFATAL) << "Could not get JNI field value.";
//     return Type();
//   }
// }

// Calls the given Java static method |id| on the |cls| with the given |args|.
// It is assumed that the signature specified by the |args| and the template
// |ReturnType| matches the method.
template <typename ReturnType, typename... Args>
typename std::enable_if<!std::is_void<ReturnType>::value, ReturnType>::type
CallJniStaticMethod(JNIEnv* env, jclass cls, jmethodID id, Args&&... args) {
  using JniReturnType = decltype(JniForward(std::declval<ReturnType>()));
  using DecayedType = typename std::decay<JniReturnType>::type;
  if (env && cls && id) {
    return detail::JniHelper<DecayedType>::CallStaticMethod(
        env, cls, id, JniForward(std::forward<Args>(args))...);
  } else {
    LOG(DFATAL) << "Could not call JNI static method.";
    return ReturnType();
  }
}

// Void specialization, we can't use JniForward(void).
template <typename ReturnType, typename... Args>
typename std::enable_if<std::is_void<ReturnType>::value, void>::type
CallJniStaticMethod(JNIEnv* env, jclass cls, jmethodID id, Args&&... args) {
  if (env && cls && id) {
    detail::JniHelper<void>::CallStaticMethod(
        env, cls, id, JniForward(std::forward<Args>(args))...);
  } else {
    LOG(DFATAL) << "Could not call JNI static method.";
  }
}

// Same as above but will find the jmethodID from the string_view |name|.
template <typename ReturnType, typename... Args>
ReturnType CallJniStaticMethod(JNIEnv* env, jclass cls, string_view name,
                               Args&&... args) {
  jmethodID id = GetJniStaticMethodId<ReturnType, Args...>(env, cls, name);
  return CallJniStaticMethod<ReturnType>(env, cls, id,
                                         std::forward<Args>(args)...);
}

// Same as above but will find the jclass from the string_view |class_name|.
template <typename ReturnType, typename... Args>
ReturnType CallJniStaticMethod(JNIEnv* env, string_view class_name,
                               string_view name, Args&&... args) {
  const jclass cls = env->FindClass(class_name.c_str());
  return CallJniStaticMethod<ReturnType>(env, cls, name,
                                         std::forward<Args>(args)...);
}

// Calls the given Java method |id| on the |obj| with the given |args|.  It is
// assumed that the signature specified by the |args| and the template
// |ReturnType| matches the method.
template <typename ReturnType, typename ObjectType, typename... Args>
typename std::enable_if<!std::is_void<ReturnType>::value, ReturnType>::type
CallJniMethod(JNIEnv* env, ObjectType obj, jmethodID id, Args&&... args) {
  using JniReturnType = decltype(JniForward(std::declval<ReturnType>()));
  using DecayedType = typename std::decay<JniReturnType>::type;
  const auto jobj = JniForward(obj);
  if (env && jobj && id) {
    return detail::JniHelper<DecayedType>::CallMethod(
        env, jobj, id, JniForward(std::forward<Args>(args))...);
  } else {
    LOG(DFATAL) << "Could not call JNI method.";
    return ReturnType();
  }
}

// Void specialization, since we can't use JniForward(void).
template <typename ReturnType, typename ObjectType, typename... Args>
typename std::enable_if<std::is_void<ReturnType>::value, void>::type
CallJniMethod(JNIEnv* env, ObjectType obj, jmethodID id, Args&&... args) {
  const auto jobj = JniForward(obj);
  if (env && jobj && id) {
    detail::JniHelper<void>::CallMethod(
        env, jobj, id, JniForward(std::forward<Args>(args))...);
  } else {
    LOG(DFATAL) << "Could not call JNI method. " << env << ", " << jobj << ", "
                << id;
  }
}

// Same as above but will find the jmethodID from the string_view |name|.
template <typename ReturnType, typename ObjectType, typename... Args>
ReturnType CallJniMethod(JNIEnv* env, ObjectType obj, string_view name,
                         Args&&... args) {
  jmethodID id =
      GetJniMethodId<ReturnType, Args...>(env, JniForward(obj), name);
  return CallJniMethod<ReturnType>(env, obj, id, std::forward<Args>(args)...);
}

// Create a new Java object instance of |cls| with constructor |id| and the
// given |args|. It is assumed that the signature specified by the |args|
// matches a constructor.
template <typename ReturnType, typename... Args>
ReturnType NewJniObject(JNIEnv* env, jclass cls, jmethodID id, Args&&... args) {
  if (env && cls && id) {
    ScopedJavaExceptionGuard guard(env);
    return env->NewObject(cls, id, JniForward(std::forward<Args>(args))...);
  } else {
    LOG(DFATAL) << "Could not create new JNI object.";
    return ReturnType();
  }
}

// Same as above but will find the constructor jmethodID from |args|.
template <typename ReturnType, typename... Args>
ReturnType NewJniObject(JNIEnv* env, jclass cls, Args&&... args) {
  jmethodID id = GetJniConstructorId<Args...>(env, cls);
  return NewJniObject<ReturnType>(env, cls, id, std::forward<Args>(args)...);
}

// Same as above but will find the jclass from the string_view |class_name|.
template <typename ReturnType, typename... Args>
ReturnType NewJniObject(JNIEnv* env, string_view class_name, Args&&... args) {
  const jclass cls = env->FindClass(class_name.c_str());
  return NewJniObject<ReturnType>(env, cls, std::forward<Args>(args)...);
}

}  // namespace lull

#endif  // LULLABY_MODULES_JNI_JNI_UTIL_H_
