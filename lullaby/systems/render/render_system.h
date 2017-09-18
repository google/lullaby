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

#ifndef LULLABY_SYSTEMS_RENDER_RENDER_SYSTEM_H_
#define LULLABY_SYSTEMS_RENDER_RENDER_SYSTEM_H_

#include <memory>
#include <string>

#include "fplbase/renderer.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/modules/render/render_view.h"
#include "lullaby/modules/render/triangle_mesh.h"
#include "lullaby/modules/render/vertex.h"
#include "lullaby/systems/render/material.h"
#include "lullaby/systems/render/mesh.h"
#include "lullaby/systems/render/shader.h"
#include "lullaby/systems/render/texture.h"
#include "lullaby/systems/render/uniform.h"
#include "lullaby/systems/text/html_tags.h"
#include "lullaby/systems/text/text_system.h"
#include "lullaby/generated/render_def_generated.h"
#include "lullaby/generated/render_target_def_generated.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

class Mesh;
using MeshPtr = std::shared_ptr<Mesh>;

/// A pair of an Entity and a HashValue id.
union EntityIdPair {
  struct {
    /// An entity associated with this pair.
    Entity entity;
    /// A unique id used to differentiate multiple components associated with a
    /// given entity.
    HashValue id;
  };
  /// The combined bit value of the union, used for hashing and indexing the
  /// entity id pair. Entity and HashValue are both 32-bit ints, and value is
  /// their unified 64-bit value.
  uint64_t value;

  EntityIdPair(Entity e, HashValue id = 0)
      : entity(e), id(id) {}
  bool operator==(const EntityIdPair& other) const {
    return value == other.value;
  }
};

struct EntityIdPairHash {
  size_t operator()(const EntityIdPair& entity_id_pair) const {
    return std::hash<uint64_t>{}(entity_id_pair.value);
  }
};

enum class StencilMode {
  kDisabled,
  kTest,
  kWrite,
};

class RenderSystemImpl;

// The RenderSystem can be used to provide Entities with FPL Shader and Mesh
// objects which can be used for rendering purposes.
class RenderSystem : public System {
 public:
  using View = RenderView;

  struct Quad {
    Quad()
        : id(0),
          size(0, 0),
          verts(2, 2),
          corner_radius(0),
          corner_verts(0),
          has_uv(false),
          corner_mask(CornerMask::kAll) {}
    HashValue id = 0;
    mathfu::vec2 size;
    mathfu::vec2i verts;
    float corner_radius;
    int corner_verts;
    bool has_uv;
    CornerMask corner_mask;
  };

  enum class SortMode {
    kNone,
    // Sort order: hierarchical sorting based on scene graph.
    kSortOrderDecreasing,
    kSortOrderIncreasing,
    // Sort based only on Z-position of the entity.  Used for simple UIs.
    kWorldSpaceZBackToFront,
    kWorldSpaceZFrontToBack,
    // Average space: eye space of the camera calculated by averaging the
    // cameras of each of the known views.  Sorting in average space is only
    // performed once, ie it's not done for every view.
    kAverageSpaceOriginBackToFront,
    kAverageSpaceOriginFrontToBack,
  };

  enum class CullMode {
    kNone,
    kVisibleInAnyView,
  };

  using PrimitiveType = MeshData::PrimitiveType;
  using SortOrder = uint64_t;
  using SortOrderOffset = int;

  using Deformation =
      std::function<void(float* data, size_t len, size_t stride)>;

  using TextureProcessor = std::function<void(const TexturePtr out_texture)>;

  explicit RenderSystem(Registry* registry);
  ~RenderSystem() override;

  // Initializes inter-system dependencies.
  void Initialize() override;

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

  /// Sets the render system to draw in stereoscopic multi view mode.
  void SetStereoMultiviewEnabled(bool enabled);

  // Loads a font.
  void PreloadFont(const char* name);

  // Returns a resident white texture with an alpha channel: (1, 1, 1, 1).
  const TexturePtr& GetWhiteTexture() const;

