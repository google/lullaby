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

#include "lullaby/util/android_context.h"

#include "lullaby/util/logging.h"

#ifdef __ANDROID__

#include <pthread.h>

#include <atomic>

namespace lull {

namespace {

// Once a valid JavaVM has been set, it should never be reset or changed.
// However, as it may be accessed from multiple threads, access needs to be
// synchronized.
std::atomic<JavaVM*> g_jvm{nullptr};
std::atomic<jint> g_jni_version{0};
pthread_key_t g_pthread_key;

JNIEnv* AttachCurrentThread() {
  JavaVM* jvm = g_jvm.load();
  if (!jvm) {
    return nullptr;
  }

  // Avoid attaching threads provided by the JVM.
  JNIEnv* env = nullptr;
  if (jvm->GetEnv(reinterpret_cast<void**>(&env), g_jni_version.load()) ==
      JNI_OK) {
    return env;
  }

  env = static_cast<JNIEnv*>(pthread_getspecific(g_pthread_key));
  if (!env) {
    jint ret =
        jvm->AttachCurrentThread(reinterpret_cast<JNIEnv**>(&env), nullptr);
    if (ret != JNI_OK) {
      return nullptr;
    }

    pthread_setspecific(g_pthread_key, env);
  }

  return env;
}

int DetachCurrentThread() {
  JavaVM* jvm = g_jvm.load();
  if (!jvm) {
    return 0;
  }

  jint ret = jvm->DetachCurrentThread();
  DCHECK_EQ(JNI_OK, ret);
  return ret;
}

// Helper function to use DetachCurrentThread with pthread_key_create.
// We can't use std::bind here since the code operates on C function pointers.
void DetachCurrentThreadWrapper(void* value) {
  JNIEnv* env = reinterpret_cast<JNIEnv*>(value);
  if (env) {
    DetachCurrentThread();
    pthread_setspecific(g_pthread_key, nullptr);
  }
}

// Sets the attached JavaVM.
void SetJavaVM(JavaVM* vm, jint jni_version) {
  JavaVM* old_jvm = g_jvm.exchange(vm);
  DCHECK(!old_jvm || old_jvm == vm) << "Only one valid Java VM should exist";
  g_jni_version = jni_version;
  pthread_key_create(&g_pthread_key, DetachCurrentThreadWrapper);
}

}  // namespace

AndroidContext::AndroidContext(JavaVM* jvm, jint version) {
  SetJavaVM(jvm, version);
}

AndroidContext::~AndroidContext() {
  JNIEnv* env = AttachCurrentThread();
  if (context_ != nullptr) {
    env->DeleteWeakGlobalRef(context_);
  }

  if (class_loader_ != nullptr) {
    env->DeleteWeakGlobalRef(class_loader_);
  }
}

JNIEnv* AndroidContext::GetJniEnv() const {
  return AttachCurrentThread();
}

void AndroidContext::SetApplicationContext(jobject context) {
  JNIEnv* env = AttachCurrentThread();
  CHECK_NOTNULL(env);
  if (context_ != nullptr) {
    env->DeleteWeakGlobalRef(context_);
  }

  context_ = env->NewWeakGlobalRef(context);
  CHECK_NOTNULL(context_);
}

ScopedJavaLocalRef AndroidContext::GetApplicationContext() const {
  JNIEnv* env = AttachCurrentThread();
  CHECK_NOTNULL(env);
  return ScopedJavaLocalRef(env->NewLocalRef(context_), env);
}

void AndroidContext::SetClassLoader(jobject loader) {
  JNIEnv* env = AttachCurrentThread();
  CHECK_NOTNULL(env);
  if (class_loader_ != nullptr) {
    env->DeleteWeakGlobalRef(class_loader_);
  }

  class_loader_ = env->NewWeakGlobalRef(loader);
  CHECK_NOTNULL(class_loader_);
}

ScopedJavaLocalRef AndroidContext::GetClassLoader() const {
  JNIEnv* env = AttachCurrentThread();
  CHECK_NOTNULL(env);
  return ScopedJavaLocalRef(env->NewLocalRef(class_loader_), env);
}

}  // namespace lull

#endif  // __ANDROID__
