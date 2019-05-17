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

#include "lullaby/systems/render/filament/mesh.h"

#include "filament/IndexBuffer.h"
#include "filament/RenderableManager.h"
#include "filament/VertexBuffer.h"
#include "utils/EntityManager.h"
#include "lullaby/util/logging.h"

namespace lull {

Mesh::Mesh(filament::Engine* engine) : engine_(engine) {}

void Mesh::Init(FVertexPtr* vertex_buffers, FIndexPtr* index_buffers,
                MeshData* mesh_datas, size_t len) {
  if (IsLoaded()) {
    DLOG(FATAL) << "Can only be initialized once.";
    return;
  }
  // Pre-allocate memory for all data members.
  size_t submesh_count = 0;
  for (size_t i = 0; i < len; ++i) {
    submesh_count += std::max(1u, mesh_datas[i].GetNumSubMeshes());
  }
  submeshes_.reserve(submesh_count);
  vertex_buffers_.reserve(len);
  index_buffers_.reserve(len);
  mesh_datas_.reserve(len);

  for (size_t i = 0; i < len; ++i) {
    CreateSubmeshes(std::move(vertex_buffers[i]), std::move(index_buffers[i]),
                    std::move(mesh_datas[i]));
  }

  for (auto& cb : on_load_callbacks_) {
    cb();
  }
  on_load_callbacks_.clear();
}

void Mesh::CreateSubmeshes(FVertexPtr vertex_buffer, FIndexPtr index_buffer,
                           MeshData mesh_data) {
  // Configure common data since all new Submeshes will reference the same
  // MeshData.
  Submesh submesh;
  submesh.vertex_format = mesh_data.GetVertexFormat();
  submesh.vertex_buffer_index = static_cast<int>(vertex_buffers_.size());
  submesh.index_buffer_index = static_cast<int>(index_buffers_.size());
  submesh.mesh_data_index = static_cast<int>(mesh_datas_.size());

  // If the mesh has no submeshes, create a single Submesh out of the base and
  // store it.
  const size_t num_submeshes = mesh_data.GetNumSubMeshes();
  if (num_submeshes == 0) {
    submesh.aabb = mesh_data.GetAabb();
    submeshes_.push_back(submesh);
  } else {
    // Otherwise create a Submesh for each one specified by the MeshData.
    const auto& submesh_aabbs = mesh_data.GetSubmeshAabbs();
    for (size_t i = 0; i < num_submeshes; ++i) {
      submesh.index_range = mesh_data.GetSubMesh(i);
      if (i < submesh_aabbs.size()) {
        submesh.aabb = submesh_aabbs[i];
      } else {
        submesh.aabb = mesh_data.GetAabb();
      }
      submeshes_.push_back(submesh);
    }

    // Flag that some submeshes share GPU buffers, which disables
    // ReplaceSubmesh() functionality. This could be implemented with some
    // additional tracking of which Submesh constructs belong to which MeshData,
    // but for now is unnecessary.
    if (num_submeshes > 1) {
      index_range_submeshes_ = true;
    }
  }

  mesh_datas_.emplace_back(std::move(mesh_data));
  vertex_buffers_.emplace_back(std::move(vertex_buffer));
  index_buffers_.emplace_back(std::move(index_buffer));
}

void Mesh::ReplaceSubmesh(size_t index, MeshData mesh_data) {
  if (index_range_submeshes_) {
    LOG(DFATAL) << "ReplaceSubmesh() is disabled because multiple submeshes "
                   "refer to the same GPU buffers.";
    return;
  }
  if (index >= submeshes_.size()) {
    LOG(DFATAL) << "Invalid submesh index.";
    return;
  }
  const size_t num_submeshes = mesh_data.GetNumSubMeshes();
  if (num_submeshes > 1) {
    LOG(DFATAL) << "Cannot replace a single submesh with multiple submeshes.";
    return;
  }

  Submesh& submesh = submeshes_[index];

  // Reconfigure the specific Submesh.
  submesh.vertex_format = mesh_data.GetVertexFormat();
  if (num_submeshes == 0) {
    submesh.index_range = MeshData::IndexRange();
    submesh.aabb = mesh_data.GetAabb();
  } else {
    // There must be exactly one submesh since we checked for multiple before.
    submesh.index_range = mesh_data.GetSubMesh(0);
    const auto& submesh_aabbs = mesh_data.GetSubmeshAabbs();
    if (!submesh_aabbs.empty()) {
      submesh.aabb = submesh_aabbs[0];
    } else {
      submesh.aabb = mesh_data.GetAabb();
    }
  }

  ReplaceVertexBuffer(submesh.vertex_buffer_index, mesh_data);
  ReplaceIndexBuffer(submesh.index_buffer_index, mesh_data);

  mesh_datas_[submesh.mesh_data_index] = std::move(mesh_data);
}

void Mesh::ReplaceVertexBuffer(int index, const MeshData& mesh) {
  const size_t vertex_size = mesh.GetVertexFormat().GetVertexSize();
  const size_t count = mesh.GetNumVertices();
  const uint8_t* bytes = mesh.GetVertexBytes();

  FVertexPtr& vptr = vertex_buffers_[index];
  filament::VertexBuffer::BufferDescriptor desc(bytes, count * vertex_size);
  vptr->setBufferAt(*engine_, 0, std::move(desc));
}

void Mesh::ReplaceIndexBuffer(int index, const MeshData& mesh) {
  const uint8_t* bytes = mesh.GetIndexBytes();
  const size_t count = mesh.GetNumIndices();
  const size_t size = mesh.GetIndexSize();

  FIndexPtr& iptr = index_buffers_[index];
  filament::IndexBuffer::BufferDescriptor desc(bytes, count * size);
  iptr->setBuffer(*engine_, std::move(desc));
}

VertexFormat Mesh::GetVertexFormat(size_t submesh_index) const {
  if (submesh_index >= submeshes_.size()) {
    return VertexFormat();
  }
  return submeshes_[submesh_index].vertex_format;
}

size_t Mesh::GetNumSubMeshes() const { return submeshes_.size(); }

Aabb Mesh::GetSubMeshAabb(size_t index) const {
  if (index < submeshes_.size()) {
    return submeshes_[index].aabb;
  }
  return Aabb();
}

MeshData::IndexRange Mesh::GetSubMeshRange(size_t index) const {
  if (index < submeshes_.size()) {
    return submeshes_[index].index_range;
  }
  return MeshData::IndexRange();
}

filament::VertexBuffer* Mesh::GetVertexBuffer(size_t index) {
  if (index < submeshes_.size()) {
    return vertex_buffers_[submeshes_[index].vertex_buffer_index].get();
  }
  return nullptr;
}

filament::IndexBuffer* Mesh::GetIndexBuffer(size_t index) {
  if (index < submeshes_.size()) {
    return index_buffers_[submeshes_[index].index_buffer_index].get();
  }
  return nullptr;
}

bool Mesh::IsLoaded() const { return !submeshes_.empty(); }

void Mesh::AddOrInvokeOnLoadCallback(const std::function<void()>& callback) {
  if (IsLoaded()) {
    callback();
  } else {
    on_load_callbacks_.push_back(callback);
  }
}

VertexFormat GetVertexFormat(const MeshPtr& mesh, size_t index) {
  return mesh ? mesh->GetVertexFormat(index) : VertexFormat();
}

bool IsMeshLoaded(const MeshPtr& mesh) {
  return mesh ? mesh->IsLoaded() : false;
}

size_t GetNumSubmeshes(const MeshPtr& mesh) {
  return mesh ? mesh->GetNumSubMeshes() : 0;
}

void SetGpuBuffers(const MeshPtr& mesh, uint32_t vbo, uint32_t vao,
                   uint32_t ibo) {
  LOG(ERROR) << "SetGpuBuffers() is unsupported.";
}

void ReplaceSubmesh(MeshPtr mesh, size_t submesh_index,
                    const MeshData& mesh_data) {
  if (mesh) {
    mesh->ReplaceSubmesh(submesh_index, mesh_data.CreateHeapCopy());
  }
}

}  // namespace lull
