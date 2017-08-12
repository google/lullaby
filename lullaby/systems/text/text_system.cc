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

#include "lullaby/systems/text/text_system.h"

#include "lullaby/modules/script/function_binder.h"
#include "lullaby/util/string_preprocessor.h"

namespace lull {

namespace {

std::vector<std::string> StdFromFbStrings(
    const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>* list) {
  std::vector<std::string> result;
  if (!list) {
    return result;
  }

  result.reserve(list->size());
  for (unsigned int i = 0; i < list->size(); ++i) {
    result.emplace_back(list->Get(i)->c_str());
  }

  return result;
}

}  // namespace

TextSystem::TextSystem(Registry* registry, std::unique_ptr<TextSystemImpl> impl)
    : System(registry), impl_(std::move(impl)) {
  RegisterDef(this, Hash("TextDef"));

  FunctionBinder* binder = registry->Get<FunctionBinder>();
  if (binder) {
    binder->RegisterFunction(
        "lull.Text.SetText",
        [this](Entity e, const std::string& text, int preprocess) {
          SetText(e, text,
                  static_cast<TextSystemPreprocessingModes>(preprocess));
        });

    // Expose enums for use in scripts.  These are functions you will need to
    // call (with parentheses)
    binder->RegisterFunction("lull.Text.PreprocessingMode.None", []() {
      return static_cast<int>(
          TextSystemPreprocessingModes::kPreprocessingModeNone);
    });
    binder->RegisterFunction(
        "lull.Text.PreprocessingMode.StringPreprocessor", []() {
          return static_cast<int>(TextSystemPreprocessingModes::
                                      kPreprocessingModeStringPreprocessor);
        });
  }
}

TextSystem::~TextSystem() {
  FunctionBinder* binder = registry_->Get<FunctionBinder>();
  if (binder) {
    binder->UnregisterFunction("lull.Text.SetText");
    binder->UnregisterFunction("lull.Text.PreprocessingMode.None");
    binder->UnregisterFunction(
        "lull.Text.PreprocessingMode.StringPreprocessor");
  }
}

void TextSystem::Initialize() { impl_->Initialize(); }

void TextSystem::Create(Entity entity, DefType type, const Def* def) {
  impl_->Create(entity, type, def);
}

void TextSystem::PostCreateInit(Entity entity, DefType type, const Def* def) {
  impl_->PostCreateInit(entity, type, def);
}

void TextSystem::Destroy(Entity entity) { impl_->Destroy(entity); }

void TextSystem::CreateEmpty(Entity entity) { impl_->CreateEmpty(entity); }

void TextSystem::CreateFromRenderDef(Entity entity,
                                     const RenderDef& render_def) {
  impl_->CreateFromRenderDef(entity, render_def);
}

FontPtr TextSystem::LoadFonts(const std::vector<std::string>& names) {
  return impl_->LoadFonts(names);
}

FontPtr TextSystem::LoadFonts(
    const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>*
        names) {
  return impl_->LoadFonts(names);
}

void TextSystem::SetFont(Entity entity, FontPtr font) {
  impl_->SetFont(entity, std::move(font));
}

const std::string* TextSystem::GetText(Entity entity) const {
  return impl_->GetText(entity);
}

void TextSystem::SetText(Entity entity, const std::string& text,
                         TextSystemPreprocessingModes preprocess) {
  switch (preprocess) {
    case kPreprocessingModeNone:
      // Adding the kLiteralStringPrefixString causes the preprocessor to ignore
      // the string.
      impl_->SetText(entity, GetUnprocessedText(text));
      break;
    case kPreprocessingModeStringPreprocessor:
      impl_->SetText(entity, text);
      break;
  }
}

void TextSystem::SetFontSize(Entity entity, float size) {
  impl_->SetFontSize(entity, size);
}

void TextSystem::SetLineHeight(Entity entity, float height) {
  SetFontSize(entity, height);
}

void TextSystem::SetBounds(Entity entity, const mathfu::vec2& bounds) {
  impl_->SetBounds(entity, bounds);
}

void TextSystem::SetWrapMode(Entity entity, TextWrapMode wrap_mode) {
  impl_->SetWrapMode(entity, wrap_mode);
}

void TextSystem::SetEllipsis(Entity entity, const std::string& ellipsis) {
  impl_->SetEllipsis(entity, ellipsis);
}

void TextSystem::SetHorizontalAlignment(Entity entity,
                                        HorizontalAlignment horizontal) {
  impl_->SetHorizontalAlignment(entity, horizontal);
}

void TextSystem::SetTextDirection(TextDirection direction) {
  impl_->SetTextDirection(direction);
}

void TextSystem::SetVerticalAlignment(Entity entity,
                                      VerticalAlignment vertical) {
  impl_->SetVerticalAlignment(entity, vertical);
}

const std::vector<LinkTag>* TextSystem::GetLinkTags(Entity entity) const {
  return impl_->GetLinkTags(entity);
}

const std::vector<mathfu::vec3>* TextSystem::GetCaretPositions(
    Entity entity) const {
  return impl_->GetCaretPositions(entity);
}

bool TextSystem::IsTextReady(Entity entity) const {
  return impl_->IsTextReady(entity);
}

void TextSystem::ProcessTasks() { impl_->ProcessTasks(); }

void TextSystem::WaitForAllTasks() { impl_->WaitForAllTasks(); }

TextSystemImpl* TextSystem::GetImpl() { return impl_.get(); }

std::string TextSystem::GetUnprocessedText(const std::string& text) {
  return lull::StringPreprocessor::kLiteralStringPrefixString + text;
}

FontPtr TextSystemImpl::LoadFonts(
    const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>*
        names) {
  return LoadFonts(StdFromFbStrings(names));
}

}  // namespace lull
