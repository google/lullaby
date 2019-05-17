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

#include "lullaby/modules/ecs/component_handlers.h"

namespace lull {

bool ComponentHandlers::IsRegistered(DefType def_type) const {
  return GetHandlers(def_type) != nullptr;
}

bool ComponentHandlers::Verify(DefType def_type, Span<uint8_t> def) const {
  const Handlers* handlers = GetHandlers(def_type);
  if (!handlers) {
    return false;
  }
  return handlers->verify(def);
}

void ComponentHandlers::ReadFromFlatbuffer(
    DefType def_type, Variant* def_t_variant,
    const flatbuffers::Table* def) const {
  const Handlers* handlers = GetHandlers(def_type);
  if (!handlers) {
    return;
  }
  handlers->read(def_t_variant, def);
}

void* ComponentHandlers::WriteToFlatbuffer(DefType def_type,
                                           Variant* def_t_variant,
                                           InwardBuffer* buffer) const {
  const Handlers* handlers = GetHandlers(def_type);
  if (!handlers) {
    return buffer->BackAt(buffer->BackSize());
  }
  return handlers->write(def_t_variant, buffer);
}

const ComponentHandlers::Handlers* ComponentHandlers::GetHandlers(
    DefType def_type) const {
  const auto iter = handlers_.find(def_type);
  if (iter != handlers_.end()) {
    return &iter->second;
  } else {
    return nullptr;
  }
}

}  // namespace lull
