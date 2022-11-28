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

#ifndef REDUX_SYSTEMS_TEXT_TEXT_SYSTEM_H_
#define REDUX_SYSTEMS_TEXT_TEXT_SYSTEM_H_

#include "absl/container/flat_hash_map.h"
#include "redux/engines/render/texture.h"
#include "redux/engines/text/text_engine.h"
#include "redux/modules/ecs/system.h"
#include "redux/systems/text/text_def_generated.h"

namespace redux {

// Manages the Mesh and Texture of an Entity (via the RenderSystem) in order to
// display text.
class TextSystem : public System {
 public:
  explicit TextSystem(Registry* registry);

  void OnRegistryInitialize();

  // Sets the text for an Entity to display.
  void SetText(Entity entity, const std::string& text);

  // Returns the text being displayed by the Entity.
  std::string_view GetText(Entity entity) const;

  // Sets the font to use for the text.
  void SetFont(Entity entity, FontPtr font);

  // Sets the font size of the font being used to render the text.
  void SetFontSize(Entity entity, float size);

  // Sets the height of the text.
  void SetLineHeightScale(Entity entity, float line_height_scale);

  // Sets the bounds within which the text will be formatted.
  void SetBounds(Entity entity, const vec2& bounds);

  // Sets how the text will be wrapped within its bounds.
  void SetWrapMode(Entity entity, TextWrapMode wrap_mode);

  // Sets how the text will be horizontally aligned within its bounds.
  void SetHorizontalAlignment(Entity entity,
                              HorizontalTextAlignment horizontal);

  // Sets how the text will be vertically aligned within its bounds.
  void SetVerticalAlignment(Entity entity, VerticalTextAlignment vertical);

  // Sets the direction in which the text will be displayed.
  void SetTextDirection(Entity entity, TextDirection direction);

  // Updates the RenderSystem with the text Entities' Meshes and Textures.
  // Note: this function is automatically bound to the Choreographer if it is
  // available.
  void PrepareToRender();

  // Returns the underlying glyph texture associated with the font.
  TexturePtr GetTexture(const FontPtr& font);

 private:
  void SetFromTextDef(Entity entity, const TextDef& def);
  void OnDestroy(Entity entity) override;
  void GenerateText(Entity entity);

  struct FontTexture {
    TexturePtr texture;
    // Tracks the number of glyphs in a texture. When new glyphs are added,
    // we need to upload the texture to the GPU, effectively using a more
    // "current" texture.
    size_t texture_generation_id = 0;
  };

  struct TextComponent {
    std::string text;
    TextParams params;
  };

  TextEngine* engine_ = nullptr;
  absl::flat_hash_map<Entity, TextComponent> components_;
  absl::flat_hash_map<HashValue, FontTexture> font_textures_;
  absl::flat_hash_set<Entity> dirty_set_;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::TextSystem);

#endif  // REDUX_SYSTEMS_TEXT_TEXT_SYSTEM_H_
