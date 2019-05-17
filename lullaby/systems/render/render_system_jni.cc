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

#ifdef __ANDROID__
#include <android/native_window_jni.h>
#endif  // __ANDROID__

#include "lullaby/events/render_events.h"
#include "lullaby/modules/ecs/entity_factory_jni.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/jni/jni_util.h"
#include "lullaby/modules/jni/registry_jni.h"
#include "lullaby/systems/render/render_system.h"

LULLABY_JNI_CREATE_SYSTEM(RenderSystem, nativeCreate)

extern "C" {

LULLABY_JNI_FN(void, RenderSystem, nativeSetNativeWindow)
(JNIEnv* env, jobject obj, jlong native_registry_handle, jobject jsurface) {
  auto registry = lull::GetRegistryFromJni(native_registry_handle);
  if (!registry) {
    return;
  }
  auto* dispatcher = registry->Get<lull::Dispatcher>();
  if (!dispatcher) {
    LOG(DFATAL) << "No Dispatcher.";
    return;
  }

#ifdef __ANDROID__
  if (jsurface) {
    auto window = ANativeWindow_fromSurface(env, jsurface);
    lull::SetNativeWindowEvent event(window);
    dispatcher->Send(event);
  }
#else  // __ANDROID__
  LOG(DFATAL) << "nativeSetNativeWindow on unsupported platform.";
#endif  // __ANDROID__
}

}  // extern "C"
