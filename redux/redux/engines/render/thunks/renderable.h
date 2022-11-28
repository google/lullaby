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

#ifndef REDUX_ENGINES_RENDER_THUNKS_RENDERABLE_H_
#define REDUX_ENGINES_RENDER_THUNKS_RENDERABLE_H_

#include "redux/engines/render/renderable.h"

namespace redux {

// Thunk functions to call the actual implementation.
void Renderable::PrepareToRender(const mat4& transform) {
  Upcast(this)->PrepareToRender(transform);
}
void Renderable::SetMesh(MeshPtr mesh) {
  Upcast(this)->SetMesh(std::move(mesh));
}
MeshPtr Renderable::GetMesh() const { return Upcast(this)->GetMesh(); }
void Renderable::EnableVertexAttribute(VertexUsage usage) {
  Upcast(this)->EnableVertexAttribute(usage);
}
void Renderable::DisableVertexAttribute(VertexUsage usage) {
  Upcast(this)->DisableVertexAttribute(usage);
}
bool Renderable::IsVertexAttributeEnabled(VertexUsage usage) const {
  return Upcast(this)->IsVertexAttributeEnabled(usage);
}
void Renderable::SetShader(ShaderPtr shader, std::optional<HashValue> part) {
  Upcast(this)->SetShader(std::move(shader), part);
}
void Renderable::Show(std::optional<HashValue> part) {
  Upcast(this)->Show(part);
}
void Renderable::Hide(std::optional<HashValue> part) {
  Upcast(this)->Hide(part);
}
bool Renderable::IsHidden(std::optional<HashValue> part) const {
  return Upcast(this)->IsHidden(part);
}
void Renderable::SetTexture(TextureUsage usage, const TexturePtr& texture) {
  Upcast(this)->SetTexture(usage, texture);
}
TexturePtr Renderable::GetTexture(TextureUsage usage) const {
  return Upcast(this)->GetTexture(usage);
}
void Renderable::SetProperty(HashValue name, MaterialPropertyType type,
                             absl::Span<const std::byte> data) {
  Upcast(this)->SetProperty(name, type, data);
}
void Renderable::SetProperty(HashValue name, HashValue part,
                             MaterialPropertyType type,
                             absl::Span<const std::byte> data) {
  Upcast(this)->SetProperty(name, part, type, data);
}

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_THUNKS_RENDERABLE_H_
