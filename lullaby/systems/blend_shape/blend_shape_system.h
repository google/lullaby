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

#ifndef LULLABY_SYSTEMS_MODEL_ASSET_BLEND_SHAPE_SYSTEM_H_
#define LULLABY_SYSTEMS_MODEL_ASSET_BLEND_SHAPE_SYSTEM_H_

#include "lullaby/generated/blend_shape_def_generated.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/systems/model_asset/model_asset.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/util/resource_manager.h"
#include "lullaby/util/string_view.h"

namespace lull {

/// Provides blend shape support to Lullaby.
///
/// This operates on a lullmodel which contains contains blend shapes.  You will
/// need to supply a BlendDef.
class BlendShapeSystem : public System {
 public:
  /// Parameters for blend vertex.
  struct BlendVertexParams {
    float weight;

    // Position and normal of a vertex in the neutral mesh.
    mathfu::vec3 neutral_pos;
    mathfu::vec3 neutral_norm;

    // Position and normal of a vertex in a blend mesh.
    mathfu::vec3 blend_pos;
    mathfu::vec3 blend_norm;

    // Resulting position and normal based on weight.  Updated after calling
    // BlendVertex.
    mathfu::vec3 calculated_pos;
    mathfu::vec3 calculated_norm;
  };

  explicit BlendShapeSystem(Registry* registry);

  BlendShapeSystem(const BlendShapeSystem&) = delete;
  BlendShapeSystem& operator=(const BlendShapeSystem&) = delete;

  void PostCreateInit(Entity entity, HashValue type, const Def* def) override;

  void Destroy(Entity entity) override;

  /// Modifies the mesh after it is influenced by the weights (clamped to 0..1).
  void UpdateWeights(Entity entity, const Span<float>& weights);

  /// Returns the number of blends.
  size_t GetBlendCount(Entity entity) const;

  /// Returns the name associated with a blend at index, or HashValue(0) if no
  /// such blend exists.
  HashValue GetBlendName(Entity entity, size_t index) const;

  /// Finds the index of a blend by the name.
  Optional<size_t> FindBlendIndex(Entity entity, HashValue name) const;

  /// Returns true if UpdateWeights can be called.
  bool IsReady(Entity entity) const;

  /// Returns the number of vertices in this blended model.
  size_t GetVertexCount(Entity entity) const;

  /// Reads a current vertex from the model. It will reflect the last time the
  /// developer called UpdateWeights.
  bool ReadVertex(Entity entity, size_t vertex_index,
                  mathfu::vec3* out_position, mathfu::vec3* out_normal) const;

  /// Interpolates a normal and a position and accumulate the results into the
  /// provided params.  Only the delta between the interpolated position and the
  /// blended position is accumulated.
  static bool BlendVertex(BlendVertexParams* params);

 private:
  /// Holds the blend information for an entity.
  struct BlendData {
    BlendData(const std::shared_ptr<ModelAsset>& asset);

    /// Updates render_system with the results of UpdateMesh.
    void UpdateDynamicMesh(Entity entity, RenderSystem* render_system,
                           const Span<float>& weights);

    /// Updated the computed vertices so that each blend vertex is merged
    /// according to weights (set between 0..1).
    void UpdateMesh(const Span<float>& weights, MeshData* output_mesh);

    /// Writes a single vertex to our computed vertices.
    void WriteComputedVertex(size_t index, const mathfu::vec3& position,
                             const mathfu::vec3& normal);

    /// Writes all of our indices into the output mesh.
    void WriteIndices(MeshData* output_mesh);

    /// Reads a neutral vertex's position and normal.  Returns false if out of
    /// range.
    bool ReadVertex(const uint8_t* vertices, size_t vertex_index,
                    mathfu::vec3* out_position, mathfu::vec3* out_normal) const;

    HashValue GetBlendName(size_t index) const;

    Optional<size_t> FindBlendIndex(HashValue name) const;

    /// Our asset, which has our source mesh and blend meshes.
    std::shared_ptr<ModelAsset> asset;
    /// Our vertices which we will be modifying when we update the mesh.
    std::vector<uint8_t> computed_vertices;
    /// Names of the different blend shapes.
    std::vector<HashValue> blend_names;
    /// Vertices for the read-only blend shapes corresponding to blend_names.
    std::vector<const uint8_t*> blend_shapes;

    /// The parsed mesh from the lullmodel.
    const MeshData& mesh_data;
    /// The format of the computed and neutral vertices.
    const VertexFormat& vertex_format;
    /// Number of vertices in the mesh.
    const size_t vertex_count;
    /// Number of submeshes in the mesh.
    const size_t submesh_count;
    /// Offset (in bytes) in the vertex for the normal.
    size_t normal_offset = 0;
    /// Offset (in bytes) in the vertex for the position.
    size_t position_offset = 0;
  };

  // Called to finish initialization of the entity once BlendData has been
  // initialized.
  void OnAssetsLoaded(Entity entity, HashValue key);

  std::unordered_map<Entity, BlendData> blends_;
  ResourceManager<ModelAsset> models_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::BlendShapeSystem);

#endif  // LULLABY_SYSTEMS_MODEL_ASSET_BLEND_SHAPE_SYSTEM_H_
