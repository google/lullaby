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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_RENDER_FACTORY_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_RENDER_FACTORY_H_

#include "fplbase/asset_manager.h"
#include "fplbase/renderer.h"
#include "lullaby/systems/render/next/mesh_factory.h"
#include "lullaby/systems/render/next/shader_factory.h"
#include "lullaby/systems/render/next/texture_atlas_factory.h"
#include "lullaby/systems/render/next/texture_factory.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/typeid.h"

namespace lull {

// Creates render objects like Meshes, Textures and Shaders.
//
// Most of the work is actually delegated to individual underlying factories:
// MeshFactory, ShaderFactory, and TextureFactory.
class RenderFactory {
 public:
  RenderFactory(Registry* registry, fplbase::Renderer* renderer);

  RenderFactory(const RenderFactory&) = delete;
  RenderFactory& operator=(const RenderFactory&) = delete;

  // DEPRECATED: Use TextureFactory.
  const TexturePtr& GetWhiteTexture() const;

  // DEPRECATED: Use TextureFactory.
  const TexturePtr& GetInvalidTexture() const;

  // Queries if |texture| was successfully loaded.
  bool IsTextureValid(const TexturePtr& texture) const;

  // DEPRECATED: Use MeshFactory.
  MeshPtr LoadMesh(const std::string& filename);

  // DEPRECATED: Use ShaderFactory.
  ShaderPtr LoadShader(const std::string& filename);

  // DEPRECATED: Use TextureFactory.
  TexturePtr LoadTexture(const std::string& filename, bool create_mips);

  // DEPRECATED: Use TextureFactory.
  TexturePtr GetCachedTexture(HashValue texture_hash);

  // DEPRECATED: Use TextureFactory.
  void LoadTextureAtlas(const std::string& filename, bool create_mips);

  // DEPRECATED: Use MeshFactory.
  MeshPtr CreateMesh(const MeshData& mesh);

  // DEPRECATED: Use MeshFactory.
  MeshPtr CreateMesh(HashValue key, const MeshData& mesh);

  // DEPRECATED: Use TextureFactory.
  TexturePtr CreateTextureFromMemory(const void* data, const mathfu::vec2i size,
                                     fplbase::TextureFormat format,
                                     bool create_mips);


  // DEPRECATED: Use TextureFactory.
  TexturePtr CreateProcessedTexture(
      const TexturePtr& source_texture, bool create_mips,
      const RenderSystem::TextureProcessor& processor);

  // DEPRECATED: Use TextureFactory.
  TexturePtr CreateProcessedTexture(
      const TexturePtr& texture, bool create_mips,
      const RenderSystem::TextureProcessor& processor,
      const mathfu::vec2i& output_dimensions);

  // DEPRECATED: Use TextureFactory.
  TexturePtr CreateTexture(uint32_t texture_target, uint32_t texture_id);

  // DEPRECATED: Use TextureFactory.
  void CacheTexture(HashValue name, const TexturePtr& texture);

  // Attempts to Finalize the load of a single asset.
  void UpdateAssetLoad();

  // Waits for all outstanding rendering assets to finish loading.
  void WaitForAssetsToLoad();

  // Start loading assets asynchronously.
  void StartLoadingAssets();

  // Pause loading assets asynchronously.
  void StopLoadingAssets();

 private:
  std::shared_ptr<fplbase::AssetManager> fpl_asset_manager_;

  MeshFactory* mesh_factory_;
  ShaderFactory* shader_factory_;
  TextureFactory* texture_factory_;
  TextureAtlasFactory* texture_atlas_factory_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::RenderFactory);

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_RENDER_FACTORY_H_
