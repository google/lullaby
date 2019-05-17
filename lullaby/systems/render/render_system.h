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

#ifndef LULLABY_SYSTEMS_RENDER_RENDER_SYSTEM_H_
#define LULLABY_SYSTEMS_RENDER_RENDER_SYSTEM_H_

#include <memory>
#include <string>

#include "fplbase/render_state.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/render/image_data.h"
#include "lullaby/modules/render/material_info.h"
#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/modules/render/render_view.h"
#include "lullaby/modules/render/vertex.h"
#include "lullaby/systems/render/detail/sort_order.h"
#include "lullaby/systems/render/mesh.h"
#include "lullaby/systems/render/render_target.h"
#include "lullaby/systems/render/render_types.h"
#include "lullaby/systems/render/shader.h"
#include "lullaby/systems/render/texture.h"
#include "lullaby/util/bits.h"
#include "lullaby/generated/material_def_generated.h"
#include "lullaby/generated/render_def_generated.h"
#include "lullaby/generated/render_pass_def_generated.h"
#include "lullaby/generated/render_target_def_generated.h"
#include "lullaby/generated/shader_def_generated.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

class RenderSystemImpl;

// Helper class for differentiating the RenderSystem::Drawable constructors
// that takes a HashValue (for a pass) and an int (for an index).  The index
// is a more "specialized" use-case and so requires the caller to use this
// helper class.
struct DrawableIndex {
  explicit DrawableIndex(unsigned int index) : index(index) {}
  unsigned int index = 0;
};

// The RenderSystem can be used to draw Entities using the GPU.
class RenderSystem : public System {
 public:
  // A useful struct that can be used to specify an Entity, an Entity + pass,
  // an Entity + submesh_index, or an Entity + pass + submesh_index.
  // This allows some functions to apply operations on Components that "match"
  // some combination of the above parameters.  In general, callers don't need
  // to specify Drawable when calling functions that require one.  Instead,
  // they can do:
  //
  //  render_system->Show(entity);
  //  render_system->Show({entity, pass});
  //  render_system->Show({entity, pass, index});
  //  render_system->Show({entity, DrawableIndex(index)});
  struct Drawable {
    Drawable(Entity entity) : entity(entity) {}
    Drawable(Entity entity, HashValue pass) : entity(entity), pass(pass) {}
    Drawable(Entity entity, HashValue pass, int draw_index)
        : entity(entity), pass(pass), index(draw_index) {}
    Drawable(Entity entity, DrawableIndex index)
        : entity(entity), index(index.index) {}
    Drawable(Entity entity, Optional<HashValue> pass, Optional<int> draw_index)
        : entity(entity), pass(pass), index(draw_index) {}

    Entity entity = kNullEntity;
    Optional<HashValue> pass = NullOpt;
    Optional<int> index = NullOpt;
  };

  // Optional parameters that can be used to specialize render system behvaiour.
  struct InitParams {
    bool enable_stereo_multiview = false;

    // Overrides the GL context major version. Intended for use in Emscripten
    // builds where it's not always possible to determine the version from just
    // accessing the GL state.
    Optional<int> gl_major_version_override;
  };

  // A special pass id that allows the RenderSystem to use whatever pass it
  // considers to be the "default" pass.  Users can change this "default" pass
  // explicitly by calling RenderSystem::SetDefaultRenderPass(HashValue pass);
  static const HashValue kDefaultPass = 0xffffffff;

  RenderSystem(Registry* registry, const InitParams& init_params);
  explicit RenderSystem(Registry* registry);
  ~RenderSystem() override;

  /// Initializes inter-system dependencies.
  void Initialize() override;

  /// Sets the render system to draw in stereoscopic multi view mode.
  void SetStereoMultiviewEnabled(bool enabled);

  /// Prepares the render system to render a new frame. This needs to be called
  /// at the beginning of the frame before issuing any render calls.
  void BeginFrame();

  /// This needs to be called at the end of the frame before submitting starting
  /// to draw another frame or calling BeginFrame a second time.
  void EndFrame();

  /// Sets the Render System to begin rendering. This will also attempt to swap
  /// the render data to the latest data submitted via |Submit()|. Must be
  /// called before any render calls are made and finished by calling
  /// |EndRendering()|.
  void BeginRendering();

