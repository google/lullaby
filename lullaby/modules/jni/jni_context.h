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

#ifndef LULLABY_MODULES_JNI_JNI_CONTEXT_H_
#define LULLABY_MODULES_JNI_JNI_CONTEXT_H_

#ifdef __ANDROID__

#include <jni.h>

#include "lullaby/modules/jni/jni_signature.h"
#include "lullaby/modules/jni/scoped_java_exception_guard.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/string_view.h"
#include "lullaby/util/typeid.h"

namespace lull {

// Provides Lullaby with access to Java-specific classes such as JNIEnv, and
// convenience methods for calling JNI methods or creating objects, using types
// defined in jni_signature.
class JniContext {
 public:
  // Initializes some global state with |env| if not already initialized.
  // We assume there is only one global JVM.
  explicit JniContext(JNIEnv* env);

  // Preferably, call SetJniEnv() with the current thread's |env| so we don't
  // need to AttachCurrentThread on the JVM.  All JNI calls from Java into C++
  // will provide a valid JNIEnv.
  void SetJniEnv(JNIEnv* env);

  // Returns the last JNIEnv set on this thread, or Attaches a new JNIEnv if
  // none available.  The attached JNIEnv will be automatically Detached when
  // the thread terminates.  Do not access this returned value from different
  // threads, get a new one for each thread.
  JNIEnv* GetJniEnv();

  // In the following methods, all |Type|, |ReturnType|, |ObjectType|, or
  // |Args| should be a JNI type such as jint, or one of the custom structs
  // defined in jni_signature.h such as JavaLangInteger. These types are used to
  // generate all Java method signatures at compile time automatically.

  // Calls the given Java static method |id| on the |cls| with the given |args|.
  // It is assumed that the signature specified by the |args| and the template
  // |ReturnType| matches the method.
  template <typename ReturnType, typename... Args>
  typename std::enable_if<!std::is_void<ReturnType>::value, ReturnType>::type
  CallJniStaticMethod(jclass cls, jmethodID id, Args&&... args) {
    using JniReturnType = decltype(JniForward(std::declval<ReturnType>()));
    using DecayedType = typename std::decay<JniReturnType>::type;
    if (CheckArgs(cls, id, "Could not call JNI static method.")) {
      return JniHelper<DecayedType>::CallStaticMethod(
          GetJniEnv(), cls, id, JniForward(std::forward<Args>(args))...);
    } else {
      return ReturnType();
    }
  }

  // Void specialization, we can't use JniForward(void).
  template <typename ReturnType, typename... Args>
  typename std::enable_if<std::is_void<ReturnType>::value, void>::type
  CallJniStaticMethod(jclass cls, jmethodID id, Args&&... args) {
    if (CheckArgs(cls, id, "Could not call JNI static method.")) {
      JniHelper<void>::CallStaticMethod(
          GetJniEnv(), cls, id, JniForward(std::forward<Args>(args))...);
    }
  }

  // Same as above but will find the jmethodID from the string_view |name|.
  template <typename ReturnType, typename... Args>
  ReturnType CallJniStaticMethod(jclass cls, string_view name,
                                 Args&&... args) {
    jmethodID id = GetJniStaticMethodId<ReturnType, Args...>(cls, name);
    return CallJniStaticMethod<ReturnType>(cls, id,
                                           std::forward<Args>(args)...);
  }

  // Same as above but will find the jclass from the string_view |class_name|.
  template <typename ReturnType, typename... Args>
  ReturnType CallJniStaticMethod(string_view class_name,
                                 string_view name, Args&&... args) {
    if (!CheckJniEnv("Could not call JNI static method.")) {
      return ReturnType();
    }
    const jclass cls = GetJniEnv()->FindClass(ToCString(class_name));
    return CallJniStaticMethod<ReturnType>(cls, name,
                                           std::forward<Args>(args)...);
  }

  // Calls the given Java method |id| on the |obj| with the given |args|.  It is
  // assumed that the signature specified by the |args| and the template
  // |ReturnType| matches the method.
  template <typename ReturnType, typename ObjectType, typename... Args>
  typename std::enable_if<!std::is_void<ReturnType>::value, ReturnType>::type
  CallJniMethod(ObjectType obj, jmethodID id, Args&&... args) {
    using JniReturnType = decltype(JniForward(std::declval<ReturnType>()));
    using DecayedType = typename std::decay<JniReturnType>::type;
    const auto jobj = JniForward(obj);
    if (CheckArgs(jobj, id, "Could not call JNI method.")) {
      return JniHelper<DecayedType>::CallMethod(
          GetJniEnv(), jobj, id, JniForward(std::forward<Args>(args))...);
    } else {
      return ReturnType();
    }
  }

  // Void specialization, since we can't use JniForward(void).
  template <typename ReturnType, typename ObjectType, typename... Args>
  typename std::enable_if<std::is_void<ReturnType>::value, void>::type
  CallJniMethod(ObjectType obj, jmethodID id, Args&&... args) {
    const auto jobj = JniForward(obj);
    if (CheckArgs(jobj, id, "Could not call JNI method.")) {
      JniHelper<void>::CallMethod(GetJniEnv(), jobj, id,
                                  JniForward(std::forward<Args>(args))...);
    }
  }

