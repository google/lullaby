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

#include "lullaby/modules/jni/jni_util.h"
#include "lullaby/modules/jni/registry_jni.h"
#include "lullaby/util/android_context.h"

extern "C" {

LULLABY_JNI_FN(void, AndroidContext, nativeCreate)
(JNIEnv* env, jobject obj, jlong native_registry_handle,
 jobject android_context, jobject android_activity,
 jobject android_class_loader, jobject android_asset_manager) {
  auto registry = lull::GetRegistryFromJni(native_registry_handle);
  if (!registry) {
    return;
  }

  JavaVM* vm = nullptr;
  env->GetJavaVM(&vm);
#ifdef __ANDROID__
  auto* context = registry->Create<lull::AndroidContext>(vm, JNI_VERSION_1_6);
  context->SetApplicationContext(android_context);
  context->SetActivity(android_activity);
  context->SetClassLoader(android_class_loader);
  context->SetAndroidAssetManager(android_asset_manager);
#endif  // __ANDROID__
}

}  // extern "C"
