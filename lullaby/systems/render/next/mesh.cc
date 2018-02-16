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

#include "lullaby/systems/render/next/mesh.h"

#include <vector>
#include "lullaby/systems/render/next/detail/glplatform.h"
#include "lullaby/systems/render/next/gl_helpers.h"
#include "lullaby/util/make_unique.h"

namespace lull {

Mesh::Mesh() {}

Mesh::~Mesh() {
  if (vao_) {
    GLuint handle = *vao_;
    GL_CALL(glDeleteVertexArrays(1, &handle));
  }
  if (ibo_) {
    GLuint handle = *ibo_;
    GL_CALL(glDeleteBuffers(1, &handle));
  }
  if (vbo_) {
    GLuint handle = *vbo_;
    GL_CALL(glDeleteBuffers(1, &handle));
  }
}

void Mesh::Init(const MeshData& mesh, const SkeletonData& skeleton) {
  if (IsLoaded()) {
    DLOG(FATAL) << "Can only be initialized once.";
    return;
  }

  vertex_format_ = mesh.GetVertexFormat();
  primitive_type_ = mesh.GetPrimitiveType();
  index_type_ = mesh.GetIndexType();
  num_vertices_ = mesh.GetNumVertices();
  num_indices_ = mesh.GetNumIndices();
  aabb_ = mesh.GetAabb();

  const size_t vbo_size = vertex_format_.GetVertexSize() * num_vertices_;
  GLuint gl_vbo = 0;
  GL_CALL(glGenBuffers(1, &gl_vbo));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, gl_vbo));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, vbo_size, mesh.GetVertexBytes(),
                       GL_STATIC_DRAW));
  vbo_ = gl_vbo;

  if (GlSupportsVertexArrays()) {
    GLuint gl_vao = 0;
    GL_CALL(glGenVertexArrays(1, &gl_vao));
    GL_CALL(glBindVertexArray(gl_vao));
    SetVertexAttributes(vertex_format_);
    GL_CALL(glBindVertexArray(0));
    vao_ = gl_vao;
  }

  if (mesh.GetIndexBytes()) {
    const size_t ibo_size = mesh.GetIndexSize() * num_indices_;
    GLuint gl_ibo = 0;
    GL_CALL(glGenBuffers(1, &gl_ibo));
    GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_ibo));
    GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, ibo_size,
                         mesh.GetIndexBytes(), GL_STATIC_DRAW));
    ibo_ = gl_ibo;

    submeshes_.reserve(mesh.GetNumSubMeshes());
    for (size_t i = 0; i < mesh.GetNumSubMeshes(); ++i) {
      submeshes_.push_back(mesh.GetSubMesh(i));
    }
  }

  GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));

  bone_parents_.assign(skeleton.parent_indices.begin(),
                       skeleton.parent_indices.end());
  shader_bone_indices_.assign(skeleton.shader_indices.begin(),
                              skeleton.shader_indices.end());
  bone_transform_inverses_.assign(skeleton.inverse_bind_pose.begin(),
                                  skeleton.inverse_bind_pose.end());
  bone_names_.resize(skeleton.bone_names.size());
  for (size_t i = 0; i < bone_names_.size(); ++i) {
    bone_names_[i] = skeleton.bone_names[i].data();
  }

  for (auto& cb : on_load_callbacks_) {
    cb();
  }
  on_load_callbacks_.clear();
}

bool Mesh::IsLoaded() const { return num_vertices_ > 0; }

void Mesh::AddOrInvokeOnLoadCallback(const std::function<void()>& callback) {
  if (IsLoaded()) {
    callback();
  } else {
    on_load_callbacks_.push_back(callback);
  }
}

int Mesh::GetNumVertices() const { return static_cast<int>(num_vertices_); }

int Mesh::GetNumTriangles() const {
  if (primitive_type_ == MeshData::kTriangles) {
    return static_cast<int>(num_indices_ / 3);
  } else if (primitive_type_ == MeshData::kTriangleFan) {
    return static_cast<int>(num_indices_ - 2);
  } else if (primitive_type_ == MeshData::kTriangleStrip) {
    return static_cast<int>(num_indices_ - 2);
  } else {
    // TODO(b/62088621): Fix this calculation for different primitive types.
    return static_cast<int>(num_indices_ / 3);
  }
}

size_t Mesh::GetNumSubmeshes() const { return submeshes_.size(); }

Aabb Mesh::GetAabb() const { return aabb_; }

void Mesh::Render() {
  if (!IsLoaded()) {
    return;
  }

  BindAttributes();
  if (submeshes_.empty()) {
    DrawArrays();
  } else {
    for (size_t i = 0; i < submeshes_.size(); ++i) {
      DrawElements(i);
    }
  }
  UnbindAttributes();
}

void Mesh::RenderSubmesh(size_t submesh) {
  if (!IsLoaded()) {
    return;
  }

  BindAttributes();
  if (submeshes_.empty()) {
    CHECK_EQ(submesh, 0u);
    DrawArrays();
  } else {
    DrawElements(submesh);
  }
  UnbindAttributes();
}

void Mesh::DrawArrays() {
  const GLenum gl_mode = GetGlPrimitiveType(primitive_type_);
  GL_CALL(glDrawArrays(gl_mode, 0, static_cast<int32_t>(num_vertices_)));
}

void Mesh::DrawElements(size_t index) {
  if (index >= submeshes_.size()) {
    LOG(DFATAL) << "Invalid submesh index.";
    return;
  }

  const auto& range = submeshes_[index];
  const GLenum gl_mode = GetGlPrimitiveType(primitive_type_);
  const GLenum gl_type = GetGlIndexType(index_type_);
  const void* offset = reinterpret_cast<void*>(
      MeshData::GetIndexSize(index_type_) * range.start);

  GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ibo_));
  GL_CALL(glDrawElements(gl_mode, range.end - range.start, gl_type, offset));
  GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void Mesh::BindAttributes() {
  if (vao_) {
    GL_CALL(glBindVertexArray(*vao_));
  } else {
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, *vbo_));
    SetVertexAttributes(vertex_format_);
  }
}

void Mesh::UnbindAttributes() {
  if (vao_) {
    GL_CALL(glBindVertexArray(0));
  } else {
    UnsetVertexAttributes(vertex_format_);
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
  }
}

size_t Mesh::TryUpdateRig(RigSystem* rig_system, Entity entity) {
  const size_t num_shader_bones = shader_bone_indices_.size();
  if (rig_system == nullptr) {
    return num_shader_bones;
  }
  if (bone_parents_.empty() || shader_bone_indices_.empty() ||
      bone_names_.empty() || bone_transform_inverses_.empty()) {
    return num_shader_bones;
  }
  rig_system->SetRig(entity, bone_parents_, bone_transform_inverses_,
                     shader_bone_indices_, bone_names_);
  return num_shader_bones;
}

}  // namespace lull
