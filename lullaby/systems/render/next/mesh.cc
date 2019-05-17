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

#include "lullaby/systems/render/next/mesh.h"

#include <vector>

#include "lullaby/systems/render/next/detail/glplatform.h"
#include "lullaby/systems/render/next/gl_helpers.h"
#include "lullaby/util/make_unique.h"

namespace lull {

Mesh::Mesh() {}

Mesh::~Mesh() { ReleaseGpuResources(); }

void Mesh::Init(const MeshData* mesh_datas, size_t len) {
  if (IsLoaded()) {
    DLOG(FATAL) << "Can only be initialized once.";
    return;
  }

  // Pre-allocate memory for all submeshes.
  size_t submesh_count = 0;
  for (size_t i = 0; i < len; ++i) {
    submesh_count += std::max(1u, mesh_datas[i].GetNumSubMeshes());
  }
  submeshes_.reserve(submesh_count);
  vbos_.reserve(len);
  vaos_.reserve(len);
  ibos_.reserve(len);

  for (size_t i = 0; i < len; ++i) {
    CreateSubmeshes(mesh_datas[i]);
  }

  for (auto& cb : on_load_callbacks_) {
    cb();
  }
  on_load_callbacks_.clear();
}

void Mesh::CreateSubmeshes(const MeshData& mesh) {
  // Configure global mesh properties, primarily used for profiling.
  num_vertices_ += mesh.GetNumVertices();
  num_primitives_ +=
      MeshData::GetNumPrimitives(mesh.GetPrimitiveType(), mesh.GetNumIndices());

  // Merge Aabbs so the overall Aabb represents the whole mesh.
  aabb_ = MergeAabbs(aabb_, mesh.GetAabb());

  // Configure common data since all new Submeshes will reference the same
  // MeshData.
  Submesh submesh;
  submesh.vertex_format = mesh.GetVertexFormat();
  submesh.primitive_type = mesh.GetPrimitiveType();
  submesh.index_type = mesh.GetIndexType();
  submesh.num_vertices = mesh.GetNumVertices();
  submesh.vbo_index = CreateVbo(mesh);
  submesh.vao_index = CreateVao(mesh, submesh.vbo_index);
  submesh.ibo_index = CreateIbo(mesh);

  // If the mesh has no submeshes, create a single Submesh out of the base and
  // store it.
  const size_t num_submeshes = mesh.GetNumSubMeshes();
  if (num_submeshes == 0) {
    submesh.aabb = mesh.GetAabb();
    submeshes_.push_back(submesh);
  } else {
    // Otherwise create a Submesh for each one specified by the MeshData.
    const auto& submesh_aabbs = mesh.GetSubmeshAabbs();
    for (size_t i = 0; i < num_submeshes; ++i) {
      submesh.index_range = mesh.GetSubMesh(i);
      if (i < submesh_aabbs.size()) {
        submesh.aabb = submesh_aabbs[i];
      } else {
        submesh.aabb = mesh.GetAabb();
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
}

void Mesh::ReplaceSubmesh(size_t index, const MeshData& mesh) {
  if (remote_gpu_buffers_) {
    LOG(DFATAL) << "Cannot replace submeshes for remote GPU buffers.";
    return;
  }
  if (index_range_submeshes_) {
    LOG(DFATAL) << "ReplaceSubmesh() is disabled because multiple submeshes "
                   "refer to the same GPU buffers.";
    return;
  }
  if (index >= submeshes_.size()) {
    LOG(DFATAL) << "Invalid submesh index.";
    return;
  }
  const size_t num_submeshes = mesh.GetNumSubMeshes();
  if (num_submeshes > 1) {
    LOG(DFATAL) << "Cannot replace a single submesh with multiple submeshes.";
    return;
  }

  Submesh& submesh = submeshes_[index];

  // Reconfigure global properties.
  num_vertices_ -= submesh.num_vertices;
  num_vertices_ += mesh.GetNumVertices();

  num_primitives_ -= MeshData::GetNumPrimitives(
      submesh.primitive_type,
      submesh.index_range.end - submesh.index_range.start);
  num_primitives_ +=
      MeshData::GetNumPrimitives(mesh.GetPrimitiveType(), mesh.GetNumIndices());

  // Reconfigure the specific Submesh.
  submesh.vertex_format = mesh.GetVertexFormat();
  submesh.primitive_type = mesh.GetPrimitiveType();
  submesh.index_type = mesh.GetIndexType();
  submesh.num_vertices = mesh.GetNumVertices();
  if (num_submeshes == 0) {
    submesh.index_range = MeshData::IndexRange();
    submesh.aabb = mesh.GetAabb();
  } else {
    // There must be exactly one submesh since we checked for multiple before.
    submesh.index_range = mesh.GetSubMesh(0);
    const auto& submesh_aabbs = mesh.GetSubmeshAabbs();
    if (!submesh_aabbs.empty()) {
      submesh.aabb = submesh_aabbs[0];
    } else {
      submesh.aabb = mesh.GetAabb();
    }
  }

  // Instead of allocating new buffers, just re-fill the existing ones.
  FillVbo(submesh.vbo_index, mesh);
  FillVao(submesh.vao_index, mesh, submesh.vbo_index);
  FillIbo(submesh.ibo_index, mesh);

  // Recompute the Aabb.
  aabb_ = Aabb(mathfu::vec3(std::numeric_limits<float>::max()),
               mathfu::vec3(std::numeric_limits<float>::min()));
  for (const auto& sub : submeshes_) {
    aabb_ = MergeAabbs(aabb_, sub.aabb);
  }
}

bool Mesh::IsLoaded() const { return !submeshes_.empty(); }

void Mesh::AddOrInvokeOnLoadCallback(const std::function<void()>& callback) {
  if (IsLoaded()) {
    callback();
  } else {
    on_load_callbacks_.push_back(callback);
  }
}

int Mesh::GetNumVertices() const { return static_cast<int>(num_vertices_); }

int Mesh::GetNumPrimitives() const { return static_cast<int>(num_primitives_); }

size_t Mesh::GetNumSubmeshes() const { return submeshes_.size(); }

Aabb Mesh::GetAabb() const { return aabb_; }

Aabb Mesh::GetSubmeshAabb(size_t index) const {
  if (submeshes_.size() > index) {
    return submeshes_[index].aabb;
  } else {
    return GetAabb();
  }
}

void Mesh::Render() {
  if (!IsLoaded()) {
    return;
  }

  for (size_t i = 0; i < submeshes_.size(); ++i) {
    RenderSubmesh(i);
  }
}

void Mesh::RenderSubmesh(size_t index) {
  if (!IsLoaded()) {
    return;
  }

  if (index >= submeshes_.size()) {
    LOG(DFATAL) << "Invalid submesh index.";
    return;
  }

  const auto& submesh = submeshes_[index];
  BindAttributes(submesh);
  if (submesh.ibo_index == -1) {
    DrawArrays(submesh);
  } else {
    DrawElements(submesh);
  }
  UnbindAttributes(submesh);
}

void Mesh::DrawArrays(const Submesh& submesh) {
  const GLenum gl_mode = GetGlPrimitiveType(submesh.primitive_type);
  GL_CALL(glDrawArrays(gl_mode, 0, static_cast<int32_t>(submesh.num_vertices)));
}

void Mesh::DrawElements(const Submesh& submesh) {
  const GLenum gl_mode = GetGlPrimitiveType(submesh.primitive_type);
  const GLenum gl_type = GetGlIndexType(submesh.index_type);
  const void* offset = reinterpret_cast<void*>(
      MeshData::GetIndexSize(submesh.index_type) * submesh.index_range.start);

  GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ibos_[submesh.ibo_index]));
  GL_CALL(glDrawElements(gl_mode,
                         submesh.index_range.end - submesh.index_range.start,
                         gl_type, offset));
  GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void Mesh::BindAttributes(const Submesh& submesh) {
  if (submesh.vao_index >= 0) {
    GL_CALL(glBindVertexArray(*vaos_[submesh.vao_index]));
  } else {
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, *vbos_[submesh.vbo_index]));
    SetVertexAttributes(submesh.vertex_format);
  }
}

void Mesh::UnbindAttributes(const Submesh& submesh) {
  if (submesh.vao_index >= 0) {
    GL_CALL(glBindVertexArray(0));
  } else {
    UnsetVertexAttributes(submesh.vertex_format);
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
  }
}

VertexFormat Mesh::GetVertexFormat(size_t submesh_index) const {
  if (submesh_index >= submeshes_.size()) {
    return VertexFormat();
  }
  return submeshes_[submesh_index].vertex_format;
}

int Mesh::CreateVbo(const MeshData& mesh) {
  GLuint gl_vbo = 0;
  GL_CALL(glGenBuffers(1, &gl_vbo));
  const int index = static_cast<int>(vbos_.size());
  vbos_.emplace_back(gl_vbo);
  FillVbo(index, mesh);
  return index;
}

void Mesh::FillVbo(int vbo_index, const MeshData& mesh) {
  if (vbo_index < 0 || vbo_index >= vbos_.size()) {
    LOG(DFATAL) << "Invalid VBO index.";
    return;
  }
  const size_t vbo_size =
      mesh.GetVertexFormat().GetVertexSize() * mesh.GetNumVertices();
  const GLuint gl_vbo = vbos_[vbo_index].Get();
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, gl_vbo));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, vbo_size, mesh.GetVertexBytes(),
                       GL_STATIC_DRAW));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

