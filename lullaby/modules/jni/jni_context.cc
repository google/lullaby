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

#include "lullaby/modules/jni/jni_context.h"

#include <atomic>

// For some reason, the signature of AttachCurrentThread is different.
#ifdef __ANDROID__
#define JNI_PTR JNIEnv
#else  // __ANDROID__
#define JNI_PTR void
#endif  // __ANDROID__

namespace lull {
namespace {

// Once a valid JavaVM has been set, it should never be reset or changed.
// However, as it may be accessed from multiple threads, access needs to be
// synchronized.
std::atomic<JavaVM*> g_jvm{nullptr};
std::atomic<jint> g_jni_version{0};

// This class is designed to only be stored as a thread_local, because it stores
// JNIEnv's that are only valid in the thread they are created in.
class ThreadLocalJniEnv {
 public:
  ~ThreadLocalJniEnv() {
    // If we call AttachCurrentThread(), we need to call DetachCurrentThread().
    // The stored result |attached_| indicates this.
    if (!attached_) {
      return;
    }

    JavaVM* jvm = g_jvm.load();
    if (!jvm) {
      return;
    }

    jint ret = jvm->DetachCurrentThread();
    DCHECK_EQ(JNI_OK, ret);
  }

  // If coming from a Java to C++ JNI call, prefer to pass in the provided
  // JNIEnv.
  void SetJniEnv(JNIEnv* env) { env_ = env; }

  // Returns the last JNIEnv Set() on this thread, or Attaches a new JNIEnv if
  // none available.  The attached JNIEnv will be automatically Detached when
  // the thread terminates.  Do not access this returned value from different
  // threads, get a new one for each thread.
  JNIEnv* GetJniEnv() {
    if (env_) {
      return env_;
    }
    if (attached_) {
      return attached_;
    }

    JavaVM* jvm = g_jvm.load();
    if (!jvm) {
      return nullptr;
    }

    // GetEnv() only returns if there is already an attached thread, which means
    // we don't need to DetachCurrentThread() afterwards.
    JNIEnv* env = nullptr;
    if (jvm->GetEnv(reinterpret_cast<void**>(&env), g_jni_version.load()) ==
        JNI_OK) {
      env_ = env;
      return env;
    }

    // If we call AttachCurrentThread(), we need to call DetachCurrentThread().
    // Store the result as |attached_| to indicate this.
    if (jvm->AttachCurrentThread(reinterpret_cast<JNI_PTR**>(&env), nullptr) ==
        JNI_OK) {
      attached_ = env;
      return env;
    }

    return nullptr;
  }

 private:
  JNIEnv* env_ = nullptr;
  JNIEnv* attached_ = nullptr;
};
thread_local ThreadLocalJniEnv tl_jni_env;

}  // namespace

JniContext::JniContext(JNIEnv* env) {
  JavaVM* vm = nullptr;
  env->GetJavaVM(&vm);

  JavaVM* old_jvm = g_jvm.exchange(vm);
  if (old_jvm && old_jvm != vm) {
    LOG(DFATAL) << "Only one valid Java VM should exist";
    return;
  }

  g_jni_version = env->GetVersion();
}

void JniContext::SetJniEnv(JNIEnv* env) { tl_jni_env.SetJniEnv(env); }

JNIEnv* JniContext::GetJniEnv() { return tl_jni_env.GetJniEnv(); }

bool JniContext::CheckArgs(jobject obj, jmethodID id, const char* err) {
  if (!CheckJniEnv(err)) {
    return false;
  }
  if (!obj) {
    LOG(DFATAL) << err << " No jobject.";
    return false;
  }
  if (!id) {
    LOG(DFATAL) << err << " No jmethodID.";
    return false;
  }
  return true;
}

bool JniContext::CheckArgs(jclass cls, jmethodID id, const char* err) {
  if (!CheckJniEnv(err)) {
    return false;
  }
  if (!cls) {
    LOG(DFATAL) << err << " No class.";
    return false;
  }
  if (!id) {
    LOG(DFATAL) << err << " No jmethodID.";
    return false;
  }
  return true;
}

bool JniContext::CheckJniEnv(const char* err) {
  if (!GetJniEnv()) {
    LOG(DFATAL) << err << " No JNIEnv.";
    return false;
  }
  return true;
}

const char* JniContext::ToCString(string_view view) {
  DCHECK(view.empty() || view.data() == nullptr ||
         view.data()[view.size()] == 0);
  return view.data();
}

}  // namespace lull
