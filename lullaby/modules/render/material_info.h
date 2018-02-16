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

#ifndef LULLABY_UTIL_MATERIAL_INFO_H_
#define LULLABY_UTIL_MATERIAL_INFO_H_

#include <string>
#include "lullaby/modules/flatbuffers/variant_fb_conversions.h"
#include "lullaby/util/variant.h"
#include "lullaby/generated/material_def_generated.h"

namespace lull {

/// Information about the material applied to a draw call.
class MaterialInfo {
 public:
  explicit MaterialInfo(std::string shading_model)
      : shading_model_(std::move(shading_model)) {}

  /// Sets all the material properties.
  void SetProperties(VariantMap properties) {
    properties_ = std::move(properties);
  }

  /// Associates a texture usage with a texture name that can be retrieved from
  /// the TextureFactory.
  void SetTexture(MaterialTextureUsage usage, std::string texture) {
    textures_[usage] = std::move(texture);
  }

  /// Returns the Shading Model to be used by the material.
  const std::string& GetShadingModel() const { return shading_model_; }

  /// Returns the property of type T associated with the |key|, or nullptr if
  /// no such property exists.
  template <typename T>
  const T* GetProperty(HashValue key) const {
    auto iter = properties_.find(key);
    return (iter != properties_.end()) ? iter->second.Get<T>() : nullptr;
  }

  /// Returns the property of type T associated with the |key|, or
  /// |default_value| if no such property exists.
  template <typename T>
  const T& GetProperty(HashValue key, const T& default_value) const {
    auto iter = properties_.find(key);
    if (iter != properties_.end()) {
      const T* ptr = iter->second.Get<T>();
      if (ptr != nullptr) {
        return *ptr;
      }
    }
    return default_value;
  }

  /// Returns the texture name associated with the given texture usage.
  const std::string& GetTexture(MaterialTextureUsage usage) const {
    return textures_[usage];
  }

 private:
  const std::string shading_model_;
  VariantMap properties_;
  std::string textures_[MaterialTextureUsage_MAX];
};

}  // namespace lull

#endif  // LULLABY_UTIL_MATERIAL_INFO_H_