int Mesh::CreateVao(const MeshData& mesh, int vbo_index) {
  if (!GlSupportsVertexArrays()) {
    return -1;
  }
  if (vbo_index < 0 || vbo_index >= vbos_.size()) {
    LOG(DFATAL) << "Invalid VBO index.";
    return -1;
  }

  GLuint gl_vao = 0;
  GL_CALL(glGenVertexArrays(1, &gl_vao));
  const int index = static_cast<int>(vaos_.size());
  vaos_.emplace_back(gl_vao);
  FillVao(index, mesh, vbo_index);
  return index;
}

void Mesh::FillVao(int vao_index, const MeshData& mesh, int vbo_index) {
  if (vbo_index < 0 || vbo_index >= vbos_.size() || vao_index < 0 ||
      vao_index >= vaos_.size()) {
    LOG(DFATAL) << "Invalid VAO or VBO index.";
    return;
  }
  // Bind the associated VBO before filling the VAO.
  const GLuint gl_vbo = vbos_[vbo_index].Get();
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, gl_vbo));

  const GLuint gl_vao = vaos_[vao_index].Get();
  GL_CALL(glBindVertexArray(gl_vao));
  SetVertexAttributes(mesh.GetVertexFormat());
  GL_CALL(glBindVertexArray(0));

  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

