// This file contains the list of deprecated RenderSystem functions.
// It is not a standard header file as this file gets directly included
// into the RenderSystem class declaration.

/// Params describing the properties of a Group.
struct GroupParams {
  GroupParams() {}
  explicit GroupParams(int sort_order_offset)
      : sort_order_offset(sort_order_offset) {}
  /// Sort order offset of this Group relative to other Groups.
  int sort_order_offset = 0;
};

/// Sets the default winding / front face to use. Default is
/// kCounterClockwise, same as OpenGL.
void SetDefaultFrontFace(RenderFrontFace face);

/// Sets |entity|'s render pass to |pass|.
void SetRenderPass(Entity entity, HashValue pass);

/// Returns |entity|'s render pass, or RenderPass_Invalid if |entity| isn't
/// known to RenderSystem.
HashValue GetRenderPass(Entity entity) const;

/// Specifies custom deformation function for dynamically generated meshes.
/// This function must be set prior to entity creation. If it is then the
/// render system will defer generating the deformed mesh until the first call
/// to ProcessTasks. Updates to this function will not affect any previously
/// generated meshes, but future calls to SetText and SetQuad will use the new
/// deformation function.
using DeformationFn = std::function<void(MeshData* mesh)>;
void SetDeformationFunction(Entity entity, const DeformationFn& deform);

/// Like SetMesh, but applies |entity|'s deformation, if any. If |mesh|
/// doesn't have read access, this will generate a warning and not apply the
/// deformation.
void SetAndDeformMesh(Entity entity, const MeshData& mesh);

/// Loads and attaches a mesh to the specified Entity for all passes.
void SetMesh(Entity entity, const std::string& file);

/// Sets the |data| on the shader uniform of given |type| with the given
/// |name| on the |entity|.  The |count| parameter is used to specify uniform
/// array data.
void SetUniform(Entity entity, string_view name, ShaderDataType type,
                Span<uint8_t> data, int count = 1);

void SetUniform(Entity entity, string_view name, Span<float> data,
                int count = 1);

void SetUniform(Entity entity, string_view name, Span<int> data,
                int count = 1);

/// Copies the cached value of the uniform |name| into |data_out|,
/// respecting the |length| limit. Returns false if the value of the uniform
/// was not found.
bool GetUniform(Entity entity, string_view name, size_t length,
                uint8_t* data_out) const;

/// Returns whether or not a texture unit has a texture for an entity.
bool IsTextureSet(const Drawable& drawable, int unit) const;

/// Returns whether or not the texture has been loaded.
bool IsTextureLoaded(const TexturePtr& texture) const;

/// Returns a resident white texture with an alpha channel: (1, 1, 1, 1).
TexturePtr GetWhiteTexture() const;

/// Returns a resident invalid texture to be used when a requested image fails
/// to load.  On debug builds it's a watermelon; on release builds it's just
/// the white texture.
TexturePtr GetInvalidTexture() const;

/// Returns whether or not a texture unit is ready to render.
bool IsTextureLoaded(Entity entity, int unit) const;

/// Create and return a pre-processed texture.  This will set up a rendering
/// environment suitable to render |source_texture| with a pre-process shader.
/// Texture and shader binding / setup should be performed in |processor|.
using TextureProcessor = std::function<void(TexturePtr)>;
TexturePtr CreateProcessedTexture(const TexturePtr& source_texture,
                                  bool create_mips,
                                  TextureProcessor processor);
TexturePtr CreateProcessedTexture(const TexturePtr& source_texture,
                                  bool create_mips,
                                  RenderSystem::TextureProcessor processor,
                                  const mathfu::vec2i& output_dimensions);

/// Creates a temporary interface that allows a mesh to be defined for
/// |entity|.  This mesh is used until UpdateDynamicMesh is called again.
//
/// This function will locate or create a suitable mesh object that matches
/// the requested primitive, vertex and index parameters.  This mesh may have
/// restrictions on it for performance reasons, or may have been previously
/// used for a different purpose, so you should not make any assumptions about
/// the readability or contents of this mesh; instead, you should only write
/// your data into it.  Note also that due to these restrictions, deformation
/// will not be performed on the mesh.
void UpdateDynamicMesh(Entity entity, MeshData::PrimitiveType primitive_type,
                       const VertexFormat& vertex_format,
                       const size_t max_vertices, const size_t max_indices,
                       MeshData::IndexType index_type,
                       const size_t max_ranges,
                       const std::function<void(MeshData*)>& update_mesh);