  // Returns a resident invalid texture to be used when a requested image fails
  // to load.  On debug builds it's a watermelon; on release builds it's just
  // the white texture.
  const TexturePtr& GetInvalidTexture() const;

  // Loads the texture with the given |filename|. When using the next backend,
  // the render system holds only a weak reference to the texture, so it will be
  // automatically unloaded when it falls out of use.
  TexturePtr LoadTexture(const std::string& filename);

  // Loads the texture with the given |filename| and optionally creates mips.
  // When using the next backend, the render system holds only a weak reference
  // to the texture, so it will be automatically unloaded when it falls out of
  // use.
  TexturePtr LoadTexture(const std::string& filename, bool create_mips);

  // Loads the texture atlas with the given |filename|. The render system
  // maintains a reference to all atlases.
  void LoadTextureAtlas(const std::string& filename);

  // Returns a texture that had been loaded by its hash. If the texture doesn't
  // exist this will return |nullptr|.
  TexturePtr GetTexture(HashValue texture_hash) const;

  // Loads a mesh with the given filename. When using the next backend the
  // render system holds a weak reference to the Mesh, otherwise it is a strong
  // reference.
  MeshPtr LoadMesh(const std::string& filename);


  // Loads the shader with the given |filename|.
  ShaderPtr LoadShader(const std::string& filename);

  // FPL: Loads a list of fonts.  Each glyph will check each font in the list
  // until it finds one that supports it.
  // Ion: Only the first font is loaded.
  FontPtr LoadFonts(const std::vector<std::string>& names);

  // Adds a mesh and corresponding rendering information with the Entity using
  // the specified ComponentDef.
  void Create(Entity e, HashValue type, const Def* def) override;

  // Creates an empty render component for an Entity. It is expected to be
  // populated in code.
  void Create(Entity e, RenderPass pass);

  // Creates an empty render component for an Entity identified by |comp_id|.
  void Create(Entity e, HashValue comp_id, RenderPass pass);

  // Performs post creation initialization.
  void PostCreateInit(Entity e, HashValue type, const Def* def) override;

  // Disassociates all rendering data from the Entity.
  void Destroy(Entity e) override;

  // Disassociates all rendering data identified by |comp_id| from the Entity.
  void Destroy(Entity e, HashValue comp_id);

  // Call once per frame before other draw calls, will execute zero or one
  // image processing tasks per call.
  void ProcessTasks();

  // Waits for all outstanding rendering assets to finish loading.
  void WaitForAssetsToLoad();

  // Returns |entity|'s render pass, or RenderPass_Invalid if |entity| isn't
  // known to RenderSystem.
  RenderPass GetRenderPass(Entity entity) const;

  // Returns |entity|'s default color, as specified in its json.
  const mathfu::vec4& GetDefaultColor(Entity entity) const;

  // Sets the |entity|'s default color, overriding the color specified in its
  // json.
  void SetDefaultColor(Entity entity, const mathfu::vec4& color);

  // Copies the cached value of |entity|'s color uniform into |color|. Returns
  // false if the value of the uniform was not found.
  bool GetColor(Entity entity, mathfu::vec4* color) const;

  // Sets the shader's color uniform for the specified |entity|.
  void SetColor(Entity entity, const mathfu::vec4& color);

  // Sets a shader uniform value for the specified Entity.  The |dimension| must
  // be 1, 2, 3, 4, or 16. Arrays of vector with dimension 2 or 3 should contain
  // vec2_packed or vec3_packed. The size of the |data| array is assumed to be
  // the same as |dimension|.
  void SetUniform(Entity e, const char* name, const float* data, int dimension);

  // Sets an array of shader uniform values for the specified Entity.  The
  // |dimension| must be 1, 2, 3, 4, or 16. Arrays of vector with dimension 2 or
  // 3 should contain vec2_packed or vec3_packed.  The size of the |data| array
  // is assumed to be the same as |dimension| * |count|.
  void SetUniform(Entity e, const char* name, const float* data, int dimension,
                  int count);

