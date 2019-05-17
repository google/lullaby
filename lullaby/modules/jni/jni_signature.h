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

#ifndef LULLABY_MODULES_JNI_JNI_SIGNATURE_H_
#define LULLABY_MODULES_JNI_JNI_SIGNATURE_H_

#ifdef __ANDROID__

#include <jni.h>

#include "lullaby/util/fixed_string.h"
#include "lullaby/util/hash.h"

namespace lull {

// Stores/represents the type signature of a JNI field or method.
//
// This class stores the JNI signature string as defined here under "Type
// Signatures":
// https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/types.html
//
// The following functions can be used to build the type signature from C++
// types.
//
//   template <typename Type>
//   const JniSignature& GetJniFieldSignature();
//
//   template <typename ReturnType, typename Args...>
//   const JniSignature& GetJniMethodSignature();
//
// For example, calling:
//   GetJniMethodSignature<bool, std::string, int, int>()
//
// will return a JniSignature storing:
//   (Ljava/lang/String;II)Z
//
// The functions return static instances, so you only pay the cost of generating
// the string once per signature. Furthermore, the JniSignature stores a hash
// of the string for convenience.
struct JniSignature {
  using String = FixedString<256 - sizeof(HashValue)>;

  JniSignature(String name) : name(std::move(name)), id(Hash(name.c_str())) {}

