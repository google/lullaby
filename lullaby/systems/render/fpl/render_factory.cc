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

#include "lullaby/systems/render/fpl/render_factory.h"

#include "fplbase/glplatform.h"
#include "fplbase/internal/type_conversions_gl.h"
#include "fplbase/render_utils.h"
#include "fplbase/utilities.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/file/file.h"
#include "lullaby/systems/render/fpl/shader.h"
#include "lullaby/systems/render/fpl/texture.h"
#include "lullaby/util/math.h"
#include "lullaby/util/trace.h"

namespace lull {
namespace {
constexpr const char* kFallbackVS =
    "attribute vec4 aPosition;\n"
    "uniform mat4 model_view_projection;\n"
    "void main() {\n"
    "  gl_Position = model_view_projection * aPosition;\n"
    "}";

constexpr const char* kFallbackFS =
    "uniform lowp vec4 color;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(color.rgb * color.a, color.a);\n"
    "}\n";

fplbase::TextureFlags GetTextureFlags(bool create_mips, bool async = false,
                                      bool is_cubemap = false,
                                      bool premultiply_alpha = true) {
  fplbase::TextureFlags none = fplbase::kTextureFlagsNone;
  return ((create_mips ? fplbase::kTextureFlagsUseMipMaps : none) |
          (is_cubemap ? (fplbase::kTextureFlagsIsCubeMap |
                         fplbase::kTextureFlagsClampToEdge)
                      : none) |
          (async ? fplbase::kTextureFlagsLoadAsync : none) |
          (premultiply_alpha ? fplbase::kTextureFlagsPremultiplyAlpha : none));
}

fplbase::Texture* CreateFplTextureFromMemory(const void* data,
                                             const mathfu::vec2i& size,
                                             fplbase::TextureFormat format,
                                             fplbase::TextureFlags flags) {
  fplbase::Texture* texture = new fplbase::Texture(nullptr, format, flags);
  texture->LoadFromMemory(static_cast<const uint8_t*>(data), size, format);
  return texture;
}

fplbase::Texture* CreateWhiteTexture() {
  constexpr int kTextureSize = 2;
  const Color4ub data[kTextureSize * kTextureSize];
  return CreateFplTextureFromMemory(
      data, mathfu::vec2i(kTextureSize, kTextureSize), fplbase::kFormat8888,
      fplbase::kTextureFlagsNone);
}

fplbase::Texture* CreateWatermelonTexture() {
  constexpr int kTextureSize = 16;
  const Color4ub kUglyGreen(0, 255, 0, 255);
  const Color4ub kUglyPink(255, 0, 128, 255);
  Color4ub data[kTextureSize * kTextureSize];
  Color4ub* ptr = data;
  for (int y = 0; y < kTextureSize; ++y) {
    for (int x = 0; x < kTextureSize; ++x, ++ptr) {
      *ptr = ((x + y) % 2 == 0) ? kUglyGreen : kUglyPink;
    }
  }
  return CreateFplTextureFromMemory(
      data, mathfu::vec2i(kTextureSize, kTextureSize), fplbase::kFormat8888,
      fplbase::kTextureFlagsNone);
}

}  // namespace

RenderFactory::RenderFactory(Registry* registry, fplbase::Renderer* renderer)
    : registry_(registry),
      fpl_renderer_(renderer),
      fpl_asset_manager_(std::make_shared<fplbase::AssetManager>(*renderer)) {
  fpl_asset_manager_->StartLoadingTextures();

  fplbase::Texture* white_fpl_texture = CreateWhiteTexture();
  white_texture_.reset(new Texture(Texture::TextureImplPtr(
      white_fpl_texture, [](const fplbase::Texture* tex) { delete tex; })));

#ifdef DEBUG
  invalid_fpl_texture_ = CreateWatermelonTexture();
  invalid_texture_.reset(new Texture(Texture::TextureImplPtr(
      invalid_fpl_texture_, [](const fplbase::Texture* tex) { delete tex; })));
#else
  invalid_fpl_texture_ = white_fpl_texture;
  invalid_texture_ = white_texture_;
#endif  // DEBUG
}

MeshPtr RenderFactory::LoadMesh(const std::string& filename) {
  const HashValue key = Hash(filename.c_str());
  return meshes_.Create(
      key, [&]() { return MeshPtr(new Mesh(LoadFplMesh(filename))); });
}

ShaderPtr RenderFactory::LoadShader(const std::string& filename) {
  const HashValue key = Hash(filename.c_str());
  return shaders_.Create(key, [&]() {
    return ShaderPtr(new Shader(fpl_renderer_, LoadFplShader(filename)));
  });
}

bool RenderFactory::IsTextureValid(const TexturePtr& texture) const {
  return (texture && fplbase::ValidTextureHandle(texture->GetResourceId()));
}

TexturePtr RenderFactory::LoadTexture(const std::string& filename,
                                      bool create_mips) {
  const HashValue key = Hash(filename.c_str());
  TexturePtr texture = textures_.Create(key, [&]() {
    return TexturePtr(new Texture(LoadFplTexture(filename, create_mips)));
  });
  if (texture->HasMips() != create_mips) {
    LOG(WARNING) << "Texture mip conflict on " << filename << ": has? "
                 << texture->HasMips() << ", wants? " << create_mips;
  }
  return texture;
}

void RenderFactory::LoadTextureAtlas(const std::string& filename,
                                     bool create_mips) {
  const HashValue key = Hash(filename.c_str());
  textures_.Create(key, [&]() {
    return TexturePtr(new Texture(LoadFplTextureAtlas(filename, create_mips)));
  });
}

TexturePtr RenderFactory::CreateTextureFromMemory(const void* data,
                                                  const mathfu::vec2i size,
                                                  fplbase::TextureFormat format,
                                                  bool create_mips) {
  const bool async = false;
  const bool is_cubemap = false;
  const auto flags = GetTextureFlags(create_mips, async, is_cubemap);
  fplbase::Texture* texture =
      CreateFplTextureFromMemory(data, size, format, flags);
  return TexturePtr(new Texture(Texture::TextureImplPtr(
      texture, [](const fplbase::Texture* texture) { delete texture; })));
}


TexturePtr RenderFactory::CreateProcessedTexture(
    const TexturePtr& texture, bool create_mips,
    const RenderSystem::TextureProcessor& processor,
    const mathfu::vec2i& output_dimensions) {
  LULLABY_CPU_TRACE_CALL();

  if (!texture) {
    LOG(DFATAL) << "null texture passed to CreateProcessedTexture()";
    return TexturePtr();
  }
  // Make and bind a framebuffer for rendering to texture.
  GLuint framebuffer_id, current_framebuffer_id;
  GL_CALL(glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,
                        reinterpret_cast<GLint *>(&current_framebuffer_id)));
  GL_CALL(glGenFramebuffers(1, &framebuffer_id));
  GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id));

  // Make an empty FPL texture for the render target, sized to the specified
  // dimensions.
  mathfu::vec2i size = output_dimensions;
  bool target_is_subtexture = false;
  float texture_u_bound = 1.f;
  float texture_v_bound = 1.f;

  // If the input texture is a subtexture, scale the size appropriately.
  if (texture->IsSubtexture()) {
    auto scale = mathfu::vec2(texture->UvBounds().z, texture->UvBounds().w);
    auto size_f = scale * mathfu::vec2(static_cast<float>(size.x),
                                       static_cast<float>(size.y));
    size =
        mathfu::vec2i(static_cast<int>(size_f.x), static_cast<int>(size_f.y));
  }

  // If we don't support NPOT and the texture is NPOT, use UV bounds to work
  // around this.
  if (!fpl_renderer_->SupportsTextureNpot() &&
      (!IsPowerOf2(size.x) || !IsPowerOf2(size.y))) {
    target_is_subtexture = true;
    uint32_t next_power_of_two_x =
        mathfu::RoundUpToPowerOf2(static_cast<uint32_t>(size.x));
    uint32_t next_power_of_two_y =
        mathfu::RoundUpToPowerOf2(static_cast<uint32_t>(size.y));
    texture_u_bound =
        static_cast<float>(size.x) / static_cast<float>(next_power_of_two_x);
    texture_v_bound =
        static_cast<float>(size.y) / static_cast<float>(next_power_of_two_y);
    size = mathfu::vec2i(next_power_of_two_x, next_power_of_two_y);
  }

  Texture::TextureImplPtr out = CreateFplTexture(size, create_mips);
  TexturePtr out_ptr;
  if (target_is_subtexture) {
    const mathfu::vec4 bounds(0.f, 0.f, texture_u_bound, texture_v_bound);
    out_ptr = TexturePtr(new Texture(std::move(out), bounds));
  } else {
    out_ptr = TexturePtr(new Texture(std::move(out)));
  }

  // Bind the output texture to the framebuffer as the color attachment.
  GL_CALL(glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
      fplbase::GlTextureHandle(out_ptr->GetResourceId()), 0));