/// See UpdateDynamicMesh above.  Does not include support for submeshes or
/// 32-bit indices.
void UpdateDynamicMesh(Entity entity, MeshData::PrimitiveType primitive_type,
                       const VertexFormat& vertex_format,
                       const size_t max_vertices, const size_t max_indices,
                       const std::function<void(MeshData*)>& update_mesh);

/// Executes zero or one deferred tasks per call. Should be called once per
/// frame.
void ProcessTasks();

/// Get the id of the Group associated with |entity|, or null if no
/// component or Group.
Optional<HashValue> GetGroupId(Entity entity) const;

/// Assigns |entity| to Group |group_id| if not null.
void SetGroupId(Entity entity, const Optional<HashValue>& group_id);

/// Get the GroupParams for the Group |group_id|, or nullptr if it doesnt
/// exist.
const GroupParams* GetGroupParams(HashValue group_id) const;

/// Set the GroupParams for the Group |group_id|.
void SetGroupParams(HashValue group_id, const GroupParams& group_params);

/// DEPRECATED: Waits for all outstanding rendering assets to finish loading.
void WaitForAssetsToLoad();

/// DEPRECATED. Sets the value used to clear the color buffer.
void SetClearColor(float r, float g, float b, float a);

/// DEPRECATED. Returns the cached value of the clear color.
mathfu::vec4 GetClearColor() const;

/// DEPRECATED: Only for fpl and ion; others should use the TextureFactory.
/// Loads the texture with the given |filename|. When using the
/// next backend, the render system holds only a weak reference to the
/// texture, so it will be automatically unloaded when it falls out of use.
TexturePtr LoadTexture(const std::string& filename);

/// DEPRECATED: Only for fpl and ion; others should use the TextureFactory.
/// Loads the texture with the given |filename| and optionally creates mips.
/// When using the next backend, the render system holds only a weak reference
/// to the texture, so it will be automatically unloaded when it falls out of
/// use.
TexturePtr LoadTexture(const std::string& filename, bool create_mips);

/// DEPRECATED: Only for fpl and ion; others should use the TextureFactory.
/// Loads and attaches a texture to the specified Entity for all passes.
void SetTexture(const Drawable& drawable, int unit, const std::string& file);

/// DEPRECATED: Loads the texture atlas with the given |filename|. The render
/// system maintains a reference to all atlases.
void LoadTextureAtlas(const std::string& filename);

/// DEPRECATED: Only for fpl and ion; others should use the TextureFactory.
/// Returns a texture that had been loaded by its hash. If the texture doesn't
/// exist this will return |nullptr|.
TexturePtr GetTexture(HashValue texture_hash) const;

/// DEPRECATED: Only for fpl and ion; others should use the TextureFactory.
/// Creates a texture from image data.
TexturePtr CreateTexture(const ImageData& image);

/// DEPRECATED: Only for fpl and ion; others should use the TextureFactory.
/// Creates a texture from image data and optionally creates mips.
TexturePtr CreateTexture(const ImageData& image, bool create_mips);

/// DEPRECATED: Only for fpl and ion; others should use SetMaterial.
/// Loads the shader with the given |filename|.
ShaderPtr LoadShader(const std::string& filename);

/// DEPRECATED: Only for fpl and ion; others should use SetMaterial.
/// Returns |entity|'s shader, or nullptr if it isn't known to RenderSystem.
ShaderPtr GetShader(Entity entity, HashValue pass) const;

/// DEPRECATED: Only for fpl and ion; others should use SetMaterial.
/// Returns |entity|'s shader, or nullptr if it isn't known to RenderSystem.
ShaderPtr GetShader(Entity entity) const;

/// DEPRECATED: Only for fpl and ion; others should use SetMaterial.
/// Attaches a shader program to specified |entity| identified by |pass|.
void SetShader(Entity entity, HashValue pass, const ShaderPtr& shader);

/// DEPRECATED: Only for fpl and ion; others should use SetMaterial.
/// Attaches a shader program to |entity|.
void SetShader(Entity entity, const ShaderPtr& shader);

/// DEPRECATED: Only for fpl and ion; others should use SetMaterial.
/// Loads and attaches a shader to the specified Entity.
void SetShader(Entity entity, const std::string& file);

/// DEPRECATED: Only for fpl and ion; others should use the MeshFactory.
/// Loads a mesh with the given filename. When using the next backend the
/// render system holds a weak reference to the Mesh, otherwise it is a strong
/// reference.
MeshPtr LoadMesh(const std::string& filename);

