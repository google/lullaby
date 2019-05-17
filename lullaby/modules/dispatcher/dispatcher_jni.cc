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

#include "lullaby/modules/dispatcher/dispatcher_binder.h"
#include "lullaby/modules/jni/jni_context.h"
#include "lullaby/modules/jni/jni_convert.h"
#include "lullaby/modules/jni/registry_jni.h"
#include "lullaby/modules/jni/scoped_java_global_ref.h"

namespace lull {

Dispatcher::EventHandler CreateJniEventHandler(
    std::shared_ptr<Registry> registry, jobject jhandler, void** owner) {
  auto jhandler_ref = std::make_shared<ScopedJavaGlobalRef>(jhandler, registry);
  *owner = jhandler_ref.get();
  return [registry, jhandler_ref](const EventWrapper& event_wrapper) {
    auto* ctx = registry->Get<JniContext>();
    if (!ctx) {
      LOG(DFATAL) << "No jni context.";
      return;
    }

    const auto event = ConvertToJniEvent(ctx, event_wrapper);
    ctx->CallJniMethod<void>(jhandler_ref->Get(), "handleEvent", event);
  };
}

}  // namespace lull

LULLABY_JNI_CALL_CLASS_WITH_REGISTRY(Dispatcher, nativeCreateQueued,
                                     DispatcherBinder, CreateQueuedDispatcher)

extern "C" {

LULLABY_JNI_FN(jlong, Dispatcher, nativeConnect)
(JNIEnv* env, jobject obj, jlong native_registry_handle, jlong jtype,
 jobject jhandler) {
  auto registry = lull::GetRegistryFromJni(native_registry_handle);
  if (!registry) {
    return 0;
  }

  auto* dispatcher = registry->Get<lull::Dispatcher>();
  if (dispatcher == nullptr) {
    LOG(DFATAL) << "No dispatcher.";
    return 0;
  }

  void* owner = nullptr;
  auto event_handler =
      lull::CreateJniEventHandler(registry, jhandler, &owner);
  const auto connection = dispatcher->Connect(static_cast<lull::TypeId>(jtype),
                                              owner, std::move(event_handler));
  return static_cast<jlong>(connection.GetId());
}

}  // extern "C"
