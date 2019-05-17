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

#ifndef LULLABY_MODULES_JNI_SCOPED_JAVA_GLOBAL_REF_H_
#define LULLABY_MODULES_JNI_SCOPED_JAVA_GLOBAL_REF_H_

#ifdef __ANDROID__

#include <jni.h>
#include <memory>

#include "lullaby/modules/jni/jni_context.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/registry.h"

namespace lull {

// Create a Java global reference and properly destroys it when it falls out of
// scope.  It acts like a unique_ptr.
class ScopedJavaGlobalRef {
 public:
  ScopedJavaGlobalRef(jobject object,
                      const std::shared_ptr<Registry>& registry) {
    Create(object, registry);
  }

  ScopedJavaGlobalRef(ScopedJavaGlobalRef&& other)
      : object_(other.object_), registry_(other.registry_) {
    other.object_ = nullptr;
    other.registry_.reset();
  }

  ScopedJavaGlobalRef& operator=(ScopedJavaGlobalRef&& other) {
    if (this != &other) {
      // Delete this object, take ownership of other.
      Delete();

      object_ = other.object_;
      registry_ = other.registry_;
      other.object_ = nullptr;
      other.registry_.reset();
    }
    return *this;
  }

  ScopedJavaGlobalRef(const ScopedJavaGlobalRef& rhs) = delete;
  ScopedJavaGlobalRef& operator=(const ScopedJavaGlobalRef& rhs) = delete;

  ~ScopedJavaGlobalRef() {
    Delete();
  }

  jobject Get() const {
    return object_;
  }

 private:
  void Create(jobject object, const std::shared_ptr<Registry>& registry) {
    auto* ctx = registry->Get<JniContext>();
    if (!ctx) {
      LOG(DFATAL) << "No jni context.";
      return;
    }

    JNIEnv* env = ctx->GetJniEnv();
    if (!env) {
      LOG(DFATAL) << "No JNIEnv.";
      return;
    }

    object_ = env->NewGlobalRef(object);
    registry_ = registry;
  }

  void Delete() {
    if (!object_) {
      return;
    }

    auto registry = registry_.lock();
    if (!registry) {
      return;
    }

    auto* ctx = registry->Get<JniContext>();
    if (!ctx) {
      LOG(DFATAL) << "No jni context.";
      return;
    }

    JNIEnv* env = ctx->GetJniEnv();
    if (!env) {
      LOG(DFATAL) << "No JNIEnv.";
      return;
    }

    env->DeleteGlobalRef(object_);
    object_ = nullptr;
    registry_.reset();
  }

  jobject object_ = nullptr;
  std::weak_ptr<Registry> registry_;
};

}  // namespace lull

#endif  // __ANDROID__

#endif  // LULLABY_MODULES_JNI_SCOPED_JAVA_GLOBAL_REF_H_
