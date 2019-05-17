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

#ifndef LULLABY_UTIL_MATERIAL_INFO_H_
#define LULLABY_UTIL_MATERIAL_INFO_H_

#include <algorithm>
#include <array>
#include <set>
#include <string>
#include "lullaby/modules/flatbuffers/variant_fb_conversions.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/variant.h"
#include "lullaby/generated/material_def_generated.h"
#include "lullaby/generated/shader_def_generated.h"

namespace lull {

/// TextureUsageInfo holds up to four usages describing how each channel in a
/// texture is used.
class TextureUsageInfo {
 public:
  /// Single usage contructor.
  explicit TextureUsageInfo(MaterialTextureUsage usage) {
    InitFromArray(&usage, 1);
  }

  /// Multi usage constructor.
  explicit TextureUsageInfo(Span<MaterialTextureUsage> usages) {
    InitFromArray(usages.data(), usages.size());
  }

  /// Initializer list constructor.
  explicit TextureUsageInfo(
      std::initializer_list<MaterialTextureUsage> usages) {
    InitFromArray(usages.begin(), usages.size());
  }

  explicit TextureUsageInfo(const ShaderSamplerDefT& sampler) {
    if (sampler.usage_per_channel.empty()) {
      InitFromArray(&sampler.usage, 1);
    } else {
      InitFromArray(sampler.usage_per_channel.data(),
                    sampler.usage_per_channel.size());
    }
  }

  /// Legacy constructor for texture usage by texture unit index.
  explicit TextureUsageInfo(int unit) {
    usages_[0] = static_cast<MaterialTextureUsage>(unit);
  }

  /// Hasher for using TextureUsageInfo in std containers requiring hashes.
  struct Hasher {
    size_t operator()(const TextureUsageInfo& info) const {
      HashValue hash = static_cast<HashValue>(info.usages_[0]);
      hash = HashCombine(hash, static_cast<HashValue>(info.usages_[1]));
      hash = HashCombine(hash, static_cast<HashValue>(info.usages_[2]));
      hash = HashCombine(hash, static_cast<HashValue>(info.usages_[3]));
      return static_cast<size_t>(hash);
    }
  };

  bool operator==(const TextureUsageInfo& rhs) const {
    return usages_[0] == rhs.usages_[0] && usages_[1] == rhs.usages_[1] &&
           usages_[2] == rhs.usages_[2] && usages_[3] == rhs.usages_[3];
  }

  /// Returns the usage for the given channel.
  MaterialTextureUsage GetChannelUsage(int channel) const {
    if (channel >= kMaxTextureChannels) {
      LOG(ERROR) << "Texture channel maximum exceeded";
      return MaterialTextureUsage_Unused;
    }
    return usages_[channel];
  }

  /// Converts the TextureUsageInfo to the hash of a string beginning with
  /// "Texture_" and appending each channel's usage name until the remaining
  /// channels are all set to Unused.
  HashValue GetHash() const {
    /// We search backward through the usage list so usage sets with leading
    /// Unused channels work properly, i.e. {Unused, Roughness, Unused...}.
    int used_count;
    for (used_count = kMaxTextureChannels; used_count > 0; --used_count) {
      if (usages_[used_count - 1] != MaterialTextureUsage_Unused) {
        break;
      }
    }

    HashValue hash = Hash("Texture_");
    for (int i = 0; i < used_count; ++i) {
      hash = Hash(hash, EnumNameMaterialTextureUsage(usages_[i]));
    }

    return hash;
  }

 private:
  enum { kMaxTextureChannels = 4 };

  static bool IsValid(MaterialTextureUsage usage) {
    return usage >= MaterialTextureUsage_MIN &&
           usage <= MaterialTextureUsage_MAX;
  }

  void InitFromArray(const MaterialTextureUsage* array, size_t num) {
    for (size_t i = 0; i < num; ++i) {
      if (!IsValid(array[i])) {
        LOG(DFATAL) << "Invalid usage: " << array[i];
        return;
      }
      if (i >= kMaxTextureChannels) {
        LOG(WARNING) << "Array should provide up to four usages.";
        break;
      }
      usages_[i] = array[i];
    }
  }

  std::array<MaterialTextureUsage, kMaxTextureChannels> usages_ = {
      MaterialTextureUsage_Unused, MaterialTextureUsage_Unused,
      MaterialTextureUsage_Unused, MaterialTextureUsage_Unused};
};

/// Information about the material applied to a draw call.
class MaterialInfo {
 public:
  using TextureInfoMap = std::unordered_map<TextureUsageInfo, std::string,
                                            TextureUsageInfo::Hasher>;

  explicit MaterialInfo(std::string shading_model)
      : shading_model_(std::move(shading_model)) {}

  /// Adds the specified material properties to the material.
  void SetProperties(const VariantMap& properties) {
    for (auto& iter : properties) {
      properties_.emplace(iter.first, std::move(iter.second));
    }
  }

  /// Associates a TextureUsageInfo with a texture name that can be retrieved
  /// from the TextureFactory.
  void SetTexture(TextureUsageInfo usage_info, std::string texture) {
    textures_[usage_info] = std::move(texture);
  }

  /// Associates a texture usage with a texture name that can be retrieved from
  /// the TextureFactory.
  void SetTexture(MaterialTextureUsage usage, std::string texture) {
    SetTexture(TextureUsageInfo(usage), texture);
  }

  /// Sets the Shading Model to be used by the material.
  void SetShadingModel(std::string model) { shading_model_ = std::move(model); }

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

  /// Returns the entire list of material properties.
  const VariantMap& GetProperties() const { return properties_; }

  /// Returns the map from TextureInfo to string.
  const TextureInfoMap& GetTextureInfos() const { return textures_; }

  /// Returns the texture name associated with the given texture usage.
  const std::string& GetTexture(MaterialTextureUsage usage) const {
    auto find = textures_.find(TextureUsageInfo(usage));
    if (find != textures_.end()) {
      return find->second;
    }
    static const std::string empty_string;
    return empty_string;
  }

 private:
  std::string shading_model_;
  VariantMap properties_;
  TextureInfoMap textures_;
};

}  // namespace lull

#endif  // LULLABY_UTIL_MATERIAL_INFO_H_
