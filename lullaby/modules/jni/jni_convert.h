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

#ifndef LULLABY_MODULES_JNI_JNI_CONVERT_H_
#define LULLABY_MODULES_JNI_JNI_CONVERT_H_

#ifdef __ANDROID__

#include "lullaby/modules/dispatcher/event_wrapper.h"
#include "lullaby/modules/jni/jni_context.h"
#include "lullaby/modules/jni/jni_signature.h"
#include "lullaby/util/variant.h"

namespace lull {

// Converters from Java types to native types and vice versa. We mostly support
// types that are defined in jni_signature, which is much fewer than the types
// supported by Variant. To get around this limitation, Variant::ImplicitCast()
// is used to cast between compatible types.
//
// Arrays (jbooleanArray, jobjectArray, etc) aren't supported for now, use
// ArrayList/VariantArray instead.

// Converts a supported java.lang.Object into a Variant, or NullOpt.
Variant ConvertToNativeObject(JniContext* ctx, jobject jobj);

// Converts a supported Variant into a java.lang.Object, or null.
jobject ConvertToJniObject(JniContext* ctx, const Variant& value);

// Converts an EventWrapper into a com.google.lullaby.Event. This is public
// so that it can used by dispatcher_jni.
ComGoogleLullabyEvent ConvertToJniEvent(JniContext* ctx,
                                        const EventWrapper& event_wrapper);

}  // namespace lull

#endif  // __ANDROID__

#endif  // LULLABY_MODULES_JNI_JNI_CONVERT_H_
