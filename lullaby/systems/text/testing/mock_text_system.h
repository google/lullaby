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

#ifndef LULLABY_SYSTEMS_TEXT_TESTING_MOCK_TEXT_SYSTEM_H_
#define LULLABY_SYSTEMS_TEXT_TESTING_MOCK_TEXT_SYSTEM_H_

#include "lullaby/systems/text/text_system.h"

#include "gmock/gmock.h"

namespace lull {

class MockTextSystem : public TextSystemImpl {
 public:
  explicit MockTextSystem(Registry* registry) : TextSystemImpl(registry) {}

  MOCK_METHOD1(CreateEmpty, void(Entity));
  MOCK_METHOD2(CreateFromRenderDef, void(Entity, const RenderDef&));
  MOCK_METHOD1(LoadFonts, FontPtr(const std::vector<std::string>&));
  MOCK_METHOD2(SetFont, void(Entity, FontPtr));
  MOCK_CONST_METHOD1(GetText, const std::string*(Entity));
  MOCK_CONST_METHOD1(GetRenderedText, const std::string*(Entity));
  MOCK_METHOD2(SetText, void(Entity, const std::string& text));
  MOCK_METHOD2(SetFontSize, void(Entity, float));
  MOCK_METHOD2(SetLineHeightScale, void(Entity, float));
  MOCK_METHOD2(SetBounds, void(Entity, const mathfu::vec2&));
  MOCK_METHOD2(SetWrapMode, void(Entity, TextWrapMode));
  MOCK_METHOD2(SetEllipsis, void(Entity, const std::string& ellipsis));
  MOCK_METHOD2(SetHorizontalAlignment, void(Entity, HorizontalAlignment));
  MOCK_METHOD2(SetVerticalAlignment, void(Entity, VerticalAlignment));
  MOCK_METHOD1(SetTextDirection, void(TextDirection));
  MOCK_METHOD2(SetTextDirection, void(Entity, TextDirection));
  MOCK_CONST_METHOD1(GetLinkTags, const std::vector<LinkTag>*(Entity));
  MOCK_CONST_METHOD1(GetCaretPositions,
                     const std::vector<mathfu::vec3>*(Entity));
  MOCK_CONST_METHOD1(IsTextReady, bool(Entity));
  MOCK_METHOD0(ProcessTasks, void());
  MOCK_METHOD0(WaitForAllTasks, void());
  MOCK_METHOD0(ReprocessAllText, void());
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_TEXT_TESTING_MOCK_TEXT_SYSTEM_H_
