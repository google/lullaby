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

#include "lullaby/systems/render/fpl/uniform.h"

#include <cstring>

#include "lullaby/util/logging.h"
#include "lullaby/util/make_unique.h"

namespace lull {

Uniform::Uniform(const Description& desc) : description_(desc) {
  data_.resize(UniformTypeToBytesSize(desc.type) * desc.count);
}

Uniform::Uniform(std::string name, ShaderDataType type, int count,
                 int binding) {
  description_.name = std::move(name);
  description_.type = type;
  description_.count = std::max(1, count);
  description_.binding = binding;
  data_.resize(UniformTypeToBytesSize(description_.type) * description_.count);
}

Uniform::Description& Uniform::GetDescription() { return description_; }

const Uniform::Description& Uniform::GetDescription() const {
  return description_;
}

void Uniform::SetData(const void* data, size_t num_bytes, size_t bytes_offset) {
  if ((bytes_offset + num_bytes) > data_.size()) {
    LOG(DFATAL) << "Uniform buffer overflow. Trying to size " << num_bytes
                << " bytes with offset of " << bytes_offset
                << " to a uniform \"" << description_.name
                << " \" with maximum size of " << data_.size() << ".";
    return;
  }
  std::memcpy(data_.data() + bytes_offset, data, num_bytes);
}

size_t Uniform::UniformTypeToBytesSize(ShaderDataType type) {
  switch (type) {
    case ShaderDataType_Float1:
      return sizeof(float);
    case ShaderDataType_Float2:
      return sizeof(float) * 2;
    case ShaderDataType_Float3:
      return sizeof(float) * 3;
    case ShaderDataType_Float4:
      return sizeof(float) * 4;
    case ShaderDataType_Float2x2:
      return sizeof(float) * 4;
    case ShaderDataType_Float3x3:
      return sizeof(float) * 9;
    case ShaderDataType_Float4x4:
      return sizeof(float) * 16;
    case ShaderDataType_Int1:
      return sizeof(int);
    case ShaderDataType_Int2:
      return sizeof(int) * 2;
    case ShaderDataType_Int3:
      return sizeof(int) * 3;
    case ShaderDataType_Int4:
      return sizeof(int) * 4;
    default:
      LOG(DFATAL) << "Failed to convert uniform type to size.";
      return 1;
  }
}

size_t Uniform::Size() const { return data_.size(); }

}  // namespace lull
