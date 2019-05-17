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

#ifndef LULLABY_SYSTEMS_TEXT_TESTING_STUB_TEXT_SYSTEM_H_
#define LULLABY_SYSTEMS_TEXT_TESTING_STUB_TEXT_SYSTEM_H_

#include "lullaby/systems/text/text_system.h"

namespace lull {

class StubTextSystem : public TextSystemImpl {
 public:
  explicit StubTextSystem(Registry* registry)
      : TextSystemImpl(registry) {}

  void CreateEmpty(Entity entity) override {}
  void CreateFromRenderDef(Entity entity,
                           const RenderDef& render_def) override {}
  FontPtr LoadFonts(const std::vector<std::string>& names) override {
    return nullptr;
  }
  void SetFont(Entity entity, FontPtr font) override {}
  const std::string* GetText(Entity entity) const override { return nullptr; }
  void SetText(Entity entity, const std::string& text) override {}
  void SetFontSize(Entity entity, float size) override {}
  void SetLineHeightScale(Entity entity, float line_height_scale) override {}
  void SetBounds(Entity entity, const mathfu::vec2& bounds) override {}
  void SetWrapMode(Entity entity, TextWrapMode wrap_mode) override {}
  void SetEllipsis(Entity entity, const std::string& ellipsis) override {}
  void SetHorizontalAlignment(Entity entity,
                              HorizontalAlignment horizontal) override {}
  void SetVerticalAlignment(Entity entity,
                            VerticalAlignment vertical) override {}
  void SetTextDirection(TextDirection direction) override {}
  void SetTextDirection(Entity entity, TextDirection direction) override {}
  const std::vector<LinkTag>* GetLinkTags(Entity entity) const override {
    return nullptr;
  }
  const std::vector<mathfu::vec3>* GetCaretPositions(
      Entity entity) const override {
    return nullptr;
  }
  const std::string* GetRenderedText(Entity entity) const override {
    return nullptr;
  }
  bool IsTextReady(Entity entity) const override { return false; }
  void ProcessTasks() override {}
  void WaitForAllTasks() override {}
  void ReprocessAllText() override {}
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_TEXT_TESTING_STUB_TEXT_SYSTEM_H_
