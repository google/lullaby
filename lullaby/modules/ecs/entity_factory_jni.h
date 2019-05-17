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

#ifndef LULLABY_MODULES_ECS_ENTITY_FACTORY_JNI_H_
#define LULLABY_MODULES_ECS_ENTITY_FACTORY_JNI_H_

#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/jni/jni_util.h"
#include "lullaby/modules/jni/registry_jni.h"
#include "lullaby/util/logging.h"

#ifdef __ANDROID__

/// Native Systems with trivial constructors can use this macro to create native
/// instances from their corresponding Java class. The Java class should declare
/// a method like this, which receives the long from Registry.getNativeHandle():
///
///   class TransformSystem {
///     private static native void nativeCreate(long registry);
///   }
///
/// Then, the jni.cc file can define something like this:
///
///   LULLABY_JNI_CREATE_SYSTEM(TransformSystem, nativeCreate)
#define LULLABY_JNI_CREATE_SYSTEM(Class_, Method_)       \
  extern "C" {                                                        \
  LULLABY_JNI_FN(void, Class_, Method_)                               \
  (JNIEnv * env, jobject obj, jlong native_registry_handle) {         \
    auto registry = lull::GetRegistryFromJni(native_registry_handle); \
    if (!registry) {                                                  \
      return;                                                         \
    }                                                                 \
    auto* entity_factory = registry->Get<lull::EntityFactory>();      \
    if (!entity_factory) {                                            \
      LOG(DFATAL) << "No EntityFactory.";                             \
      return;                                                         \
    }                                                                 \
    entity_factory->CreateSystem<lull::Class_>();                     \
  }                                                                   \
  }  // extern "C"

#else  // __ANDROID__

#define LULLABY_JNI_CREATE_SYSTEM(Class_, Method_)

#endif  // __ANDROID__

#endif  // LULLABY_MODULES_ECS_ENTITY_FACTORY_JNI_H_