  /// Sets the Render System to finish the render sequence. This also frees the
  /// render buffer data for writing new data. Must be preceded by
  /// |BeginRendering()|.
  void EndRendering();

  /// Submits the render data buffers to be processed for rendering. Note this
  /// doesn't actually render the data, only makes the buffers ready to be
  /// processed by the render functions.
  void SubmitRenderData();

  /// Renders all objects in |views| for each predefined render pass.
  void Render(const RenderView* views, size_t num_views);

  /// Renders all objects in |views| for the specified |pass|.
  void Render(const RenderView* views, size_t num_views, HashValue pass);

  /// Creates a render target that can be used in a pass for rendering, and as a
  /// texture on top of an object.
  void CreateRenderTarget(HashValue render_target_name,
                          const RenderTargetCreateParams& create_params);

  // Gets the content of the render target on the CPU.
  ImageData GetRenderTargetData(HashValue render_target_name);

  /// Sets the RenderPass value to use when RenderSystem::kDefaultPass is
  /// specified as an argument to a function.
  void SetDefaultRenderPass(HashValue pass);

  /// Returns the RenderPass that is used when RenderSystem::kDefaultPass is
  /// specified as an argument to a function.
  HashValue GetDefaultRenderPass() const;

  /// Sets |pass|'s clear params.
  void SetClearParams(HashValue pass, const RenderClearParams& clear_params);

  /// Sets a render state to be used when rendering a specific render pass. If
  /// a pass is rendered without a state being set, a default render state will
  /// be used.
  void SetRenderState(HashValue pass, const fplbase::RenderState& render_state);

  /// Sets the render target to be used when rendering a specific pass.
  void SetRenderTarget(HashValue pass, HashValue render_target_name);

  /// Sets |pass|'s sort mode.
  void SetSortMode(HashValue pass, SortMode mode);

  /// Sets the |pass|'s sort vector (for WorldSpaceVector** sort modes).
  void SetSortVector(HashValue pass, const mathfu::vec3& vector);

  /// Sets |pass|'s cull mode.
  void SetCullMode(HashValue pass, RenderCullMode mode);

  /// Adds a mesh and corresponding rendering information with the Entity using
  /// the specified ComponentDef.
  void Create(Entity entity, HashValue type, const Def* def) override;

  /// Creates an empty render component for |entity| in |pass|. It is expected
  /// to be populated in code. Does nothing if a render component already exists
  /// for this |pass|. RenderSystemFpl and RenderSystemIon only support one
  /// component per entity, so they will change an existing component to |pass|.
  void Create(Entity entity, HashValue pass);

  /// Performs post creation initialization.
  void PostCreateInit(Entity entity, HashValue type, const Def* def) override;

  /// Disassociates all rendering data from the Entity.
  void Destroy(Entity entity) override;

  /// Disassociates all rendering data identified by |pass| from the Entity.
  void Destroy(Entity entity, HashValue pass);

  /// Returns true if all currently set assets have loaded.
  bool IsReadyToRender(const Drawable& drawable) const;

  /// Executes the callback |fn| when the entity's pass is ready to render.
  using OnReadyToRenderFn = std::function<void()>;
  void OnReadyToRender(const Drawable& drawable,
                       const OnReadyToRenderFn& fn) const;

  /// Returns a list of all render passes in which the |entity| lives.
  std::vector<HashValue> GetRenderPasses(Entity entity) const;

  /// Returns whether |drawable| is hidden or rendering. Will return true for
  /// invalid drawables.
  bool IsHidden(const Drawable& drawable) const;

  /// Stops the rendering of the specified |drawable|.
  void Hide(const Drawable& drawable);

  /// Resumes rendering the specified |drawable|.
  void Show(const Drawable& drawable);

  /// Attaches a mesh to the specified |entity| identified by |pass|.
  void SetMesh(const Drawable& drawable, const MeshData& mesh);
  void SetMesh(const Drawable& drawable, const MeshPtr& mesh);

  /// Retrieves a mesh attached to the specified |entity| identified by
  /// |pass|.
  MeshPtr GetMesh(const Drawable& drawable);

