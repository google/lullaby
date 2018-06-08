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

#ifndef LULLABY_MODULES_JNI_JNI_METHOD_H_
#define LULLABY_MODULES_JNI_JNI_METHOD_H_

#include <jni.h>

#include "lullaby/modules/jni/jni_signature.h"
#include "lullaby/util/string_view.h"

namespace lull {

// TODO(b/90951536) Cache method ids

template <typename ReturnType, typename... Args>
jmethodID GetJniStaticMethodId(JNIEnv* env, jclass cls, string_view name) {
  if (!cls) {
    return nullptr;
  }
  const JniSignature& sig = GetJniMethodSignature<ReturnType, Args...>();
  return env->GetStaticMethodID(cls, name.c_str(), sig.name.c_str());
}

template <typename ReturnType, typename... Args>
jmethodID GetJniMethodId(JNIEnv* env, jobject obj, string_view name) {
  const jclass cls = env->GetObjectClass(obj);
  const JniSignature& sig = GetJniMethodSignature<ReturnType, Args...>();
  return env->GetMethodID(cls, name.c_str(), sig.name.c_str());
}

template <typename... Args>
jmethodID GetJniConstructorId(JNIEnv* env, jclass cls) {
  const JniSignature& sig = GetJniMethodSignature<void, Args...>();
  return env->GetMethodID(cls, "<init>", sig.name.c_str());
}

}  // namespace lull

#endif  // LULLABY_MODULES_JNI_JNI_METHOD_H_