  // Sets an array of shader uniform values for the specified Entity identified
  // via |comp_id|.  The |dimension| must be 1, 2, 3, 4, or 16. Arrays of vector
  // with dimension 2 or 3 should contain vec2_packed or vec3_packed.  The size
  // of the |data| array is assumed to be the same as |dimension| * |count|.
  void SetUniform(Entity e, HashValue comp_id, const char* name,
                  const float* data, int dimension, int count);

  // Copies the cached value of the uniform |name| into |data_out|, respecting
  // the |length| limit. Returns false if the value of the uniform was not
  // found.
  bool GetUniform(Entity e, const char* name, size_t length,
                  float* data_out) const;

  // Copies an |entity|'s (associated with an |comp_id|) cached value of the
  // uniform |name| into |data_out|, respecting the |length| limit. Returns
  // false if the value of the uniform was not found.
  bool GetUniform(Entity e, HashValue comp_id, const char* name, size_t length,
                  float* data_out) const;

  // Makes |entity| use all the same uniform values as |source|.
  void CopyUniforms(Entity entity, Entity source);

  // Returns the number of bones associated with |entity|.
  int GetNumBones(Entity entity) const;

  // Returns the array of bone indices associated with the Entity |e| used for
  // skeletal animations.
  const uint8_t* GetBoneParents(Entity e, int* num) const;

  // Returns the array of bone names associated with the Entity |e|.  The length
  // of the array is GetNumBones(e), and 'num' will be set to this if non-null.
  const std::string* GetBoneNames(Entity e, int* num) const;

  // Returns the array of default bone transform inverses (AKA inverse bind-pose
  // matrices) associated with the Entity |e|.  The length of the array is
  // GetNumBones(e), and 'num' will be set to this if non-null.
  const mathfu::AffineTransform* GetDefaultBoneTransformInverses(
      Entity e, int* num) const;

  // Sets |entity|'s shader uniforms using |transforms|.
  void SetBoneTransforms(Entity entity,
                         const mathfu::AffineTransform* transforms,
                         int num_transforms);

  // Attaches a texture to the specified Entity.
  void SetTexture(Entity e, int unit, const TexturePtr& texture);
  void SetTexture(Entity e, HashValue component_id, int unit,
                  const TexturePtr& texture);

  // Loads and attach a texture to the specified Entity.
  void SetTexture(Entity e, int unit, const std::string& file);

  // Create and return a pre-processed texture.  This will set up a rendering
  // environment suitable to render |source_texture| with a pre-process shader.
  // texture and shader binding / setup should be performed in |processor|.
  TexturePtr CreateProcessedTexture(const TexturePtr& source_texture,
                                    bool create_mips,
                                    TextureProcessor processor);

  TexturePtr CreateProcessedTexture(const TexturePtr& source_texture,
                                    bool create_mips,
                                    RenderSystem::TextureProcessor processor,
                                    const mathfu::vec2i& output_dimensions);

  // Attaches a texture object with given GL |texture_target| and |texture_id|
  // to the specified Entity.
  void SetTextureId(Entity e, int unit, uint32_t texture_target,
                    uint32_t texture_id);
  void SetTextureId(Entity e, HashValue component_id, int unit,
                    uint32_t texture_target, uint32_t texture_id);

  // Returns a pointer to the texture assigned to |entity|'s |unit|.
  TexturePtr GetTexture(Entity entity, int unit) const;


  // Updates the entity to display a text string. If there is a deformation
  // function set on this entity then the quad generation will be deferred until
  // ProcessTasks is called.
  void SetText(Entity e, const std::string& text);

  // Returns a vector of tags associated with the entity. This will only be
  // populated if parse_and_strip_html is set to true in the FontDef. Returns
  // null if this entity does not have a render def.
  const std::vector<LinkTag>* GetLinkTags(Entity e) const;

