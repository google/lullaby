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

#include "lullaby/systems/render/uniform.h"

#include <cstring>

#include "lullaby/util/logging.h"
#include "lullaby/util/make_unique.h"

namespace lull {

Uniform::Uniform(const Description& desc) : description_(desc) {
  data_.resize(desc.num_bytes);
}

Uniform::Description& Uniform::GetDescription() { return description_; }

const Uniform::Description& Uniform::GetDescription() const {
  return description_;
}

void Uniform::SetData(const void* data, size_t num_bytes, size_t bytes_offset) {
  if ((bytes_offset + num_bytes) > description_.num_bytes) {
    LOG(DFATAL) << "Uniform buffer overflow. Trying to size " << num_bytes
                << " bytes with offset of " << bytes_offset
                << " to a uniform \"" << description_.name
                << " \" with maximum size of " << description_.num_bytes << ".";
    return;
  }
  std::memcpy(data_.data() + bytes_offset, data, num_bytes);
}

}  // namespace lull
