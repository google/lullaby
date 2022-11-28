/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#include "redux/modules/base/static_registry.h"

#include "absl/base/attributes.h"

namespace redux {

ABSL_CONST_INIT StaticRegistry* global_registry_ = nullptr;

StaticRegistry::StaticRegistry(CreateFn fn) : fn_(fn) {
  next_ = global_registry_;
  global_registry_ = this;
}

void StaticRegistry::Create(Registry* registry) {
  for (StaticRegistry* node = global_registry_; node != nullptr;
       node = node->next_) {
    node->fn_(registry);
  }
}

}  // namespace redux