  // Copies the cached value of the |entity|'s Quad into |quad|. Returns false
  // if no quad is found.
  bool GetQuad(Entity e, Quad* quad) const;

  // Creates a Quad of a given size. If there is a deformation function set on
  // this entity then the quad generation will be deferred until ProcessTasks is
  // called.
  void SetQuad(Entity e, const Quad& quad);
  void SetQuad(Entity e, HashValue component_id, const Quad& quad);

  // Retrieves a mesh attached to the specified |entity| identified by
  // |comp_id|.
  MeshPtr GetMesh(Entity e, HashValue comp_id);

  // Attaches a mesh to the specified |entity| identified by |comp_id|. Does
  // not apply deformation.
  void SetMesh(Entity e, HashValue comp_id, const MeshPtr& mesh);

  // Attaches a mesh to the specified Entity.
  // TODO(b/34981311): This is not a template because it would be annoying to
  // unpack the TriangleMesh into vertices and indices, since we'll need to pack
  // it back up into a TriangleMesh anyways to use some nice helper methods.
  // This will get fixed when we migrate over to using MeshData instead.
  void SetMesh(Entity e, const TriangleMesh<VertexPT>& mesh);
  void SetMesh(Entity e, HashValue component_id,
               const TriangleMesh<VertexPT>& mesh);

  // Like SetMesh, but applies |entity|'s deformation, if any.
  void SetAndDeformMesh(Entity entity, const TriangleMesh<VertexPT>& mesh);
  void SetAndDeformMesh(Entity entity, HashValue component_id,
                        const TriangleMesh<VertexPT>& mesh);

  // Attaches a mesh to the specified Entity. Does not apply deformation.
  void SetMesh(Entity e, const MeshData& mesh);

  // Loads and attaches a mesh to the specified Entity.
  void SetMesh(Entity e, const std::string& file);

  // Returns |entity|'s shader, or nullptr if it isn't known to RenderSystem.
  ShaderPtr GetShader(Entity entity, HashValue comp_id) const;

  // Returns |entity|'s shader, or nullptr if it isn't known to RenderSystem.
  ShaderPtr GetShader(Entity entity) const;

  // Attaches a shader program to specified |entity| identified by |comp_id|.
  void SetShader(Entity entity, HashValue comp_id, const ShaderPtr& shader);

  // Attaches a shader program to |entity|.
  void SetShader(Entity entity, const ShaderPtr& shader);

  // Loads and attaches a shader to the specified Entity.
  void SetShader(Entity e, const std::string& file);

  // Sets |font| on |entity|.  Takes effect on the next call to SetText.
  void SetFont(Entity entity, const FontPtr& font);

  // Sets |entity|'s text size to |size|, which measures mm between lines.
  void SetTextSize(Entity entity, int size);

  // Returns |e|'s sort order offset.
  SortOrderOffset GetSortOrderOffset(Entity e) const;

  // Sets the offset used when determining this Entity's draw order.
  void SetSortOrderOffset(Entity e, SortOrderOffset sort_order_offset);
  void SetSortOrderOffset(Entity e, HashValue component_id,
                          SortOrderOffset sort_order_offset);

  // Defines an entity's stencil mode.
  void SetStencilMode(Entity e, StencilMode mode, int value);

  // Returns whether or not a texture unit has a texture for an entity.
  bool IsTextureSet(Entity e, int unit) const;

  // Returns whether or not a texture unit is ready to render.
  bool IsTextureLoaded(Entity e, int unit) const;

  // Returns whether or not the texture has been loaded.
  bool IsTextureLoaded(const TexturePtr& texture) const;

  // Returns true if all currently set assets have loaded.
  bool IsReadyToRender(Entity entity) const;

  // Returns whether or not |e| is hidden / rendering. If the entity is not
  // registered with the RenderSystem, this will return true.
  bool IsHidden(Entity e) const;