int Mesh::CreateIbo(const MeshData& mesh) {
  if (!mesh.GetIndexBytes()) {
    return -1;
  }
  GLuint gl_ibo = 0;
  GL_CALL(glGenBuffers(1, &gl_ibo));
  const int index = static_cast<int>(ibos_.size());
  ibos_.emplace_back(gl_ibo);
  FillIbo(index, mesh);
  return index;
}

void Mesh::FillIbo(int ibo_index, const MeshData& mesh) {
  if (ibo_index < 0 || ibo_index >= ibos_.size()) {
    LOG(DFATAL) << "Invalid IBO index.";
    return;
  }
  const GLuint gl_ibo = ibos_[ibo_index].Get();
  const size_t ibo_size = mesh.GetIndexSize() * mesh.GetNumIndices();
  GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_ibo));
  GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, ibo_size, mesh.GetIndexBytes(),
                       GL_STATIC_DRAW));
  GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void Mesh::ReleaseGpuResources() {
  if (remote_gpu_buffers_) {
    return;
  }
  for (auto vao : vaos_) {
    GLuint handle = *vao;
    GL_CALL(glDeleteVertexArrays(1, &handle));
  }
  for (auto ibo : ibos_) {
    GLuint handle = *ibo;
    GL_CALL(glDeleteBuffers(1, &handle));
  }
  for (auto vbo : vbos_) {
    GLuint handle = *vbo;
    GL_CALL(glDeleteBuffers(1, &handle));
  }
}

void Mesh::SetGpuBuffers(BufferHnd vbo, BufferHnd vao, BufferHnd ibo) {
  if (vaos_.size() > 1 || vbos_.size() > 1 || ibos_.size() > 1) {
    LOG(DFATAL) << "SetGpuBuffers called on a mesh with multiple existing "
                   "buffers, which may cause crashes with bad index ranges.";
  }
  ReleaseGpuResources();

  vbos_.clear();
  vbos_.resize(1, vbo);

  vaos_.clear();
  vaos_.resize(1, vao);

  ibos_.clear();
  ibos_.resize(1, ibo);

  remote_gpu_buffers_ = true;
}

VertexFormat GetVertexFormat(const MeshPtr& mesh, size_t submesh_index) {
  return mesh && mesh->IsLoaded() ? mesh->GetVertexFormat(submesh_index)
                                  : VertexFormat();
}

bool IsMeshLoaded(const MeshPtr& mesh) {
  return mesh ? mesh->IsLoaded() : false;
}

size_t GetNumSubmeshes(const MeshPtr& mesh) {
  return mesh ? mesh->GetNumSubmeshes() : 0;
}

void SetGpuBuffers(const MeshPtr& mesh, uint32_t vbo, uint32_t vao,
                   uint32_t ibo) {
  if (mesh) {
    mesh->SetGpuBuffers(vbo, vao, ibo);
  }
}

void ReplaceSubmesh(MeshPtr mesh, size_t submesh_index,
                    const MeshData& mesh_data) {
  if (mesh) {
    mesh->ReplaceSubmesh(submesh_index, mesh_data);
  }
}

}  // namespace lull
