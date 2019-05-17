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

#ifndef LULLABY_SYSTEMS_RENDER_FILAMENT_SHADER_FACTORY_H_
#define LULLABY_SYSTEMS_RENDER_FILAMENT_SHADER_FACTORY_H_

#include "lullaby/modules/file/asset.h"
#include "lullaby/modules/render/shader_snippets_selector.h"
#include "lullaby/systems/render/filament/shader.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/resource_manager.h"
#include "lullaby/util/string_view.h"

namespace lull {

class ShaderMaterialBuilder;

// Creates and manages Shader objects.
//
// Shaders will be automatically released along with the last external
// reference unless they are explicitly cached.
class ShaderFactory final {
 public:
  ShaderFactory(Registry* registry, filament::Engine* engine);
  ShaderFactory(const ShaderFactory&) = delete;
  ShaderFactory& operator=(const ShaderFactory&) = delete;
  ~ShaderFactory();

  // Sets the path where shader assets will be loaded when only the shading
  // model is known.
  void SetShadingModelPath(string_view path);

  // Creates a shader using the provided |shader_name| which is either a
  // filename to a resource on disk (eg. matc, lullshader) or a shading model.
  // Shading models are automatically resolved to filenames.
  ShaderPtr CreateShader(string_view shader_name,
                         const ShaderSelectionParams& params = {});

  // Returns the shader in the cache associated with |key|, else nullptr.
  ShaderPtr GetCachedShader(HashValue key) const;

  // Attempts to add |shader| to the cache using |key|.
  void CacheShader(HashValue key, const ShaderPtr& shader);

  // Releases the cached shader associated with |key|.
  void ReleaseShaderFromCache(HashValue key);

 private:
  // Shader assets are either lullshaders or filament matc binaries.
  enum AssetType {
    kUnknown,
    kLullShader,
    kFilamentMatc,
  };

  // A shader asset loaded off disk. The binary data itself is stored in the
  // base "SimpleAsset" class. This class just adds the shading model and asset
  // type as meta data.
  struct ShaderAsset : public SimpleAsset {
    std::string model;
    AssetType type = kUnknown;
  };

  using ShaderAssetPtr = std::shared_ptr<ShaderAsset>;

  // Uses the given shader material builder to create a Shader object with the
  // specified key if it does not already exist.
  ShaderPtr BuildShader(HashValue key, ShaderMaterialBuilder* builder);

  // Finds the shader asset off disk with the given filename.  Will fallback to
  // trying alternative files (eg. matc, lullshader) using the basename of the
  // given filename.
  ShaderAssetPtr FindShaderAsset(string_view shader_name);

  // Loads the asset with the specified filename from disk.  If successful, will
  // add the model and type to the ShaderAsset metadata.
  ShaderAssetPtr LoadShaderAsset(string_view filename, string_view model,
                                 AssetType type);

  Registry* registry_;
  filament::Engine* engine_;
  ResourceManager<Shader> shaders_;
  ResourceManager<ShaderAsset> assets_;
  std::string shading_model_path_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ShaderFactory);

#endif  // LULLABY_SYSTEMS_RENDER_FILAMENT_SHADER_FACTORY_H_
