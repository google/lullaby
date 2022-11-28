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

#ifndef REDUX_ENGINES_TEXT_TEXT_ENGINE_H_
#define REDUX_ENGINES_TEXT_TEXT_ENGINE_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "redux/engines/text/font.h"
#include "redux/engines/text/text_enums.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/graphics/mesh_data.h"

namespace redux {

// Information that the TextEngine uses to construct a single piece of text.
struct TextParams {
  FontPtr font;

  float font_size = 0;

  float line_height = 0;

  Bounds2f bounds = {vec2::Zero(), vec2::Zero()};

  HorizontalTextAlignment horizontal_alignment =
      HorizontalTextAlignment::kCenter;

  VerticalTextAlignment vertical_alignment = VerticalTextAlignment::kBaseline;

  TextWrapMode wrap = TextWrapMode::kNone;

  TextDirection text_direction = TextDirection::kLanguageDefault;

  std::string language_iso_639;
};

// Manages Font objects and uses them to generate image and mesh data for text
// rendering.
class TextEngine {
 public:
  static void Create(Registry* registry);

  TextEngine(const TextEngine&) = delete;
  TextEngine& operator=(const TextEngine&) = delete;

  FontPtr LoadFont(std::string_view path);

  MeshData GenerateTextMesh(std::string_view text, const TextParams& params);

 protected:
  explicit TextEngine(Registry* registry);

  Registry* registry_ = nullptr;
  absl::flat_hash_map<HashValue, FontPtr> fonts_;
};
}  // namespace redux

REDUX_SETUP_TYPEID(redux::TextEngine);

#endif  // REDUX_ENGINES_TEXT_TEXT_ENGINE_H_
