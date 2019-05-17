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

#ifndef LULLABY_MODULES_JNI_REGISTRY_JNI_H_
#define LULLABY_MODULES_JNI_REGISTRY_JNI_H_

#ifdef __ANDROID__

#include <memory>

#include "lullaby/modules/jni/jni_util.h"
#include "lullaby/util/registry.h"

namespace lull {

/// Turns the long from Java Registry.getNativeHandle() into a
/// std::shared_ptr<Registry>.
std::shared_ptr<Registry> GetRegistryFromJni(jlong native_registry_handle);

}  // namespace lull

/// Native classes can use this macro to call simple static methods from
/// their corresponding Java class. The Java class should declare a method
/// like this, which receives the long from Registry.getNativeHandle():
///
///   class FunctionBinder {
///     private static native void nativeCreate(long registry);
///   }
///
/// The native class should declare a static method like this, which receives a
/// Registry pointer:
///
///   class FunctionBinder {
///    public:
///     static FunctionBinder* Create(Registry* registry);
///   };
///
/// Then, the jni.cc file can define something like one of these:
///
///   LULLABY_JNI_CALL_WITH_REGISTRY(FunctionBinder, nativeCreate, Create)
///   LULLABY_JNI_CALL_CLASS_WITH_REGISTRY(Dispatcher, nativeCreateQueued,
///                                        QueuedDispatcher, Create)
///
/// A similar convenience macro is provided in entity_factory_jni.h for Systems.
#define LULLABY_JNI_CALL_CLASS_WITH_REGISTRY(Class_, Method_, NativeClass_, \
                                             NativeName_)                   \
  extern "C" {                                                              \
  LULLABY_JNI_FN(void, Class_, Method_)                                     \
  (JNIEnv * env, jobject obj, jlong native_registry_handle) {               \
    if (auto registry = lull::GetRegistryFromJni(native_registry_handle)) { \
      lull::NativeClass_::NativeName_(registry.get());                      \
    }                                                                       \
  }                                                                         \
  }  // extern "C"

#else  // __ANDROID__

#define LULLABY_JNI_CALL_CLASS_WITH_REGISTRY(Class_, Method_, NativeClass_, \
                                             NativeName_)

#endif  // __ANDROID__

/// Convenience macro if the Java and native class have the same name.
#define LULLABY_JNI_CALL_WITH_REGISTRY(Class_, Method_, NativeMethod_) \
  LULLABY_JNI_CALL_CLASS_WITH_REGISTRY(Class_, Method_, Class_, NativeMethod_)

#endif  // LULLABY_MODULES_JNI_REGISTRY_JNI_H_
