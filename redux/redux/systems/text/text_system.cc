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

#include "redux/systems/text/text_system.h"

#include "redux/engines/render/mesh_factory.h"
#include "redux/engines/render/texture_factory.h"
#include "redux/modules/base/choreographer.h"
#include "redux/systems/render/render_system.h"

namespace redux {

TextSystem::TextSystem(Registry* registry) : System(registry) {
  RegisterDef(&TextSystem::SetFromTextDef);
  RegisterDependency<TextEngine>(this);
  RegisterDependency<RenderSystem>(this);
  auto choreo = registry_->Get<Choreographer>();
  choreo->Add<&TextSystem::PrepareToRender>(Choreographer::Stage::kRender)
      .Before<&RenderEngine::Render>();
}

void TextSystem::OnRegistryInitialize() {
  engine_ = registry_->Get<TextEngine>();
  CHECK(engine_);
}

void TextSystem::SetFromTextDef(Entity entity, const TextDef& def) {
  TextComponent& c = components_[entity];
  c.text = def.text;
  c.params.font = engine_->LoadFont(def.font);
  c.params.font_size = def.font_size;
  c.params.line_height = def.line_height;
  c.params.bounds = def.bounds;
  c.params.horizontal_alignment = def.horizontal_alignment;
  c.params.vertical_alignment = def.vertical_alignment;
  c.params.wrap = def.wrap;
  c.params.text_direction = def.text_direction;
  c.params.language_iso_639 = def.language_iso_639;
  dirty_set_.emplace(entity);
}

void TextSystem::OnDestroy(Entity entity) {
  dirty_set_.erase(entity);
  components_.erase(entity);
}

void TextSystem::PrepareToRender() {
  for (Entity entity : dirty_set_) {
    GenerateText(entity);
  }
  dirty_set_.clear();
}

void TextSystem::SetFont(Entity entity, FontPtr font) {
  auto iter = components_.find(entity);
  if (iter != components_.end()) {
    iter->second.params.font = font;
    dirty_set_.emplace(entity);
  }
}

std::string_view TextSystem::GetText(Entity entity) const {
  auto iter = components_.find(entity);
  return (iter != components_.end()) ? iter->second.text : std::string_view{};
}

void TextSystem::SetText(Entity entity, const std::string& text) {
  auto iter = components_.find(entity);
  if (iter != components_.end()) {
    iter->second.text = text;
    dirty_set_.emplace(entity);
  }
}

TexturePtr TextSystem::GetTexture(const FontPtr& font) {
  FontTexture& iter = font_textures_[font->GetName()];
  TexturePtr& texture = iter.texture;

  const ImageAtlaser& atlas = font->GetGlyphAtlas();
  if (texture == nullptr || texture->GetDimensions() != atlas.GetSize()) {
    if (atlas.GetSize().x > 0 && atlas.GetSize().y > 0) {
      auto texture_factory = registry_->Get<TextureFactory>();
      texture = texture_factory->CreateTexture(
          atlas.GetSize(), ImageFormat::Alpha8, TextureParams());
    }
  }

  if (iter.texture_generation_id != atlas.GetNumSubimages()) {
    texture->Update(atlas.GetImageData());
    iter.texture_generation_id = atlas.GetNumSubimages();
  }

  return texture;
}

static inline vec4 CalculateSdfParams(float font_size,
                                      float sdf_dist_offset = 0.f,
                                      float sdf_dist_scale = 1.f) {
  constexpr float kNominalGlyphSize = 64.f;
  constexpr float kMetersFromMillimeters = 0.001f;
  constexpr float kSoftnessMultiplier = 0.3f;
  constexpr float kThreshold = 0.5f;

  const float softness = kSoftnessMultiplier * kNominalGlyphSize *
                         kMetersFromMillimeters / font_size;
  const float min = Clamp(kThreshold - (0.5f * softness), 0.0f, 1.0f);
  const float max = Clamp(kThreshold + (0.5f * softness), min, 1.0f) + 0.001f;
  return vec4(sdf_dist_offset, sdf_dist_scale, min, max);
}

void TextSystem::GenerateText(Entity entity) {
  auto iter = components_.find(entity);
  if (iter == components_.end()) {
    return;
  }

  const TextParams& params = iter->second.params;
  CHECK(params.font);

  MeshData mesh_data = engine_->GenerateTextMesh(iter->second.text, params);
  auto* mesh_factory = registry_->Get<MeshFactory>();
  MeshPtr mesh = mesh_factory->CreateMesh(std::move(mesh_data));
  TexturePtr texture = GetTexture(params.font);

  auto* render_system = registry_->Get<RenderSystem>();
  render_system->SetMesh(entity, mesh);
  render_system->SetTexture(entity, {MaterialTextureType::Glyph}, texture);
  render_system->SetShadingModel(entity, "text");
  if (texture) {
    render_system->EnableShadingFeature(entity, ConstHash("SDF_TEXT"));

    const vec4 sdf_params = CalculateSdfParams(params.font_size);
    render_system->SetMaterialProperty(entity, ConstHash("SdfParams"),
                                       sdf_params);
  }
}

void TextSystem::SetFontSize(Entity entity, float size) {
  auto iter = components_.find(entity);
  if (iter != components_.end()) {
    iter->second.params.font_size = size;
    dirty_set_.emplace(entity);
  }
}

void TextSystem::SetLineHeightScale(Entity entity, float line_height_scale) {
  auto iter = components_.find(entity);
  if (iter != components_.end()) {
    iter->second.params.line_height = line_height_scale;
    dirty_set_.emplace(entity);
  }
}

void TextSystem::SetBounds(Entity entity, const vec2& bounds) {
  auto iter = components_.find(entity);
  if (iter != components_.end()) {
    iter->second.params.bounds.max = iter->second.params.bounds.min + bounds;
    dirty_set_.emplace(entity);
  }
}

void TextSystem::SetWrapMode(Entity entity, TextWrapMode wrap_mode) {
  auto iter = components_.find(entity);
  if (iter != components_.end()) {
    iter->second.params.wrap = wrap_mode;
    dirty_set_.emplace(entity);
  }
}

void TextSystem::SetHorizontalAlignment(Entity entity,
                                        HorizontalTextAlignment horizontal) {
  auto iter = components_.find(entity);
  if (iter != components_.end()) {
    iter->second.params.horizontal_alignment = horizontal;
    dirty_set_.emplace(entity);
  }
}

void TextSystem::SetVerticalAlignment(Entity entity,
                                      VerticalTextAlignment vertical) {
  auto iter = components_.find(entity);
  if (iter != components_.end()) {
    iter->second.params.vertical_alignment = vertical;
    dirty_set_.emplace(entity);
  }
}

void TextSystem::SetTextDirection(Entity entity, TextDirection direction) {
  auto iter = components_.find(entity);
  if (iter != components_.end()) {
    iter->second.params.text_direction = direction;
    dirty_set_.emplace(entity);
  }
}
}  // namespace redux
