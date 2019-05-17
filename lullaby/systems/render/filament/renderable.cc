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

#include "lullaby/systems/render/filament/renderable.h"

#include "filament/Box.h"
#include "lullaby/modules/render/vertex_format_util.h"
#include "lullaby/systems/render/filament/filament_utils.h"

namespace lull {

static bool IsSkinned(const MeshPtr& mesh, size_t index) {
  const VertexFormat vertex_format = mesh->GetVertexFormat(index);
  if (vertex_format.GetAttributeWithUsage(VertexAttributeUsage_BoneWeights) &&
      vertex_format.GetAttributeWithUsage(VertexAttributeUsage_BoneIndices)) {
    return true;
  }
  return false;
}

static size_t GetNumBones(const detail::UniformData& uniform) {
  if (uniform.Type() != ShaderDataType_Float4 ||  uniform.Count() % 3 != 0) {
    return 0;
  }
  return uniform.Count() / 3;
}

Renderable::Renderable(filament::Engine* engine)
    : engine_(engine) {
}

Renderable::~Renderable() {
  DestroyFilamentState();
}

void Renderable::Show() {
  data_.hidden = false;
}

void Renderable::Hide() {
  data_.hidden = true;
}

bool Renderable::IsHidden() const {
  return data_.hidden;
}

bool Renderable::IsReadyToRender() const {
  return filament_entity_ != NullOpt;
}

void Renderable::RequestFeature(HashValue feature) {
  data_.features.emplace(feature);
}

void Renderable::ClearFeature(HashValue feature) {
  data_.features.erase(feature);
}

bool Renderable::IsFeatureRequested(HashValue feature) const {
  return data_.features.count(feature) > 0;
}

mathfu::vec4 Renderable::GetColor() const {
  return data_.color;
}

void Renderable::SetColor(const mathfu::vec4& color) {
  data_.color = color;
}

void Renderable::SetTexture(TextureUsageInfo usage, const TexturePtr& texture) {
  data_.textures[usage] = texture;
}

TexturePtr Renderable::GetTexture(TextureUsageInfo usage) const {
  auto iter = data_.textures.find(usage);
  return iter != data_.textures.end() ? iter->second : nullptr;
}

void Renderable::SetUniform(HashValue name, ShaderDataType type,
                            Span<uint8_t> data) {
  data_.uniforms[name].SetData(type, data);
}

bool Renderable::ReadUniformData(HashValue name, size_t length,
                                 uint8_t* data_out) const {
  auto iter = data_.uniforms.find(name);
  if (iter == data_.uniforms.end()) {
    return false;
  }

  const detail::UniformData& uniform = iter->second;
  if (length > uniform.Size()) {
    return false;
  }
  memcpy(data_out, uniform.GetData<uint8_t>(), length);
  return true;
}

void Renderable::ReadEnvironmentFlags(std::set<HashValue>* flags) const {
  AddShaderEnvironmentFlags(data_, flags);
  if (mesh_) {
    SetEnvironmentFlags(mesh_->GetVertexFormat(index_), flags);
  }
}

void Renderable::ReadFeatureFlags(std::set<HashValue>* flags) const {
  AddShaderFeatureFlags(data_, flags);
  if (mesh_) {
    SetFeatureFlags(mesh_->GetVertexFormat(index_), flags);
  }
}

const std::string& Renderable::GetShadingModel() const {
  if (shader_) {
    return shader_->GetDescription().shading_model;
  }
  static std::string empty;
  return empty;
}

void Renderable::SetGeometry(MeshPtr mesh, size_t index) {
  if (mesh_ != mesh || index_ != index) {
    mesh_ = mesh;
    index_ = index;
    InitFilamentState();
  }
}

void Renderable::SetShader(ShaderPtr shader) {
  if (shader_ != shader) {
    // A filament shader cannot be destroyed while it's in use. Keep a copy of
    // the old shader in this scope until the InitFilamentState() function
    // properly releases it.
    ShaderPtr old = std::move(shader_);
    shader_ = shader;
    InitFilamentState();
  }
}

void Renderable::CopyFrom(const Renderable& rhs) {
  engine_ = rhs.engine_;
  mesh_ = rhs.mesh_;
  shader_ = rhs.shader_;
  data_ = rhs.data_;
  index_ = rhs.index_;
}

bool Renderable::PrepareForRendering(
    filament::Scene* scene, const mathfu::mat4* world_from_entity_matrix) {
  bool add_to_scene = true;
  if (world_from_entity_matrix == nullptr) {
    add_to_scene = false;
  }
  if (scene == nullptr) {
    add_to_scene = false;
  }
  if (IsHidden()) {
    add_to_scene = false;
  }
  if (!IsReadyToRender()) {
    add_to_scene = false;
  }

  if (add_to_scene) {
    AddToScene(scene);
    UpdateTransform(*world_from_entity_matrix);
    UpdateMaterialInstance();
    return true;
  } else {
    RemoveFromScene();
    return false;
  }
}

void Renderable::InitFilamentState() {
  if (mesh_ == nullptr) {
    return;
  }
  if (shader_ == nullptr) {
    return;
  }

  DestroyFilamentState();

  auto type = filament::RenderableManager::PrimitiveType::TRIANGLES;
  auto range = mesh_->GetSubMeshRange(index_);
  auto* vertices = mesh_->GetVertexBuffer(index_);
  auto* indices = mesh_->GetIndexBuffer(index_);

  const Aabb aabb = mesh_->GetSubMeshAabb(index_);
  const filament::math::float3 min = FilamentFloat3FromMathfuVec3(aabb.min);
  const filament::math::float3 max = FilamentFloat3FromMathfuVec3(aabb.max);
  filament::Box box;
  box.set(min, max);

  material_instance_ = shader_->CreateMaterialInstance();

  filament::RenderableManager::Builder builder(1);
  builder.geometry(0, type, vertices, indices, range.start,
                   range.end - range.start);
  builder.material(0, material_instance_);
  builder.boundingBox(box);
  builder.castShadows(true);
  builder.culling(false);
  if (IsSkinned(mesh_, index_)) {
    auto iter = data_.uniforms.find(ConstHash("bone_transforms"));
    if (iter != data_.uniforms.end()) {
      const size_t num_bones = GetNumBones(iter->second);
      builder.skinning(num_bones ? num_bones : 255);
    } else {
      builder.skinning(255);
    }
  }

  filament_entity_ = utils::EntityManager::get().create();
  builder.build(*engine_, *filament_entity_);
}

void Renderable::DestroyFilamentState() {
  if (scene_) {
    scene_->remove(*filament_entity_);
  }
  if (filament_entity_) {
    engine_->getRenderableManager().destroy(*filament_entity_);
    utils::EntityManager::get().destroy(*filament_entity_);
  }
  if (material_instance_) {
    engine_->destroy(material_instance_);
  }

  material_instance_ = nullptr;
  filament_entity_ = NullOpt;
  scene_ = nullptr;
}

template <typename T>
void SetData(filament::MaterialInstance* instance, const char* name,
             const detail::UniformData& uniform) {
  instance->setParameter<T>(name, uniform.GetData<T>(), uniform.Count());
}

void Renderable::BindUniform(HashValue name,
                             const detail::UniformData& uniform) {
  const char* pname = shader_->GetFilamentPropertyName(name);
  if (pname == nullptr) {
    return;
  }

  switch (uniform.Type()) {
    case ShaderDataType_Float1:
      SetData<float>(material_instance_, pname, uniform);
      return;
    case ShaderDataType_Float2:
      SetData<filament::math::float2>(material_instance_, pname, uniform);
      return;
    case ShaderDataType_Float3:
      SetData<filament::math::float3>(material_instance_, pname, uniform);
      return;
    case ShaderDataType_Float4:
      SetData<filament::math::float4>(material_instance_, pname, uniform);
      return;
    default:
      LOG(ERROR) << "Unsupported.";
      return;
  }
}

void Renderable::BindTexture(TextureUsageInfo usage, TexturePtr texture) {
  if (shader_ == nullptr || texture == nullptr) {
    return;
  }

  if (!texture->IsLoaded()) {
    return;
  }

  const char* pname = shader_->GetFilamentSamplerName(usage);
  if (pname == nullptr) {
    return;
  }

  const TextureParams& params = texture->GetTextureParams();
  filament::TextureSampler sampler(filament::TextureSampler::MagFilter::LINEAR);
  sampler.setMinFilter(ToFilamentMinFilter(params.min_filter));
  sampler.setMagFilter(ToFilamentMagFilter(params.mag_filter));
  sampler.setWrapModeS(ToFilamentWrapMode(params.wrap_s));
  sampler.setWrapModeT(ToFilamentWrapMode(params.wrap_t));
  material_instance_->setParameter(pname, texture->GetFilamentTexture(),
                                   sampler);
}

void Renderable::UpdateMaterialInstance() {
  const char* color_pname =
      shader_->GetFilamentPropertyName(ConstHash("color"));
  if (color_pname) {
    material_instance_->setParameter(color_pname, filament::RgbaType::LINEAR,
                                     ToLinearColorA(data_.color));
  }

  for (auto& iter : data_.uniforms) {
    if (iter.first == ConstHash("bone_transforms") &&
        IsSkinned(mesh_, index_)) {
      const size_t num_bones = GetNumBones(iter.second);
      if (num_bones == 0) {
        continue;
      }

      // Convert the bone animation data (which represents an array of
      // transposed affine matrices, where each matrix is stored as 3
      // mathfu::vec4s) and convert them into the format that filament expects
      // (an array of math::mat4fs).
      const mathfu::vec4* data = iter.second.GetData<mathfu::vec4>();
      std::vector<filament::math::mat4f> bones(num_bones);
      for (size_t i = 0; i < num_bones; ++i) {
        const size_t index = (i * 3);
        bones[i][0] = FilamentFloat4FromMathfuVec4(data[index + 0]);
        bones[i][1] = FilamentFloat4FromMathfuVec4(data[index + 1]);
        bones[i][2] = FilamentFloat4FromMathfuVec4(data[index + 2]);
        bones[i][3] = {0.f, 0.f, 0.f, 1.f};
        bones[i] = transpose(bones[i]);
      }
      auto& renderable_manager = engine_->getRenderableManager();
      auto ri = renderable_manager.getInstance(*filament_entity_);
      renderable_manager.setBones(ri, bones.data(), bones.size());
    } else {
      BindUniform(iter.first, iter.second);
    }
  }
  for (auto& iter : data_.textures) {
    BindTexture(iter.first, iter.second);
  }
}

void Renderable::UpdateTransform(const mathfu::mat4& transform) {
  auto& transform_manager = engine_->getTransformManager();
  auto ti = transform_manager.getInstance(*filament_entity_);
  transform_manager.setTransform(ti, MathFuMat4ToFilamentMat4f(transform));

  // The mesh's AABB can be updated by Mesh::ReplaceSubmesh.
  const Aabb aabb = mesh_->GetSubMeshAabb(index_);
  const filament::math::float3 min = FilamentFloat3FromMathfuVec3(aabb.min);
  const filament::math::float3 max = FilamentFloat3FromMathfuVec3(aabb.max);
  filament::Box box;
  box.set(min, max);

  auto& renderable_manager = engine_->getRenderableManager();
  auto ri = renderable_manager.getInstance(*filament_entity_);
  renderable_manager.setAxisAlignedBoundingBox(ri, box);
}

void Renderable::AddToScene(filament::Scene* scene) {
  if (scene_ == scene) {
    return;
  }

  RemoveFromScene();
  if (scene && filament_entity_) {
    scene_ = scene;
    scene_->addEntity(*filament_entity_);
  }
}

void Renderable::RemoveFromScene() {
  if (scene_ && filament_entity_) {
    scene_->remove(*filament_entity_);
  }
  scene_ = nullptr;
}

}  // namespace lull