/// DEPRECATED: Loads a font.
void PreloadFont(const char* name);

/// DEPRECATED: Updates the entity to display a text string. If there is a
/// deformation function set on this entity then the quad generation will be
/// deferred until ProcessTasks is called.
void SetText(Entity entity, const std::string& text);

/// DEPRECATED: Copies the cached value of the |entity|'s Quad into |quad|.
/// Returns false if no quad is found.
bool GetQuad(Entity entity, RenderQuad* quad) const;

/// DEPRECATED: Creates a Quad of a given size. If there is a deformation
/// function set on this entity then the quad generation will be deferred
/// until ProcessTasks is called.
void SetQuad(Entity entity, const RenderQuad& quad);

/// DEPRECATED: Sets a shader uniform value for the specified Entity for all
/// passes.  The |dimension| must be 1, 2, 3, 4, or 16. Arrays of vector with
/// dimension 2 or 3 should contain vec2_packed or vec3_packed. The size of
/// the |data| array is assumed to be the same as |dimension|.
void SetUniform(Entity entity, const char* name, const float* data,
                int dimension);

/// DEPRECATED: Sets an array of shader uniform values for the specified
/// Entity for all passes.  The |dimension| must be 1, 2, 3, 4, or 16. Arrays
/// of vector with dimension 2 or 3 should contain vec2_packed or vec3_packed.
/// The size of the |data| array is assumed to be the same as |dimension| *
/// |count|.
void SetUniform(Entity entity, const char* name, const float* data,
                int dimension, int count);

/// DEPRECATED: Sets an array of shader uniform values for the specified
/// Entity identified via |pass|.  The |dimension| must be 1, 2, 3, 4, or 16.
/// Arrays of vector with dimension 2 or 3 should contain vec2_packed or
/// vec3_packed.  The size of the |data| array is assumed to be the same as
/// |dimension| * |count|.
void SetUniform(Entity entity, HashValue pass, const char* name,
                const float* data, int dimension, int count);

/// DEPRECATED: Copies the cached value of the uniform |name| into |data_out|,
/// respecting the |length| limit. Returns false if the value of the uniform
/// was not found.
bool GetUniform(Entity entity, const char* name, size_t length,
                float* data_out) const;

/// DEPRECATED: Copies an |entity|'s (associated with an |pass|) cached value
/// of the uniform |name| into |data_out|, respecting the |length| limit.
/// Returns false if the value of the uniform was not found.
bool GetUniform(Entity entity, HashValue pass, const char* name,
                size_t length, float* data_out) const;

/// DEPRECATED: Returns the array of bone indices associated with the Entity
/// |e| used for skeletal animations.
const uint8_t* GetBoneParents(Entity entity, int* num) const;

/// DEPRECATED: Returns the array of bone names associated with the Entity
/// |e|. The length of the array is GetNumBones(e), and 'num' will be set to
/// this if non-null.
const std::string* GetBoneNames(Entity entity, int* num) const;

/// DEPRECATED: Returns the array of default bone transform inverses (AKA
/// inverse bind-pose matrices) associated with the Entity |e|.  The length of
/// the array is GetNumBones(e), and 'num' will be set to this if non-null.
const mathfu::AffineTransform* GetDefaultBoneTransformInverses(
    Entity entity, int* num) const;

/// DEPRECATED: Sets |entity|'s shader uniforms using |transforms|.
void SetBoneTransforms(Entity entity,
                       const mathfu::AffineTransform* transforms,
                       int num_transforms);

const fplbase::RenderState& GetCachedRenderState() const;

/// DEPRECATED. Updates the render state cached in the renderer. This should
/// be used if your app is sharing a GL context with another framework which
/// affects the GL state, or if you are making GL calls on your own outside of
/// Lullaby.
void UpdateCachedRenderState(const fplbase::RenderState& render_state);


/// DEPRECATED. Type aliases for backwards compatibility.
using Deformation = DeformationFn;
using CullMode = RenderCullMode;
using FrontFace = RenderFrontFace;
using StencilMode = RenderStencilMode;
using PrimitiveType = MeshData::PrimitiveType;
using Quad = RenderQuad;
using SortOrder = RenderSortOrder;
using SortOrderOffset = RenderSortOrderOffset;
using View = RenderView;
using ClearParams = RenderClearParams;
