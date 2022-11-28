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

#include "redux/engines/text/text_engine.h"

#include <utility>

#include "redux/engines/text/internal/text_layout.h"
#include "redux/modules/base/asset_loader.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/base/static_registry.h"

namespace redux {

std::vector<TextCharacterBreakType> GetBreaks(std::string_view text,
                                              const TextParams& params);

void TextEngine::Create(Registry* registry) {
  registry->Register(std::unique_ptr<TextEngine>(new TextEngine(registry)));
}

TextEngine::TextEngine(Registry* registry) : registry_(registry) {}

FontPtr TextEngine::LoadFont(std::string_view path) {
  const HashValue key = Hash(path);
  auto iter = fonts_.find(key);
  if (iter != fonts_.end()) {
    return iter->second;
  }

  auto asset_loader = registry_->Get<AssetLoader>();
  auto asset = asset_loader->LoadNow(path);
  CHECK(asset.ok()) << "Could not load font: " << path;

  FontPtr font = std::make_shared<Font>(key, std::move(*asset));
  fonts_[key] = font;
  return font;
}

MeshData TextEngine::GenerateTextMesh(std::string_view text,
                                      const TextParams& params) {
  CHECK(params.font);

  static constexpr float kFontRasterizationSize = 48.f;
  GlyphSequence sequence = params.font->GenerateGlyphSequence(
      text, params.language_iso_639, kFontRasterizationSize,
      params.text_direction);

  if (params.wrap != TextWrapMode::kNone) {
    sequence.breaks = GetBreaks(text, params);
  }

  TextLayout layout(params, kFontRasterizationSize);
  return layout.GenerateMesh(text, sequence);
}

static StaticRegistry Static_Register(TextEngine::Create);

}  // namespace redux