#if defined(DEBUG)
  // Check for completeness of the framebuffer.
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    LOG(DFATAL) << "Failed to create offscreen framebuffer: " << std::hex
                 << glCheckFramebufferStatus(GL_FRAMEBUFFER);
  }
#endif

  // Subtexturing on output texture can pick up sample noise around the edges
  // of the rendered area. Clear to transparent black.
  if (target_is_subtexture) {
    GL_CALL(glClearColor(0.f, 0.f, 0.f, 0.f));
    GL_CALL(glClear(GL_COLOR_BUFFER_BIT));
  }

  processor(out_ptr);

  // Setup viewport, input texture, shader, and draw quad.
  fpl_renderer_->SetViewport(fplbase::Viewport(mathfu::kZeros2i, size));

  // We render a quad starting in the lower left corner and extending up and
  // right for as long as the output subtexture is needed.
  fplbase::RenderAAQuadAlongX(
      mathfu::vec3(-1.f, -1.f, 0.f),
      mathfu::vec3((texture_u_bound * 2.f) - 1.f, (texture_v_bound * 2.f) - 1.f,
                   0),
      mathfu::vec2(texture->UvBounds().x, texture->UvBounds().y),
      mathfu::vec2(texture->UvBounds().x + texture->UvBounds().z,
                   texture->UvBounds().y + texture->UvBounds().w));

  // Delete framebuffer, we retain the texture.
  GL_CALL(glDeleteFramebuffers(1, &framebuffer_id));

  // Regenerate Mipmaps on the processed texture.
  if (create_mips) {
    out_ptr->Bind(0);
    GL_CALL(glGenerateMipmap(GL_TEXTURE_2D));
  }

  GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, current_framebuffer_id));

  return out_ptr;
}

