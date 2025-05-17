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

#ifndef REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_MESH_H_
#define REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_MESH_H_

#include <cstddef>
#include <vector>

#include "absl/types/span.h"
#include "filament/IndexBuffer.h"
#include "filament/RenderableManager.h"
#include "filament/VertexBuffer.h"
#include "redux/engines/render/filament/filament_utils.h"
#include "redux/engines/render/mesh.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/graphics/mesh_data.h"

namespace redux {

// Manages a filament VertexBuffer and IndexBuffer created using MeshData.
class FilamentMesh : public Mesh, public Readiable {
 public:
  explicit FilamentMesh(Registry* registry);
  FilamentMesh(Registry* registry, absl::Span<MeshData> meshes);

  // Returns the number of parts in the mesh.
  size_t GetNumParts() const;

  // Returns the name of the part at the given index.
  HashValue GetPartName(size_t index) const;

  // Populates the RenderableManager::Builder with information from this mesh.
  void PreparePartRenderable(size_t index,
                             filament::RenderableManager::Builder& builder);

  // Returns the list of vertex data usages encoded in the mesh.
  absl::Span<const VertexUsage> GetVertexUsages() const;

 private:
  struct PartData {
    HashValue name;
    Box bounding_box;
    MeshPrimitiveType primitive_type;
    FilamentResourcePtr<filament::VertexBuffer> vbuffer;
    FilamentResourcePtr<filament::IndexBuffer> ibuffer;
  };

  filament::Engine* fengine_ = nullptr;
  std::vector<PartData> parts_;
  std::vector<VertexUsage> usages_;
};
}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_MESH_H_
