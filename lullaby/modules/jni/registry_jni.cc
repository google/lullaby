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

#include "lullaby/modules/jni/registry_jni.h"

#include "lullaby/modules/jni/jni_context.h"

namespace {
using RegistrySharedPtr = std::shared_ptr<lull::Registry>;
using RegistryWeakPtr = std::weak_ptr<lull::Registry>;

RegistrySharedPtr* ToSharedPtr(jlong handle) {
  if (handle == 0) {
    return nullptr;
  }
  return reinterpret_cast<RegistrySharedPtr*>(handle);
}

RegistryWeakPtr* ToWeakPtr(jlong handle) {
  if (handle == 0) {
    return nullptr;
  }
  return reinterpret_cast<RegistryWeakPtr*>(handle);
}
}  // namespace

namespace lull {
std::shared_ptr<Registry> GetRegistryFromJni(jlong native_registry_handle) {
  if (auto* weak = ToWeakPtr(native_registry_handle)) {
    return weak->lock();
  }
  return nullptr;
}
}  // namespace lull

extern "C" {

LULLABY_JNI_FN(jlong, Registry, nativeAcquireWeakPtrAndCreateJniContext)
(JNIEnv* env, jobject obj, jlong native_registry_shared_ptr) {
  if (auto* shared = ToSharedPtr(native_registry_shared_ptr)) {
    (*shared)->Create<lull::JniContext>(env);
    auto* weak = new RegistryWeakPtr(*shared);
    return reinterpret_cast<intptr_t>(weak);
  }
  return 0;
}

LULLABY_JNI_FN(void, Registry, nativeReleaseWeakPtr)
(JNIEnv* env, jobject obj, jlong native_registry_handle) {
  auto* weak = ToWeakPtr(native_registry_handle);
  delete weak;
}

LULLABY_JNI_FN(jlong, Registry, nativeCreate)
(JNIEnv* env, jobject obj) {
  auto* native_registry_shared_ptr =
      new RegistrySharedPtr(new lull::Registry());
  return reinterpret_cast<intptr_t>(native_registry_shared_ptr);
}

LULLABY_JNI_FN(void, Registry, nativeDestroy)
(JNIEnv* env, jobject obj, jlong native_registry_shared_ptr) {
  auto* shared = ToSharedPtr(native_registry_shared_ptr);
  delete shared;
}

}  // extern "C"