TexturePtr RenderFactory::CreateProcessedTexture(
    const TexturePtr& source_texture, bool create_mips,
    const RenderSystem::TextureProcessor& processor) {
  auto size = source_texture->GetDimensions();
  return CreateProcessedTexture(source_texture, create_mips, processor, size);
}

TexturePtr RenderFactory::CreateTexture(uint32_t texture_target,
                                        uint32_t texture_id) {
  return TexturePtr(new Texture(texture_target, texture_id));
}


void RenderFactory::UpdateAssetLoad() { fpl_asset_manager_->TryFinalize(); }

void RenderFactory::WaitForAssetsToLoad() {
  while (!fpl_asset_manager_->TryFinalize()) {
  }
}

Mesh::MeshImplPtr RenderFactory::LoadFplMesh(const std::string& name) {
  auto mesh = fpl_asset_manager_->LoadMesh(name.c_str());
  if (!mesh) {
    LOG(ERROR) << "Could not load mesh: " << name;
    return nullptr;
  }

  std::weak_ptr<fplbase::AssetManager> asset_manager_weak = fpl_asset_manager_;
  return Mesh::MeshImplPtr(
      mesh, [asset_manager_weak, name](const fplbase::Mesh*) {
        if (auto asset_manager = asset_manager_weak.lock()) {
          asset_manager->UnloadMesh(name.c_str());
        }
      });
}

