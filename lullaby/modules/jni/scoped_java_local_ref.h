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

#ifndef LULLABY_MODULES_JNI_SCOPED_JAVA_LOCAL_REF_H_
#define LULLABY_MODULES_JNI_SCOPED_JAVA_LOCAL_REF_H_

#ifdef __ANDROID__

#include <jni.h>

namespace lull {

/// Create a Java local reference and properly destroys it when it falls out of
/// scope. There are no guarantees that the stored object or JNIEnv are valid
/// outside of the scope it is acquired in, so this reference should not be
/// stored in any persistent way.
class ScopedJavaLocalRef {
 public:
  ScopedJavaLocalRef(jobject object, JNIEnv* env)
      : object_(env->NewLocalRef(object)), env_(env) {}

  ScopedJavaLocalRef(ScopedJavaLocalRef&& other)
      : object_(other.object_), env_(other.env_) {
    other.object_ = nullptr;
    other.env_ = nullptr;
  }

  ScopedJavaLocalRef& operator=(ScopedJavaLocalRef&& other) {
    if (this != &other) {
      if (env_) {
        env_->DeleteLocalRef(object_);
      }

      object_ = other.object_;
      env_ = other.env_;
      other.object_ = nullptr;
      other.env_ = nullptr;
    }
    return *this;
  }

  ScopedJavaLocalRef(const ScopedJavaLocalRef& rhs) = delete;
  ScopedJavaLocalRef& operator=(const ScopedJavaLocalRef& rhs) = delete;

  ~ScopedJavaLocalRef() {
    if (env_) {
      env_->DeleteLocalRef(object_);
    }
  }

  jobject Get() const {
    return object_;
  }

 private:
  jobject object_;
  JNIEnv* env_;
};

}  // namespace lull

#endif  // __ANDROID__

#endif  // LULLABY_MODULES_JNI_SCOPED_JAVA_LOCAL_REF_H_
