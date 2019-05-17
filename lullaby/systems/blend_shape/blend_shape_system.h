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

#ifndef LULLABY_SYSTEMS_BLEND_SHAPE_BLEND_SHAPE_SYSTEM_H_
#define LULLABY_SYSTEMS_BLEND_SHAPE_BLEND_SHAPE_SYSTEM_H_

#include "lullaby/generated/blend_shape_def_generated.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/util/resource_manager.h"
#include "lullaby/util/string_view.h"

namespace lull {

/// Manipulates an Entity's mesh vertex data by blending between meshes.
///
/// Blend shape functionality uses four pieces of data:
/// 1. The Entity's mesh data. This is a writeable copy of *all* of the vertex
///    attribute data required to render the mesh, including properties that
///    will be modified by this system (e.g. position, normal) and properties
///    that won't (e.g. texture coordinates and bone weights).
/// 2. The Entity's base shape. This is a read-only copy of the *original*
///    vertex attribute data that this system modifies. The system supports
///    blending vertex positions, normals, and tangents. Orientation support is
///    present but disabled (see b/129949188).
/// 3. The Entity's blend shapes. Each blend shape consists of vertex attribute
///    data that modifies the base shape using a particular BlendMode.
/// 4. A weight for each blend shape. This affects how much influence each blend
///    shape has on the final mesh.
///
/// Blend modes determine how to interpret blend data when recomputing vertex
/// attribute data.
class BlendShapeSystem : public System {
 public:
  explicit BlendShapeSystem(Registry* registry);

  BlendShapeSystem(const BlendShapeSystem&) = delete;
  BlendShapeSystem& operator=(const BlendShapeSystem&) = delete;

  /// Determines how final vertex data is computed from the original mesh data
  /// and blend data.
  enum BlendMode {
    /// Blended vertex data is computed by interpolating between the base vertex
    /// data and blend shape vertex data.
    kInterpolate,
    /// Blended vertex data is computed by adding blend shape vertex data to the
    /// base vertex data, treating it as displacements.
    kDisplacement,
  };

  void InitBlendShape(Entity entity, MeshData mesh,
                      const VertexFormat& blend_format,
                      DataContainer base_shape, BlendMode mode);
  void AddBlendShape(Entity entity, HashValue name, DataContainer blend_shape);

  void Destroy(Entity entity) override;

  /// Modifies the mesh after it is influenced by the weights (clamped to 0..1).
  void UpdateWeights(Entity entity, Span<float> weights);

  /// Gets the current weights for an Entity, or an empty Span if no weights are
  /// currently set. Weights are in the order they were set in UpdateWeights().
  Span<float> GetWeights(Entity entity);

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

  /// Supported blendable attributes of a vertex.
  struct BlendableVertex {
    mathfu::vec3 position;
    mathfu::vec3 normal;
    mathfu::vec3 tangent;
    mathfu::quat orientation;
  };

  /// Reads a current vertex from the model. It will reflect the last time the
  /// developer called UpdateWeights.
  bool ReadVertex(Entity entity, size_t vertex_index,
                  BlendableVertex* out_vertex) const;

  /// Parameters for blending a vertex.
  struct BlendVertexParams {
    float weight;

    // Vertex data in the neutral mesh.
    BlendableVertex neutral;
    // Vertex data in the blend shape mesh.
    BlendableVertex blend;
    // Resulting vertex data from blending the above two with BlendVertex().
    BlendableVertex calculated;
  };

  /// Blends a position, normal, tangent, and orientation with the provided
  /// params. The blend mode determines how the neutral and blend attributes are
  /// combined.
  static void BlendVertex(BlendVertexParams* params, BlendMode mode);

 private:
  /// Blend information for a single Entity.
  struct BlendData {
    BlendData() {}

    /// Updated the computed vertices so that each blend vertex is merged
    /// according to weights (set between 0..1).
    void UpdateMesh(Span<float> weights);

    /// Writes a single vertex to our computed vertices.
    void UpdateMeshVertex(size_t index, const BlendableVertex& vertex);

    /// Offsets (in bytes) to the supported blendable attributes of a vertex, or
    /// -1 if the attribute isn't present.
    struct BlendableAttributeOffsets {
      int position = -1;
      int normal = -1;
      int tangent = -1;
      int orientation = -1;
    };

    /// Reads a vertex's attributes.
    void ReadVertex(const uint8_t* vertices, size_t vertex_size,
                    size_t vertex_index,
                    const BlendableAttributeOffsets& offsets,
                    BlendableVertex* out_vertex) const;

    BlendMode mode;
    MeshData mesh;
    DataContainer base_shape;
    /// Names of the different blend shapes.
    std::vector<HashValue> blend_names;
    /// Vertices for the read-only blend shapes corresponding to blend_names.
    std::vector<DataContainer> blend_shapes;
    std::vector<float> current_weights;
    size_t blend_vertex_size = 0;
    BlendableAttributeOffsets mesh_offsets;
    BlendableAttributeOffsets blend_offsets;
  };

  std::unordered_map<Entity, BlendData> blends_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::BlendShapeSystem);

#endif  // LULLABY_SYSTEMS_BLEND_SHAPE_BLEND_SHAPE_SYSTEM_H_