  // Same as above but will find the jmethodID from the string_view |name|.
  template <typename ReturnType, typename ObjectType, typename... Args>
  ReturnType CallJniMethod(ObjectType obj, string_view name,
                           Args&&... args) {
    jmethodID id = GetJniMethodId<ReturnType, Args...>(JniForward(obj), name);
    return CallJniMethod<ReturnType>(obj, id, std::forward<Args>(args)...);
  }

  // Create a new Java object instance of |cls| with constructor |id| and the
  // given |args|. It is assumed that the signature specified by the |args|
  // matches a constructor.
  template <typename ReturnType, typename... Args>
  ReturnType NewJniObject(jclass cls, jmethodID id,
                          Args&&... args) {
    if (CheckArgs(cls, id, "Could not create new JNI object.")) {
      ScopedJavaExceptionGuard guard(GetJniEnv());
      return GetJniEnv()->NewObject(cls, id,
                                    JniForward(std::forward<Args>(args))...);
    } else {
      return ReturnType();
    }
  }

  // Same as above but will find the constructor jmethodID from |args|.
  template <typename ReturnType, typename... Args>
  ReturnType NewJniObject(jclass cls, Args&&... args) {
    jmethodID id = GetJniConstructorId<Args...>(cls);
    return NewJniObject<ReturnType>(cls, id, std::forward<Args>(args)...);
  }

  // Same as above but will find the jclass from the string_view |class_name|.
  template <typename ReturnType, typename... Args>
  ReturnType NewJniObject(string_view class_name, Args&&... args) {
    if (!CheckJniEnv("Could not create new JNI object.")) {
      return ReturnType();
    }
    const jclass cls = GetJniEnv()->FindClass(ToCString(class_name));
    return NewJniObject<ReturnType>(cls, std::forward<Args>(args)...);
  }

  // TODO Fields are not implemented for now.
  // Returns the value of the member variable field |id| in the |obj|. It is
  // assumed that the type of the member variables matches |Type|.
  // template <typename Type, typename ObjectType>
  // Type GetJniField(ObjectType obj, jfieldID id) {
  //   using JniType = decltype(JniForward(std::declval<Type>()));
  //   using DecayedType = typename std::decay<JniType>::type;
  //   const auto jobj = JniForward(obj);
  //   if (CheckArgs(jobj, id, "Could not get JNI field value.")) {
  //     return JniHelper<DecayedType>::GetFieldValue(GetJniEnv(), jobj, id);
  //   } else {
  //     return Type();
  //   }
  // }

  // Getters for jmethodID of various types.
  // TODO Cache method ids
  template <typename ReturnType, typename... Args>
  jmethodID GetJniStaticMethodId(jclass cls, string_view name) {
    if (!CheckJniEnv("Could not get JNI method id.")) {
      return nullptr;
    }
    if (!cls) {
      LOG(DFATAL) << "Could not get JNI method id. No class.";
      return nullptr;
    }
    const JniSignature& sig = GetJniMethodSignature<ReturnType, Args...>();
    return GetJniEnv()->GetStaticMethodID(cls, ToCString(name),
                                          sig.name.c_str());
  }

  template <typename ReturnType, typename... Args>
  jmethodID GetJniMethodId(jobject obj, string_view name) {
    if (!CheckJniEnv("Could not get JNI method id.")) {
      return nullptr;
    }
    const jclass cls = GetJniEnv()->GetObjectClass(obj);
    const JniSignature& sig = GetJniMethodSignature<ReturnType, Args...>();
    return GetJniEnv()->GetMethodID(cls, ToCString(name), sig.name.c_str());
  }

  template <typename... Args>
  jmethodID GetJniConstructorId(jclass cls) {
    if (!CheckJniEnv("Could not get JNI constructor id.")) {
      return nullptr;
    }
    const JniSignature& sig = GetJniMethodSignature<void, Args...>();
    return GetJniEnv()->GetMethodID(cls, "<init>", sig.name.c_str());
  }

 private:
  // Specializations of this class are used to help call JNI functions.
  // UNUSED is a workaround for CWG 727:
  // "full specializations not allowed in class scope, even though partial are"
  template <typename Type, typename UNUSED = void>
  struct JniHelper;

// Specializations of JniHelper for common types.
#define LULLABY_JNI_HELPER(NativeType_, JavaType_)                             \
  template <typename UNUSED>                                                   \
  struct JniHelper<NativeType_, UNUSED> {                                      \
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

  // An (partial) explicit specialization of JniHelper for void.  We can't use
  // the macro specialization because there's no such thing as GetVoidField.
  template <typename UNUSED>
  struct JniHelper<void, UNUSED> {
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

  // Returns true if all of |GetJniEnv()|, |obj|, |cls|, and |id| are not null.
  // If any are null, return false along with logging |err|.
  bool CheckArgs(jobject obj, jmethodID id, const char* err);
  bool CheckArgs(jclass cls, jmethodID id, const char* err);
  // Returns true if |GetJniEnv()| is not null, else false and logs |err|.
  bool CheckJniEnv(const char* err);

  // Returns |view.data()|, with a DCHECK that it is either a null-terminated
  // c-string or a nullptr.
  static const char* ToCString(string_view view);
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::JniContext);

#endif  // __ANDROID__

#endif  // LULLABY_MODULES_JNI_JNI_CONTEXT_H_
