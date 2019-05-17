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

#ifndef LULLABY_MODULES_JNI_SCOPED_JAVA_EXCEPTION_GUARD_H_
#define LULLABY_MODULES_JNI_SCOPED_JAVA_EXCEPTION_GUARD_H_

#ifdef __ANDROID__

#include <jni.h>
#include "lullaby/util/logging.h"

namespace lull {

// Checks if there has been any pending JNI exception in its destructor.
class ScopedJavaExceptionGuard {
 public:
  explicit ScopedJavaExceptionGuard(JNIEnv* env) : env_(env) {}

  ~ScopedJavaExceptionGuard() {
    if (env_->ExceptionCheck()) {
      env_->ExceptionDescribe();
      env_->ExceptionClear();
      LOG(DFATAL) << "Unhandled JNI Exception";
    }
  }

 private:
  JNIEnv* env_;
};

}  // namespace lull

#endif  // __ANDROID__

#endif  // LULLABY_MODULES_JNI_SCOPED_JAVA_EXCEPTION_GUARD_H_