  /// Sets the material (which is a combination of shaders, textures, render
  /// state, etc.) on the specified Entity.
  void SetMaterial(const Drawable& drawable, const MaterialInfo& info);

  // Returns if a shader feature is requested.
  bool IsShaderFeatureRequested(const Drawable& drawable,
                                HashValue feature) const;

  // Request a shader feature for an entity. Features will only be enabled if
  // the shader snippet's prerequisites are available.
  void RequestShaderFeature(const Drawable& drawable, HashValue feature);

  // Clears a single requested shader feature for an entity. The requested
  // feature will be removed and the shader will be reset to a version without
  // the removed feature if possible.
  void ClearShaderFeature(const Drawable& drawable, HashValue feature);

  /// Sets the |data| on the shader uniform of given |type| with the given
  /// |name| on the |drawable|.  The |count| parameter is used to specify
  /// uniform array data.
  void SetUniform(const Drawable& drawable, string_view name,
                  ShaderDataType type, Span<uint8_t> data, int count = 1);

  template <typename T>
  void SetUniform(const Drawable& drawable, string_view name, Span<T> data,
                  int count = 1);

  /// Copies the cached value of the uniform |name| into |data_out|,
  /// respecting the |length| limit. Returns false if the value of the uniform
  /// was not found.
  bool GetUniform(const Drawable& drawable, string_view name, size_t length,
                  uint8_t* data_out) const;

  /// Makes |entity| use all the same uniform values as |source|.
  void CopyUniforms(Entity entity, Entity source);

  /// Sets a callback that is invoked every time SetUniform is called.
  using UniformChangedCallback =
      std::function<void(int submesh_index, string_view name,
                         ShaderDataType type, Span<uint8_t> data, int count)>;
  void SetUniformChangedCallback(Entity entity, HashValue pass,
                                 UniformChangedCallback callback);

  /// Returns |entity|'s default color, as specified in its json.
  const mathfu::vec4& GetDefaultColor(Entity entity) const;

  /// Sets the |entity|'s default color, overriding the color specified in its
  /// json.
  void SetDefaultColor(Entity entity, const mathfu::vec4& color);

  /// Copies the cached value of |entity|'s color uniform into |color|. Returns
  /// false if the value of the uniform was not found.
  bool GetColor(Entity entity, mathfu::vec4* color) const;

  /// Sets the shader's color uniform for the specified |entity|.
  void SetColor(Entity entity, const mathfu::vec4& color);

  /// Attaches a texture to the specified Entity for all passes.
  void SetTexture(const Drawable& drawable, int unit,
                  const TexturePtr& texture);

  /// Returns a pointer to the texture assigned to |entity|'s |unit|.
  TexturePtr GetTexture(const Drawable& drawable, int unit) const;

  /// Attaches a texture object with given GL |texture_target| and |texture_id|
  /// to the specified Entity for all passes.
  void SetTextureId(const Drawable& drawable, int unit, uint32_t texture_target,
                    uint32_t texture_id);

  /// Sets an external texture to the specified Entity for the specified pass.
  /// This is only valid on platforms like mobile that support external
  /// textures.
  void SetTextureExternal(const Drawable& drawable, int unit);

  /// Defines an entity's stencil mode.
  void SetStencilMode(Entity entity, RenderStencilMode mode, int value);
  void SetStencilMode(Entity entity, HashValue pass, RenderStencilMode mode,
                      int value);

  /// Sets the offset used when determining this Entity's draw order.
  void SetSortOrderOffset(Entity entity,
                          RenderSortOrderOffset sort_order_offset);
  void SetSortOrderOffset(Entity entity, HashValue pass,
                          RenderSortOrderOffset sort_order_offset);

  /// Returns |entity|'s sort order offset.
  RenderSortOrderOffset GetSortOrderOffset(Entity entity) const;

  /// Returns |entity|'s sort order.
  RenderSortOrder GetSortOrder(Entity entity) const;

  /// IMMEDIATE MODE RENDERING.

  /// Immediately binds |shader|.
  void BindShader(const ShaderPtr& shader);

  /// Immediately binds |texture| in |unit|.
  void BindTexture(int unit, const TexturePtr& texture);

  /// Immediately binds |uniform| on the currently bound shader.
  void BindUniform(const char* name, const float* data, int dimension);

