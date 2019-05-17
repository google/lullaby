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

#include "lullaby/modules/dispatcher/dispatcher_jni.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/jni/jni_util.h"
#include "lullaby/modules/jni/registry_jni.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"

extern "C" {

// Copied from entity_factory_jni, because we need EnableQueuedDispatch().
LULLABY_JNI_FN(void, DispatcherSystem, nativeCreateQueued)
(JNIEnv * env, jobject obj, jlong native_registry_handle) {
  auto registry = lull::GetRegistryFromJni(native_registry_handle);
  if (!registry) {
    return;
  }
  auto* entity_factory = registry->Get<lull::EntityFactory>();
  if (!entity_factory) {
    LOG(DFATAL) << "No EntityFactory.";
    return;
  }
  lull::DispatcherSystem::EnableQueuedDispatch();
  entity_factory->CreateSystem<lull::DispatcherSystem>();
}

LULLABY_JNI_FN(jlong, DispatcherSystem, nativeConnect)
(JNIEnv* env, jobject obj, jlong native_registry_handle, jlong jentity,
 jlong jtype, jobject jconnection) {
  auto registry = lull::GetRegistryFromJni(native_registry_handle);
  if (!registry) {
    return 0;
  }

  auto* dispatcher_system = registry->Get<lull::DispatcherSystem>();
  if (dispatcher_system == nullptr) {
    LOG(DFATAL) << "No DispatcherSystem.";
    return 0;
  }

  void* owner = nullptr;
  auto event_handler =
      lull::CreateJniEventHandler(registry, jconnection, &owner);
  const auto connection = dispatcher_system->Connect(
      static_cast<lull::Entity>(jentity), static_cast<lull::TypeId>(jtype),
      owner, std::move(event_handler));
  return static_cast<jlong>(connection.GetId());
}

}  // extern "C"
