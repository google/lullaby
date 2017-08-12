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

#include "lullaby/modules/script/lull/script_value.h"

namespace lull {

ScriptValue::ScriptValue(ScriptValue&& rhs) { std::swap(impl_, rhs.impl_); }

ScriptValue::ScriptValue(const ScriptValue& rhs) { Acquire(rhs.impl_); }

ScriptValue& ScriptValue::operator=(const ScriptValue& rhs) {
  if (this != &rhs) {
    Release();
    Acquire(rhs.impl_);
  }
  return *this;
}

ScriptValue& ScriptValue::operator=(ScriptValue&& rhs) {
  if (this != &rhs) {
    Release();
    std::swap(impl_, rhs.impl_);
  }
  return *this;
}

void ScriptValue::Acquire(Impl* other) {
  Release();
  if (other) {
    ++other->count;
    impl_ = other;
  }
}

void ScriptValue::Release() {
  if (impl_) {
    --impl_->count;
    if (impl_->count == 0) {
      delete impl_;
    }
    impl_ = nullptr;
  }
}

}  // namespace lull