  // Specifies custom deformation function for dynamically generated meshes.
  // This function must be set prior to entity creation. If it is then the
  // render system will defer generating the deformed mesh until the first call
  // to ProcessTasks. Updates to this function will not affect any previously
  // generated meshes, but future calls to SetText and SetQuad will use the new
  // deformation function.
  void SetDeformationFunction(Entity e, const Deformation& deform);

  // Stops rendering the entity.
  void Hide(Entity e);

  // Resumes rendering the entity.
  void Show(Entity e);

  // Sets |e|'s render pass to |pass|.
  void SetRenderPass(Entity e, RenderPass pass);

  // Returns |pass|'s sort mode.
  SortMode GetSortMode(RenderPass pass) const;

  // Sets |pass|'s sort mode.
  void SetSortMode(RenderPass pass, SortMode mode);

  // Sets |pass|'s cull mode.
  void SetCullMode(RenderPass pass, CullMode mode);

  /// Sets the render target to be used when rendering a specific pass.
  void SetRenderTarget(HashValue pass, HashValue render_target_name);

  // Sets depth function to kDepthFunctionLess if |enabled| and to
  // kDepthFunctionDisabled if !|enabled| in RenderSystemFpl.
  // Sets kDepthTest to |enabled| in RenderSystemIon.
  void SetDepthTest(const bool enabled);

  // Sets glDepthMask to |enabled|.
  void SetDepthWrite(const bool enabled);

  // Sets the GL blend mode to |blend_mode|.
  void SetBlendMode(const fplbase::BlendMode blend_mode);

  // Sets |view| to be the screen rectangle that gets rendered to.
  void SetViewport(const View& view);

  // Sets |mvp| to be a position at which to project models.
  void SetClipFromModelMatrix(const mathfu::mat4& mvp);

  // Returns the cached value of the clear color.
  mathfu::vec4 GetClearColor() const;

  // Sets the value used to clear the color buffer.
  void SetClearColor(float r, float g, float b, float a);

  // Renders all objects in |views| for each predefined render pass.
  void Render(const View* views, size_t num_views);

  // Renders all objects in |views| for the specified |pass|.
  void Render(const View* views, size_t num_views, RenderPass pass);

  // Returns the underlyng RenderSystemImpl (Ion-based or FPL-based)
  // to expose implementation-specific behaviour depending on which
  // implementation is depended upon. The RenderSystemImpl header which
  // is used must match the render system that is depended upon in the
  // BUILD rule.
  RenderSystemImpl* GetImpl();

  /// Prepares the render system to render a new frame. This needs to be called
  /// at the beginning of the frame before issuing any render calls. This
  /// function handles clearing the frame buffer and readying up for drawing to
  /// the display.
  void BeginFrame();

  /// Performs post-frame processing. This needs to be called at the end of the
  /// frame before submitting starting to draw another frame or calling
  /// BeginFrame a second time.
  void EndFrame();

  // Creates a temporary interface that allows a mesh to be defined for
  // |entity|.  This mesh is used until UpdateDynamicMesh is called again.
  //
  // This function will locate or create a suitable mesh object that matches the
  // requested primitive, vertex and index parameters.  This mesh may have
  // restrictions on it for performance reasons, or may have been previously
  // used for a different purpose, so you should not make any assumptions about
  // the readability or contents of this mesh; instead, you should only write
  // your data into it.  Note also that due to these restrictions, deformation
  // will not be performed on the mesh.
  void UpdateDynamicMesh(Entity entity, PrimitiveType primitive_type,
                         const VertexFormat& vertex_format,
                         const size_t max_vertices, const size_t max_indices,
                         const std::function<void(MeshData*)>& update_mesh);

  // Immediately binds |shader|.
  void BindShader(const ShaderPtr& shader);

  // Immediately binds |texture| in |unit|.
  void BindTexture(int unit, const TexturePtr& texture);

  // Immediately binds |uniform| on the currently bound shader.
  void BindUniform(const char* name, const float* data, int dimension);