Shader::ShaderImplPtr RenderFactory::LoadFplShader(const std::string& name) {
  auto shader = fpl_asset_manager_->LoadShaderDef(name.c_str());
  if (!shader) {
    LOG(DFATAL) << "Could not load shader: " << name;
    shader = fpl_renderer_->CompileAndLinkShader(kFallbackVS, kFallbackFS);
  }

  std::weak_ptr<fplbase::AssetManager> asset_manager_weak = fpl_asset_manager_;
  return Shader::ShaderImplPtr(
      shader, [asset_manager_weak, name](const fplbase::Shader*) {
        if (auto asset_manager = asset_manager_weak.lock()) {
          asset_manager->UnloadShader(name.c_str());
        }
      });
}

Texture::TextureImplPtr RenderFactory::LoadFplTexture(const std::string& name,
                                                      bool create_mips) {
  const bool async = true;
  // TODO(b/29898942) proper cubemap detection
  const bool is_cubemap = (name.find("cubemap") != std::string::npos);
  const bool is_nopremult = (name.find("nopremult") != std::string::npos);
  fplbase::Texture* texture = fpl_asset_manager_->LoadTexture(
      name.c_str(), fplbase::kFormatNative,
      GetTextureFlags(create_mips, async, is_cubemap, !is_nopremult));
  if (!texture) {
    // We'll never hit this.  Texture is always created since async is true.
    LOG(ERROR) << "Could not load texture: " << name;
    return Texture::TextureImplPtr(invalid_fpl_texture_,
                                   [](const fplbase::Texture*) {});
  }

  std::weak_ptr<fplbase::AssetManager> asset_manager_weak = fpl_asset_manager_;
  return Texture::TextureImplPtr(
      texture, [asset_manager_weak, name](const fplbase::Texture*) {
        if (auto asset_manager = asset_manager_weak.lock()) {
          asset_manager->UnloadTexture(name.c_str());
        }
      });
}

Texture::AtlasImplPtr RenderFactory::LoadFplTextureAtlas(
    const std::string& name, bool create_mips) {
  auto atlas = fpl_asset_manager_->LoadTextureAtlas(
      name.c_str(), fplbase::kFormatNative, GetTextureFlags(create_mips));
  if (!atlas) {
    // This will be hit when the flatbuffer file isn't valid.
    LOG(ERROR) << "Could not load atlas: " << name;
    return nullptr;
  }

  // Push all the subtextures in the texture atlas into our asset manager.
  auto fpl_texture = atlas->atlas_texture();
  const auto& map = atlas->index_map();
  const auto& bounds = atlas->subtexture_bounds();
  for (const auto& iter : map) {
    const mathfu::vec4 uvs = bounds[iter.second];
    const HashValue key = Hash(iter.first.c_str());
    textures_.Create(key, [&]() {
      Texture::TextureImplPtr ptr(fpl_texture, [](const fplbase::Texture*) {
        // Do nothing.  The texture is owned by the atlas.
      });
      return TexturePtr(new Texture(std::move(ptr), uvs));
    });
  }

  std::weak_ptr<fplbase::AssetManager> asset_manager_weak = fpl_asset_manager_;
  return Texture::AtlasImplPtr(
      atlas, [asset_manager_weak, name](const fplbase::TextureAtlas*) {
        if (auto asset_manager = asset_manager_weak.lock()) {
          asset_manager->UnloadTextureAtlas(name.c_str());
        }
      });
}

Texture::TextureImplPtr RenderFactory::CreateFplTexture(
    const mathfu::vec2i& size, bool create_mips) {
  const fplbase::TextureFormat format = fplbase::kFormat8888;
  Texture::TextureImplPtr out(
      new fplbase::Texture(nullptr, format, GetTextureFlags(create_mips)),
      [](const fplbase::Texture* tex) { delete tex; });
  out->LoadFromMemory(nullptr, size, format);
  return out;
}

MeshPtr RenderFactory::CreateMesh(const MeshData& mesh) {
  if (mesh.GetNumVertices() == 0) {
    return MeshPtr();
  }
  return MeshPtr(new Mesh(mesh));
}

void RenderFactory::StartLoadingAssets() {
  fpl_asset_manager_->StartLoadingTextures();
}

void RenderFactory::StopLoadingAssets() {
  fpl_asset_manager_->StopLoadingTextures();
}

}  // namespace lull
