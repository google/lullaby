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

#ifndef LULLABY_UTIL_ANDROID_CONTEXT_H_
#define LULLABY_UTIL_ANDROID_CONTEXT_H_

#include "lullaby/util/scoped_java_local_ref.h"
#include "lullaby/util/typeid.h"

#ifdef __ANDROID__

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>

#include "lullaby/util/jni_util.h"

namespace lull {

// Provides Lullaby with access to Java- and Android-specific classes such as
// the JNIEnv, application Context, ClassLoader, etc.  Instances of this object
// must only be interacted with on the thread they were created on.
class AndroidContext {
 public:
  // Initializes the AndroidContext using the provided Java VM and JNI version.
  AndroidContext(JavaVM* jvm, jint version);
  ~AndroidContext();

  AndroidContext(const AndroidContext&) = delete;
  AndroidContext& operator=(const AndroidContext&) = delete;

  // Returns the JNIEnv attached to the calling thread.
  JNIEnv* GetJniEnv() const;

  // Sets the java.android.Context associated with the current running app.  It
  // is expected that this reference's lifetime will outlive the lifetime of the
  // AndroidContext.
  void SetApplicationContext(jobject context);

  // Returns a weak JNI reference to the java.android.Context associated with
  // this AndroidContext.  It is expected that callers will acquire their own
  // appropriately-scoped reference to the object for use.
  ScopedJavaLocalRef GetApplicationContext() const;

  // Sets the java.lang.ClassLoader associated with the current running app.  It
  // is expected that this reference's lifetime will outlive the lifetime of the
  // AndroidContext.
  void SetClassLoader(jobject loader);

  // Returns a weak JNI reference to the java.lang.ClassLoader associated with
  // this AndroidContext.  It is expected that callers will acquire their own
  // appropriately-scoped reference to the object for use.
  ScopedJavaLocalRef GetClassLoader() const;

  // Sets the AAssetManager to use for loading assets.
  void SetAndroidAssetManager(jobject manager);

  // Sets the AAssetManager to use for loading assets from the specified
  // java.android.Context.
  void SetAndroidAssetManagerFromContext(jobject context);

  // Returns the AAssetManager to use for loading assets.
  AAssetManager* GetAndroidAssetManager();

  // Gets the value of the member variable with |name| from the |object| with
  // the given |Type|.
  template <typename Type>
  Type GetMember(jobject object, string_view name) {
    return GetJniField<Type>(GetJniEnv(), object, name);
  }

  // Calls the method with |name| on the |object| with the given |ReturnType|.
  template <typename ReturnType, typename... Args>
  ReturnType CallMethod(jobject object, string_view name, Args&&... args) {
    return CallJniMethod<ReturnType>(GetJniEnv(), object, name,
                                     std::forward<Args>(args)...);
  }

 private:
  jweak context_ = nullptr;
  jweak class_loader_ = nullptr;
  jweak asset_manager_ = nullptr;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::AndroidContext);

#endif  // __ANDROID__

#endif  // LULLABY_UTIL_ANDROID_CONTEXT_H_
