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

#ifndef LULLABY_SYSTEMS_RENDER_FPL_RENDER_FACTORY_H_
#define LULLABY_SYSTEMS_RENDER_FPL_RENDER_FACTORY_H_

#include <map>
#include <memory>
#include <vector>

#include "fplbase/asset_manager.h"
#include "fplbase/renderer.h"
#include "fplbase/texture_atlas.h"
#include "lullaby/systems/render/fpl/mesh.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/resource_manager.h"
#include "lullaby/systems/render/fpl/shader.h"
#include "lullaby/systems/render/fpl/texture.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/util/typeid.h"

namespace lull {

// The RenderFactory is used to create Render objects like Meshes, Textures and
// Shaders.
class RenderFactory {
 public:
  RenderFactory(Registry* registry, fplbase::Renderer* renderer);

  RenderFactory(const RenderFactory&) = delete;
  RenderFactory& operator=(const RenderFactory&) = delete;

  // Returns a resident white texture with an alpha channel: (1, 1, 1, 1).
  const TexturePtr& GetWhiteTexture() const { return white_texture_; }

  // Returns a resident invalid texture to be used when a requested image fails
  // to load.  On debug builds it's a watermelon; on release builds it's just
  // the white texture.
  const TexturePtr& GetInvalidTexture() const { return invalid_texture_; }

  // Queries if |texture| was successfully loaded.
  bool IsTextureValid(const TexturePtr& texture) const;

  // Loads the mesh with the given |filename|. The mesh is automatically cached.
  MeshPtr LoadMesh(const std::string& filename);

  // Loads the shader with the given |filename|. The shader is automatically
  // cached.
  ShaderPtr LoadShader(const std::string& filename);

  // Loads the texture with the given |filename| and optionally creates mips.
  // The texture is automatically cached.
  TexturePtr LoadTexture(const std::string& filename, bool create_mips);

  // Loads the texture atlas with the given |filename| and optionally creates
  // mips. The atlas is automatically cached.
  void LoadTextureAtlas(const std::string& filename, bool create_mips);

  // Creates a mesh using the specified data.
  MeshPtr CreateMesh(const MeshData& mesh);

  // Creates and caches a named mesh using the specified data.
  MeshPtr CreateMesh(HashValue key, const MeshData& mesh);

  // Creates a texture from memory.  |data| is copied into GL memory, so it's no
  // longer needed after calling this function.
  TexturePtr CreateTextureFromMemory(const void* data, const mathfu::vec2i size,
                                     fplbase::TextureFormat format,
                                     bool create_mips);


  // Create and return a pre-processed texture.  This will set up a rendering
  // environment suitable to render |sourcE_texture| with a pre-process shader.
  // texture and shader binding / setup should be performed in |processor|.
  TexturePtr CreateProcessedTexture(
      const TexturePtr& source_texture, bool create_mips,
      const RenderSystem::TextureProcessor& processor);

  // Create and return a pre-processed texture as above, but size the output
  // according to |output_dimensions|.
  TexturePtr CreateProcessedTexture(
      const TexturePtr& texture, bool create_mips,
      const RenderSystem::TextureProcessor& processor,
      const mathfu::vec2i& output_dimensions);

  // Creates a texture from specified GL |texture_target| and |texture_id|.
  TexturePtr CreateTexture(uint32_t texture_target, uint32_t texture_id);


  // Attempts to Finalize the load of a single asset.
  void UpdateAssetLoad();

  // Waits for all outstanding rendering assets to finish loading.
  void WaitForAssetsToLoad();

  // Start loading assets asynchronously.
  void StartLoadingAssets();

  // Pause loading assets asynchronously.
  void StopLoadingAssets();

  // Releases the cached mesh associated with |key|.
  void ReleaseMeshFromCache(HashValue key);

  // Caches a texture for later retrieval.
  void CacheTexture(HashValue key, const TexturePtr& texture);

  // Retrieves a cached texture by its name hash. If the texture isn't cached
  // this returns nullptr.
  TexturePtr GetCachedTexture(HashValue key) const;

  // Releases the cached texture associated with |key|.
  void ReleaseTextureFromCache(HashValue key);

 private:
  Mesh::MeshImplPtr LoadFplMesh(const std::string& name);
  Shader::ShaderImplPtr LoadFplShader(const std::string& name);
  Texture::TextureImplPtr LoadFplTexture(const std::string& name,
                                         bool create_mips);
  Texture::AtlasImplPtr LoadFplTextureAtlas(const std::string& name,
                                            bool create_mips);
  Texture::TextureImplPtr CreateFplTexture(const mathfu::vec2i& size,
                                           bool create_mips);

  Registry* registry_;
  ResourceManager<Mesh> meshes_;
  ResourceManager<Texture> textures_;
  ResourceManager<Shader> shaders_;

  fplbase::Renderer* fpl_renderer_;
  std::shared_ptr<fplbase::AssetManager> fpl_asset_manager_;
  TexturePtr white_texture_;
  fplbase::Texture* invalid_fpl_texture_;
  TexturePtr invalid_texture_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::RenderFactory);

#endif  // LULLABY_SYSTEMS_RENDER_FPL_RENDER_FACTORY_H_
