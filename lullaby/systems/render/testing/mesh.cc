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

#include "lullaby/systems/render/testing/mesh.h"

namespace lull {

void SetGpuBuffers(const MeshPtr& mesh, uint32_t vbo, uint32_t vao,
                   uint32_t ibo) {}

size_t GetNumSubmeshes(const MeshPtr& mesh) { return 0; }

VertexFormat GetVertexFormat(const MeshPtr& mesh, size_t submesh_index) {
  return VertexFormat();
}

bool IsMeshLoaded(const MeshPtr& mesh) { return false; }

}  // namespace lull