  String name;
  HashValue id;
};

// JNI only defines the type "jobject" to represent all descendents of Object.
// In order to capture the types at cpp's compile time, we define these custom
// structs for all Java classes to wrap a jobject, which are used in jni_util.h
// to generate java method signatures. JniForward is used to unwrap the custom
// type into base JNI types for native jni functions.

// Default template forwards jni types as is.
template <typename T>
inline T&& JniForward(T& value) {
  return static_cast<T&&>(value);
}
template <typename T>
inline T&& JniForward(T&& value) {
  return static_cast<T&&>(value);
}

// Define the custom type and JniForward specializations.
#define LULLABY_JNI_TYPE(Type_)                     \
  struct Type_ {                                    \
    Type_() : jobj(0) {}                            \
    Type_(jobject jobj) : jobj(jobj) {}             \
    jobject jobj;                                   \
  };                                                \
  inline jobject JniForward(const Type_& value) {   \
    return value.jobj;                              \
  }                                                 \
  inline jobject JniForward(Type_& value) {         \
    return value.jobj;                              \
  }                                                 \
  inline jobject JniForward(Type_&& value) {        \
    return value.jobj;                              \
  }

LULLABY_JNI_TYPE(JavaLangBoolean);
LULLABY_JNI_TYPE(JavaLangInteger);
LULLABY_JNI_TYPE(JavaLangLong);
LULLABY_JNI_TYPE(JavaLangFloat);
LULLABY_JNI_TYPE(JavaLangDouble);
LULLABY_JNI_TYPE(JavaxVecmathVector2f);
LULLABY_JNI_TYPE(JavaxVecmathVector3f);
LULLABY_JNI_TYPE(JavaxVecmathVector4f);
LULLABY_JNI_TYPE(JavaxVecmathQuat4f);
LULLABY_JNI_TYPE(JavaxVecmathMatrix4f);
LULLABY_JNI_TYPE(JavaUtilArrayList);
LULLABY_JNI_TYPE(JavaUtilHashMap);
LULLABY_JNI_TYPE(JavaUtilSet);
LULLABY_JNI_TYPE(JavaUtilIterator);
LULLABY_JNI_TYPE(ComGoogleLullabyEntity);
LULLABY_JNI_TYPE(ComGoogleLullabyEvent);
#undef LULLABY_JNI_TYPE

// Utility classes and functions used to generate the JNI type signature from
// native C++ types.
namespace detail {

template <typename Type>
struct JniSignatureHelper;

#define LULLABY_JNI_SIGNATURE_HELPER(Sig_, NativeType_) \
  template <>                                           \
  struct JniSignatureHelper<NativeType_> {              \
    static const char* Sig() { return Sig_; }           \
  }

LULLABY_JNI_SIGNATURE_HELPER("V", void);
LULLABY_JNI_SIGNATURE_HELPER("Z", jboolean);
LULLABY_JNI_SIGNATURE_HELPER("I", jint);
LULLABY_JNI_SIGNATURE_HELPER("J", jlong);
LULLABY_JNI_SIGNATURE_HELPER("F", jfloat);
LULLABY_JNI_SIGNATURE_HELPER("D", jdouble);
LULLABY_JNI_SIGNATURE_HELPER("[Z", jbooleanArray);
LULLABY_JNI_SIGNATURE_HELPER("[I", jintArray);
LULLABY_JNI_SIGNATURE_HELPER("[J", jlongArray);
LULLABY_JNI_SIGNATURE_HELPER("[F", jfloatArray);
LULLABY_JNI_SIGNATURE_HELPER("[D", jdoubleArray);
LULLABY_JNI_SIGNATURE_HELPER("Ljava/lang/String;", jstring);
LULLABY_JNI_SIGNATURE_HELPER("Ljava/lang/Class;", jclass);
LULLABY_JNI_SIGNATURE_HELPER("Ljava/lang/Object;", jobject);
LULLABY_JNI_SIGNATURE_HELPER("[Ljava/lang/Object;", jobjectArray);

LULLABY_JNI_SIGNATURE_HELPER("Ljava/lang/Boolean;", JavaLangBoolean);
LULLABY_JNI_SIGNATURE_HELPER("Ljava/lang/Integer;", JavaLangInteger);
LULLABY_JNI_SIGNATURE_HELPER("Ljava/lang/Long;", JavaLangLong);
LULLABY_JNI_SIGNATURE_HELPER("Ljava/lang/Float;", JavaLangFloat);
LULLABY_JNI_SIGNATURE_HELPER("Ljava/lang/Double;", JavaLangDouble);
LULLABY_JNI_SIGNATURE_HELPER("Ljavax/vecmath/Vector2f;", JavaxVecmathVector2f);
LULLABY_JNI_SIGNATURE_HELPER("Ljavax/vecmath/Vector3f;", JavaxVecmathVector3f);
LULLABY_JNI_SIGNATURE_HELPER("Ljavax/vecmath/Vector4f;", JavaxVecmathVector4f);
LULLABY_JNI_SIGNATURE_HELPER("Ljavax/vecmath/Quat4f;", JavaxVecmathQuat4f);
LULLABY_JNI_SIGNATURE_HELPER("Ljavax/vecmath/Matrix4f;", JavaxVecmathMatrix4f);
LULLABY_JNI_SIGNATURE_HELPER("Ljava/util/ArrayList;", JavaUtilArrayList);
LULLABY_JNI_SIGNATURE_HELPER("Ljava/util/HashMap;", JavaUtilHashMap);
LULLABY_JNI_SIGNATURE_HELPER("Ljava/util/Set;", JavaUtilSet);
LULLABY_JNI_SIGNATURE_HELPER("Ljava/util/Iterator;", JavaUtilIterator);
LULLABY_JNI_SIGNATURE_HELPER("Lcom/google/lullaby/Entity;",
                             ComGoogleLullabyEntity);
LULLABY_JNI_SIGNATURE_HELPER("Lcom/google/lullaby/Event;",
                             ComGoogleLullabyEvent);
#undef LULLABY_JNI_SIGNATURE_HELPER

// Recursive template class used to generate the signature string for a JNI
// method.
template <typename... Args>
struct JniMethodSignatureBuilder;

// Base-case for JniMethodSignatureBuilder recursion, returns an empty string.
template <>
struct JniMethodSignatureBuilder<> {
  static JniSignature::String Sig() { return ""; }
};

// Recurisve-case for JniMethodSignatureBuilder recusion.
template <typename First, typename... Args>
struct JniMethodSignatureBuilder<First, Args...> {
  using FirstType = typename std::decay<First>::type;
  static JniSignature::String Sig() {
    JniSignature::String res;
    res.append(JniSignatureHelper<FirstType>::Sig());
    res.append(JniMethodSignatureBuilder<Args...>::Sig());
    return res;
  }
};

// Concatenates the string generated by the JniSignatureHelper for the |Args|
// with the java string representation for the |ReturnType| to form a complete
// JNI method signature.
template <typename ReturnType, typename... Args>
JniSignature::String GetJniMethodSignature() {
  JniSignature::String res;
  res.append("(");
  res.append(JniMethodSignatureBuilder<Args...>::Sig());
  res.append(")");
  res.append(JniSignatureHelper<ReturnType>::Sig());
  return res;
}

template <typename Type>
JniSignature::String GetJniFieldSignature() {
  return JniSignatureHelper<Type>::Sig();
}

}  // namespace detail

template <typename Type>
const JniSignature& GetJniFieldSignature() {
  static JniSignature sig(detail::GetJniFieldSignature<Type>());
  return sig;
}

template <typename ReturnType, typename... Args>
const JniSignature& GetJniMethodSignature() {
  static JniSignature sig(detail::GetJniMethodSignature<ReturnType, Args...>());
  return sig;
}

}  // namespace lull

#endif  // __ANDROID__

#endif  // LULLABY_MODULES_JNI_JNI_SIGNATURE_H_