  /// Enables depth testing.
  void SetDepthTest(bool enabled);

  /// Enables depth writing.
  void SetDepthWrite(bool enabled);

  /// Sets the blend mode.
  void SetBlendMode(fplbase::BlendMode blend_mode);

  /// Sets |view| to be the screen rectangle that gets rendered to.
  void SetViewport(const RenderView& view);

  /// Immediately draws |mesh| using the clip from model transform.
  void DrawMesh(const MeshData& mesh,
                Optional<mathfu::mat4> clip_from_model = NullOpt);

  /// EDITOR ONLY. Do not use in production.

  /// EDITOR ONLY: Returns the shader string used by an entity.
  std::string GetShaderString(Entity entity, HashValue pass, int submesh_index,
                              ShaderStageType stage) const;

  /// EDITOR ONLY: Compiles a shader string into a shader.
  ShaderPtr CompileShaderString(const std::string& vertex_string,
                                const std::string& fragment_string);

  /// Returns the underlying RenderSystemImpl (eg. RenderSystemFpl,
  /// RenderSystemIon) to expose implementation-specific behaviour. Th
  /// RenderSystemImpl header which is used must match the render system that is
  /// depended upon in the BUILD rule.
  RenderSystemImpl* GetImpl();

  /// IMPORTANT: The following legacy functions are deprecated.
  #include "lullaby/systems/render/render_system_deprecated.inc"

 private:
  std::unique_ptr<RenderSystemImpl> impl_;
};

template <typename T>
inline ShaderDataType GetShaderDataType() {
  CHECK(false);
  return ShaderDataType_MAX;
}

template <>
inline ShaderDataType GetShaderDataType<int>() {
  return ShaderDataType_Int1;
}

template <>
inline ShaderDataType GetShaderDataType<float>() {
  return ShaderDataType_Float1;
}

template <typename T>
inline void RenderSystem::SetUniform(const Drawable& drawable, string_view name,
                                     Span<T> data, int count) {
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.data());
  const size_t num_bytes = data.size() * sizeof(T);
  SetUniform(drawable, name, GetShaderDataType<T>(), {bytes, num_bytes}, count);
}

struct SetColorEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&color, ConstHash("color"));
    archive(&int_argb, ConstHash("int_argb"));
  }

  Entity entity = kNullEntity;
  mathfu::vec4 color = mathfu::kZeros4f;
  int int_argb = 0;
};

struct SetDefaultColorEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&color, ConstHash("color"));
    archive(&int_argb, ConstHash("int_argb"));
  }

  Entity entity = kNullEntity;
  mathfu::vec4 color = mathfu::kZeros4f;
  int int_argb = 0;
};

struct SetTextureIdEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&texture_target, ConstHash("texture_target"));
    archive(&texture_id, ConstHash("texture_id"));
  }

  Entity entity = kNullEntity;
  int32_t texture_target = 0;
  int32_t texture_id = 0;
};

struct SetTextureEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&filename, ConstHash("filename"));
  }

  Entity entity = kNullEntity;
  std::string filename;
};

struct SetImageEvent {
  Entity entity = kNullEntity;
  std::string id;
  std::shared_ptr<ImageData> image;
  bool create_mips = false;
};


struct SetSortOffsetEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&sort_offset, ConstHash("sort_offset"));
  }

  Entity entity = kNullEntity;
  int32_t sort_offset = 0;
};

struct SetRenderPassEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&render_pass, ConstHash("render_pass"));
  }

  Entity entity = kNullEntity;
  int32_t render_pass = -1;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::RenderSystem);
LULLABY_SETUP_TYPEID(lull::SetColorEvent);
LULLABY_SETUP_TYPEID(lull::SetDefaultColorEvent);
LULLABY_SETUP_TYPEID(lull::SetImageEvent);
LULLABY_SETUP_TYPEID(lull::SetRenderPassEvent);
LULLABY_SETUP_TYPEID(lull::SetSortOffsetEvent);
LULLABY_SETUP_TYPEID(lull::SetTextureEvent);
LULLABY_SETUP_TYPEID(lull::SetTextureIdEvent);

#endif  // LULLABY_SYSTEMS_RENDER_RENDER_SYSTEM_H_
