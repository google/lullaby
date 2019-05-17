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

#ifndef LULLABY_UTIL_ANDROID_CONTEXT_H_
#define LULLABY_UTIL_ANDROID_CONTEXT_H_

#ifdef __ANDROID__

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>

#include "lullaby/modules/jni/scoped_java_local_ref.h"
#include "lullaby/util/typeid.h"

namespace lull {

// Provides Lullaby with access to Java- and Android-specific classes such as
// the JNIEnv, application Context, ClassLoader, etc.  Instances of this object
// must only be interacted with on the thread they were created on.
class AndroidContext {
 public:
  // Initializes the AndroidContext using the provided Java VM and JNI version.
  AndroidContext(JavaVM* jvm, jint version);
  explicit AndroidContext(AAssetManager* asset_manager);
  ~AndroidContext();

  AndroidContext(const AndroidContext&) = delete;
  AndroidContext& operator=(const AndroidContext&) = delete;

  // Returns the JNIEnv attached to the calling thread.
  // TODO Deprecate and replace with JniContext.
  JNIEnv* GetJniEnv() const;

  // Sets the java.android.Context associated with the current running app.  It
  // is expected that this reference's lifetime will outlive the lifetime of the
  // AndroidContext.
  void SetApplicationContext(jobject context);

  // Returns a weak JNI reference to the java.android.Context associated with
  // this AndroidContext.  It is expected that callers will acquire their own
  // appropriately-scoped reference to the object for use.
  ScopedJavaLocalRef GetApplicationContext() const;

  // Sets the java.android.Activity associated with the current running app.
  // It is expected that this reference's lifetime will outlive the lifetime of
  // the AndroidContext.
  void SetActivity(jobject activity);

  // Returns a weak JNI reference to the java.android.Activity associated
  // with this AndroidContext.  It is expected that callers will acquire their
  // own appropriately-scoped reference to the object for use.
  ScopedJavaLocalRef GetActivity() const;

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
  // AAssetManager*.
  void SetAndroidAssetManagerFromPtr(AAssetManager* manager);

  // Sets the AAssetManager to use for loading assets from the specified
  // java.android.Context.
  void SetAndroidAssetManagerFromContext(jobject context);

  // Returns the AAssetManager to use for loading assets.
  AAssetManager* GetAndroidAssetManager();

 private:
  jweak context_ = nullptr;
  jweak activity_ = nullptr;
  jweak class_loader_ = nullptr;
  jweak asset_manager_ = nullptr;
  AAssetManager* asset_manager_ptr_ = nullptr;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::AndroidContext);

#endif  // __ANDROID__

#endif  // LULLABY_UTIL_ANDROID_CONTEXT_H_