  // Immediately draws a list of vertices arranged by |primitive_type|.
  void DrawPrimitives(PrimitiveType primitive_type,
                      const VertexFormat& vertex_format,
                      const void* vertex_data, size_t num_vertices);

  template <typename Vertex>
  void DrawPrimitives(PrimitiveType primitive_type, const Vertex* vertices,
                      size_t num_vertices);

  template <typename Vertex>
  void DrawPrimitives(PrimitiveType primitive_type,
                      const std::vector<Vertex>& vertices);

  // Immediately draws an indexed list of vertices arranged by |primitive_type|.
  void DrawIndexedPrimitives(PrimitiveType primitive_type,
                             const VertexFormat& vertex_format,
                             const void* vertex_data, size_t num_vertices,
                             const uint16_t* indices, size_t num_indices);

  template <typename Vertex>
  void DrawIndexedPrimitives(PrimitiveType primitive_type,
                             const Vertex* vertices, size_t num_vertices,
                             const uint16_t* indices, size_t num_indices);

  template <typename Vertex>
  void DrawIndexedPrimitives(PrimitiveType primitive_type,
                             const std::vector<Vertex>& vertices,
                             const std::vector<uint16_t>& indices);

  // Immediately draws a triangle mesh.
  template <typename Vertex>
  void DrawMesh(const TriangleMesh<Vertex>& mesh);

  // Gets all possible caret positions for a given text entity. Returns nullptr
  // if given entity is not text or doesn't support caret.
  const std::vector<mathfu::vec3>* GetCaretPositions(Entity e) const;

  /// Creates a render target that can be used in a pass for rendering, and as a
  /// texture on top of an object.
  void CreateRenderTarget(HashValue render_target_name,
                          const mathfu::vec2i& dimensions,
                          TextureFormat texture_format,
                          DepthStencilFormat depth_stencil_format);

  /// Bind a uniform to the currently bound shader.
  void BindUniform(const Uniform& uniform);

 private:
  std::unique_ptr<RenderSystemImpl> impl_;
};

template <typename Vertex>
inline void RenderSystem::DrawPrimitives(PrimitiveType primitive_type,
                                         const Vertex* vertices,
                                         size_t num_vertices) {
  DrawPrimitives(primitive_type, Vertex::kFormat, vertices, num_vertices);
}

template <typename Vertex>
inline void RenderSystem::DrawPrimitives(PrimitiveType primitive_type,
                                         const std::vector<Vertex>& vertices) {
  DrawPrimitives(primitive_type, vertices.data(), vertices.size());
}

template <typename Vertex>
inline void RenderSystem::DrawIndexedPrimitives(PrimitiveType primitive_type,
                                                const Vertex* vertices,
                                                size_t num_vertices,
                                                const uint16_t* indices,
                                                size_t num_indices) {
  DrawIndexedPrimitives(primitive_type, Vertex::kFormat, vertices,
                        num_vertices, indices, num_indices);
}

template <typename Vertex>
inline void RenderSystem::DrawIndexedPrimitives(
    PrimitiveType primitive_type, const std::vector<Vertex>& vertices,
    const std::vector<uint16_t>& indices) {
  DrawIndexedPrimitives(primitive_type, Vertex::kFormat, vertices.data(),
                        vertices.size(), indices.data(), indices.size());
}

template <typename Vertex>
inline void RenderSystem::DrawMesh(const TriangleMesh<Vertex>& mesh) {
  if (!mesh.GetIndices().empty()) {
    DrawIndexedPrimitives(MeshData::PrimitiveType::kTriangles,
                          mesh.GetVertices().data(), mesh.GetVertices().size(),
                          mesh.GetIndices().data(), mesh.GetIndices().size());
  } else {
    DrawPrimitives(MeshData::PrimitiveType::kTriangles,
                   mesh.GetVertices().data(), mesh.GetVertices().size());
  }
}

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::RenderSystem);

#endif  // LULLABY_SYSTEMS_RENDER_RENDER_SYSTEM_H_
