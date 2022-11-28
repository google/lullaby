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

#ifndef REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_SHADER_H_
#define REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_SHADER_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "filament/Material.h"
#include "filament/MaterialInstance.h"
#include "redux/data/asset_defs/shader_asset_def_generated.h"
#include "redux/engines/render/filament/filament_texture.h"
#include "redux/engines/render/filament/filament_utils.h"
#include "redux/engines/render/shader.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/graphics/enums.h"
#include "redux/modules/graphics/texture_usage.h"

namespace redux {

// Defines how the "surface" of a renderable will be "colored in".
//
// A FilamentShader consists of multiple variants, each of which is basically a
// filament::Material. Each variant supports a set of features (e.g. skinning)
// and depends on a set of conditions (i.e. bone weights/indices vertex
// attributes).
class FilamentShader : public Shader {
 public:
  FilamentShader(Registry* registry, const ShaderAssetDef* def);

  // Internally, variants are stored in an array, and this is simply the index
  // of the variant in that array.
  using VariantId = int;
  static constexpr VariantId kInvalidVariant = -1;

  // Finds the best matching variant given the `features` and `conditions`.
  using FlagSet = absl::btree_set<HashValue>;
  VariantId DetermineVariantId(const FlagSet& conditions,
                               const FlagSet& features) const;

  // Returns the filament::Material for the given variant.
  const filament::Material* GetFilamentMaterial(VariantId id) const;

  // Information about a single parameter in a material.
  struct ParameterInfo {
    std::string name;
    HashValue key;
    MaterialPropertyType type;
    TextureUsage texture_usage;
  };

  // Iterates over all the parameters in the material for the variant.
  using ForParameterFn = std::function<void(const ParameterInfo&)>;
  void ForEachParameter(VariantId id, const ForParameterFn& fn);

  // Sets the value of the parameter with the given `name` in the
  // MaterialInstance based on the `type`.
  static void SetParameter(filament::MaterialInstance* instance,
                           const char* name, MaterialPropertyType type,
                           absl::Span<const std::byte> data);

  // Sets the value of the texture sampler with the given `name` in the
  // MaterialInstance based on the `type`.
  static void SetParameter(filament::MaterialInstance* instance,
                           const char* name, MaterialPropertyType type,
                           const TexturePtr& texture);

 private:
  struct Variant {
    FilamentResourcePtr<filament::Material> fmaterial;
    std::vector<ParameterInfo> params;
    FlagSet conditions;
    FlagSet features;
  };

  void BuildVariant(const ShaderVariantAssetDef* def);

  filament::Engine* fengine_ = nullptr;
  std::vector<std::unique_ptr<Variant>> variants_;
  absl::flat_hash_map<HashValue, size_t> cache_;
};

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_SHADER_H_
