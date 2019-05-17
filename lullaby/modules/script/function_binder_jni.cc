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

#include "lullaby/modules/function/function_call.h"
#include "lullaby/modules/jni/jni_context.h"
#include "lullaby/modules/jni/jni_convert.h"
#include "lullaby/modules/jni/jni_util.h"
#include "lullaby/modules/jni/registry_jni.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/util/hash.h"

LULLABY_JNI_CALL_WITH_REGISTRY(FunctionBinder, nativeCreate, Create)

extern "C" {

LULLABY_JNI_FN(jobject, FunctionBinder, nativeCallFunction)
(JNIEnv* env, jobject obj, jlong native_registry_handle, jlong function,
 jobjectArray args) {
  auto registry = lull::GetRegistryFromJni(native_registry_handle);
  if (!registry) {
    return nullptr;
  }

  auto* ctx = registry->Get<lull::JniContext>();
  if (!ctx) {
    LOG(DFATAL) << "No jni context.";
    return nullptr;
  }
  ctx->SetJniEnv(env);

  auto* function_binder = registry->Get<lull::FunctionBinder>();
  if (function_binder == nullptr) {
    LOG(DFATAL) << "No function binder.";
    return nullptr;
  }

  lull::FunctionCall call(static_cast<lull::HashValue>(function));
  const jsize num_args = env->GetArrayLength(args);
  for (jsize i = 0; i < num_args; ++i) {
    const jobject arg = env->GetObjectArrayElement(args, i);
    call.AddArg(ConvertToNativeObject(ctx, arg));
  }

  const auto res = function_binder->Call(&call);
  const jobject jres = ConvertToJniObject(ctx, res);
  return jres;
}

}  // extern "C"
