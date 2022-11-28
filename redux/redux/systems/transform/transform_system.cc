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

#include "redux/systems/transform/transform_system.h"

namespace redux {

static Box CalculateTransformedBox(const mat4& mat, const Box& box) {
  const vec3 c0(mat.cols[0][0], mat.cols[0][1], mat.cols[0][2]);
  const vec3 c1(mat.cols[1][0], mat.cols[1][1], mat.cols[1][2]);
  const vec3 c2(mat.cols[2][0], mat.cols[2][1], mat.cols[2][2]);
  const vec3 mid(mat.cols[3][0], mat.cols[3][1], mat.cols[3][2]);
  const vec3 min_x(box.min.x * c0);
  const vec3 min_y(box.min.y * c1);
  const vec3 min_z(box.min.z * c2);
  const vec3 max_x(box.max.x * c0);
  const vec3 max_y(box.max.y * c1);
  const vec3 max_z(box.max.z * c2);
  return Box({
      mid + min_x + min_y + min_z,
      mid + min_x + min_y + max_z,
      mid + min_x + max_y + min_z,
      mid + min_x + max_y + max_z,
      mid + max_x + min_y + min_z,
      mid + max_x + min_y + max_z,
      mid + max_x + max_y + min_z,
      mid + max_x + max_y + max_z,
  });
}

TransformSystem::TransformSystem(Registry* registry)
    : System(registry), fns_(registry) {
  RegisterDef(&TransformSystem::SetFromTransformDef);
}

void TransformSystem::OnRegistryInitialize() {
  dirty_flag_ = RequestFlag();
  fns_.RegisterMemFn("rx.Transform.SetTranslation", this,
                     &TransformSystem::SetTranslation);
  fns_.RegisterMemFn("rx.Transform.SetRotation", this,
                     &TransformSystem::SetRotation);
  fns_.RegisterMemFn("rx.Transform.SetScale", this, &TransformSystem::SetScale);
  fns_.RegisterMemFn("rx.Transform.GetTranslation", this,
                     &TransformSystem::GetTranslation);
  fns_.RegisterMemFn("rx.Transform.GetRotation", this,
                     &TransformSystem::GetRotation);
  fns_.RegisterMemFn("rx.Transform.GetScale", this, &TransformSystem::GetScale);
}

vec3 TransformSystem::GetTranslation(Entity entity) const {
  auto data = transforms_.Find<kTranslation>(entity);
  return data ? *data : vec3::Zero();
}

void TransformSystem::SetTranslation(Entity entity, const vec3& translation) {
  Transforms::Row data = transforms_.TryEmplace(entity);
  if (data) {
    CHECK(data.Get<kOwner>() == nullptr);
    data.Get<kTranslation>() = translation;
    data.Get<kFlags>().Set(dirty_flag_);
  }
}

quat TransformSystem::GetRotation(Entity entity) const {
  auto data = transforms_.Find<kRotation>(entity);
  return data ? *data : quat::Identity();
}

void TransformSystem::SetRotation(Entity entity, const quat& rotation) {
  auto data = transforms_.TryEmplace(entity);
  if (data) {
    CHECK(data.Get<kOwner>() == nullptr);
    data.Get<kRotation>() = rotation;
    data.Get<kFlags>().Set(dirty_flag_);
  }
}

vec3 TransformSystem::GetScale(Entity entity) const {
  auto data = transforms_.Find<kScale>(entity);
  return data ? *data : vec3::One();
}

void TransformSystem::SetScale(Entity entity, const vec3& scale) {
  auto data = transforms_.TryEmplace(entity);
  if (data) {
    CHECK(data.Get<kOwner>() == nullptr);
    data.Get<kScale>() = scale;
    data.Get<kFlags>().Set(dirty_flag_);
  }
}

mat4 TransformSystem::GetWorldTransformMatrix(Entity entity) const {
  Transforms::Row data = transforms_.FindRow(entity);
  if (data) {
    UpdateRow(data);
    return data.Get<kWorldMatrix>();
  }
  return mat4::Identity();
}

Transform TransformSystem::GetTransform(Entity entity) const {
  Transform transform;
  const Transforms::Row data = transforms_.FindRow(entity);
  if (data) {
    transform.translation = data.Get<kTranslation>();
    transform.rotation = data.Get<kRotation>();
    transform.scale = data.Get<kScale>();
  }
  return transform;
}

void TransformSystem::SetTransform(Entity entity, const Transform& transform) {
  SetTransform(entity, transform, nullptr);
}

void TransformSystem::SetTransform(Entity entity, const Transform& transform,
                                   void* owner) {
  Transforms::Row data = transforms_.TryEmplace(entity);
  if (data) {
    CHECK_EQ(data.Get<kOwner>(), owner);

    data.Get<kTranslation>() = transform.translation;
    data.Get<kRotation>() = transform.rotation;
    data.Get<kScale>() = transform.scale;
    data.Get<kFlags>().Set(dirty_flag_);
  }
}

void TransformSystem::LockTransform(Entity entity, void* owner) {
  auto data = transforms_.TryEmplace(entity);
  if (data) {
    CHECK(data.Get<kOwner>() == nullptr);
    data.Get<kOwner>() = owner;
  }
}

void TransformSystem::UnlockTransform(Entity entity, void* owner) {
  auto data = transforms_.Find<kOwner>(entity);
  if (data) {
    CHECK(data.Get<kOwner>() == owner);
    data.Get<kOwner>() = nullptr;
  }
}

void TransformSystem::SetBox(Entity entity, Box box) {
  auto data = transforms_.TryEmplace(entity);
  if (data) {
    data.Get<kLocalBoundingBox>() = box;
    data.Get<kFlags>().Set(dirty_flag_);
  }
}

Box TransformSystem::GetEntityAlignedBox(Entity entity) const {
  auto data = transforms_.Find<kLocalBoundingBox>(entity);
  return data ? *data : Box::Empty();
}

Box TransformSystem::GetWorldAlignedBox(Entity entity) const {
  Transforms::Row data = transforms_.FindRow(entity);
  if (data) {
    UpdateRow(data);
    return data.Get<kWorldBoundingBox>();
  }
  return Box::Empty();
}

void TransformSystem::SetFromTransformDef(Entity entity,
                                          const TransformDef& def) {
  if (entity == kNullEntity) {
    return;
  }

  Transforms::Row data = transforms_.TryEmplace(entity);
  data.Get<kTranslation>() = def.translation;
  data.Get<kRotation>() = def.rotation;
  data.Get<kScale>() = def.scale;
  data.Get<kLocalBoundingBox>() = def.box;
  UpdateRow(data);
}

void TransformSystem::RemoveTransform(Entity entity) {
  transforms_.Erase(entity);
}

void TransformSystem::OnDestroy(Entity entity) { RemoveTransform(entity); }

TransformSystem::TransformFlags TransformSystem::RequestFlag() {
  constexpr int kNumBits = 8 * sizeof(TransformFlags);
  for (uint32_t i = 0; i < kNumBits; ++i) {
    uint32_t flag = 1 << i;
    if (!CheckBits(reserved_flags_, flag)) {
      reserved_flags_ = SetBits(reserved_flags_, flag);
      return TransformFlags(flag);
    }
  }
  LOG(FATAL) << "Ran out of flags";
  return TransformFlags::None();
}

void TransformSystem::ReleaseFlag(TransformFlags flag) {
  CHECK(flag != TransformFlags::None()) << "Cannot release invalid flag.";
  reserved_flags_ = ClearBits(reserved_flags_, flag.Value());
}

void TransformSystem::SetFlag(Entity entity, TransformFlags flag) {
  auto data = transforms_.Find<kFlags>(entity);
  if (data) {
    (*data).Set(flag);
  }
}

void TransformSystem::ClearFlag(Entity entity, TransformFlags flag) {
  auto data = transforms_.Find<kFlags>(entity);
  if (data) {
    (*data).Clear(flag);
  }
}

bool TransformSystem::HasFlag(Entity entity, TransformFlags flag) const {
  auto data = transforms_.Find<kFlags>(entity);
  return data ? (*data).Any(flag) : false;
}

void TransformSystem::UpdateRow(Transforms::Row& data) const {
  if (data.Get<kFlags>().Any(dirty_flag_)) {
    data.Get<kWorldMatrix>() = TransformMatrix(
        data.Get<kTranslation>(), data.Get<kRotation>(), data.Get<kScale>());
    data.Get<kLocalBoundingBox>() = CalculateTransformedBox(
        data.Get<kWorldMatrix>(), data.Get<kLocalBoundingBox>());
    data.Get<kFlags>().Clear(dirty_flag_);
  }
}

}  // namespace redux
