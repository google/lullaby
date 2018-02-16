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

#ifndef LULLABY_UTIL_JNI_UTIL_H_
#define LULLABY_UTIL_JNI_UTIL_H_

#ifdef __ANDROID__

#include <jni.h>
#include <string>
#include "lullaby/util/scoped_java_exception_guard.h"

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

// Returns the value of the member variable |field| in the |obj|. It is assumed
// that the type of the member variables matches |Type|.
template <typename Type>
Type GetJniField(JNIEnv* env, jobject obj, jfieldID id) {
  if (env && obj && id) {
    return detail::JniHelper<Type>::GetFieldValue(env, obj, id);
  } else {
    LOG(DFATAL) << "Could not get JNI field value.";
    return Type();
  }
}

// Calls the given Java static |method| on the |cls| with the given |args|.  I
// is assumed that the signature specified by the |args| (and the template
// ReturnType) matches the method.
template <typename ReturnType, typename... Args>
ReturnType CallJniStaticMethod(JNIEnv* env, jclass cls, jmethodID id,
                               Args&&... args) {
  if (env && cls && id) {
    return detail::JniHelper<ReturnType>::CallStaticMethod(
        env, cls, id, std::forward<Args>(args)...);
  } else {
    LOG(DFATAL) << "Could not call JNI static method.";
    return ReturnType();
  }
}

// Calls the given Java |method| on the |obj| with the given |args|.  It is
// assumed that the signature specified by the |args| (and the template
// ReturnType) matches the method.
template <typename ReturnType, typename... Args>
ReturnType CallJniMethod(JNIEnv* env, jobject obj, jmethodID id,
                         Args&&... args) {
  if (env && obj && id) {
    return detail::JniHelper<ReturnType>::CallMethod(
        env, obj, id, std::forward<Args>(args)...);
  } else {
    LOG(DFATAL) << "Could not call JNI method.";
    return ReturnType();
  }
}

}  // namespace lull

#endif  // __ANDROID__

#endif  // LULLABY_UTIL_JNI_UTIL_H_
