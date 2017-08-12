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

#include "lullaby/systems/text_input/text_input_system.h"

#include <algorithm>
#include <vector>

#include "lullaby/events/entity_events.h"
#include "lullaby/events/input_events.h"
#include "lullaby/events/text_events.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/math.h"
#include "lullaby/util/string_preprocessor.h"
#include "lullaby/generated/text_input_def_generated.h"

namespace {

constexpr int kCaretAnimationTimeMs = 500;

size_t GetCaretIndexFromPosition(const std::vector<mathfu::vec3>* positions,
                                 const mathfu::vec3& target) {
  if (!positions || positions->empty()) {
    return 0;
  }

  size_t min_index = 0;
  float min_dist = lull::DistanceBetween(target, (*positions)[0]);

  for (size_t i = 1; i < positions->size(); ++i) {
    const float dist = lull::DistanceBetween(target, (*positions)[i]);
    if (min_dist > dist) {
      min_dist = dist;
      min_index = i;
    }
  }
  return min_index;
}

}  // namespace

namespace lull {
const HashValue kTextInputDefHash = Hash("TextInputDef");
TextInputSystem::TextInput::TextInput(Entity e) : Component(e) {}

TextInputSystem::TextInputSystem(Registry* registry)
    : System(registry), active_input_(0), inputs_(10) {
  RegisterDef(this, kTextInputDefHash);
  RegisterDependency<DispatcherSystem>(this);
  RegisterDependency<RenderSystem>(this);
  RegisterDependency<TransformSystem>(this);
}

void TextInputSystem::Create(Entity e, HashValue type, const Def* def) {
  if (type != kTextInputDefHash) {
    LOG(DFATAL) << "Invalid type passed to Create. Expecting TextInputDef!";
    return;
  }
  auto transform_system = registry_->Get<TransformSystem>();
  auto dispatcher_system = registry_->Get<DispatcherSystem>();
  auto input = inputs_.Emplace(e);

  auto data = ConvertDef<TextInputDef>(def);
  if (data->hint()) {
    input->hint = data->hint()->c_str();
  }

  if (data->caret_entity()) {
    input->caret_entity =
        transform_system->CreateChild(e, data->caret_entity()->c_str());
    transform_system->Disable(input->caret_entity);
  }

  if (data->composing_entity()) {
    input->composing_entity =
        transform_system->CreateChild(e, data->composing_entity()->c_str());
  }

  input->composing_distance = data->composing_distance();
  input->composing_thickness = data->composing_thickness();

  input->deactivate_on_accept = data->deactivate_on_accept();

  SetCaretIndex(e, 0);
  SetComposingIndices(e, 0, 0);

  auto text_ready_handler = [this](const TextReadyEvent& event) {
    UpdateCaret(event.target);
    UpdateComposingIndicator(event.target);
  };
  input->text_ready_connection =
      dispatcher_system->Connect(e, text_ready_handler);

  if (data->is_clipped()) {
    auto aabb_changed_handler = [this](const AabbChangedEvent& event) {
      UpdatePosition(event.target);
    };
    input->aabb_changed_connection =
        dispatcher_system->Connect(e, aabb_changed_handler);
  }

  if (data->activate_immediately()) {
    Activate(e);
  }

  dispatcher_system->Connect(e, this, [this](const ClickEvent& event) {
    auto* input = inputs_.Get(event.target);
    if (!input || input->text.empty()) {
      return;
    }
    const std::vector<mathfu::vec3>* positions =
        registry_->Get<RenderSystem>()->GetCaretPositions(event.target);
    const size_t caret_index_from_position =
        GetCaretIndexFromPosition(positions, event.location);
    SetCaretIndex(event.target, caret_index_from_position);
    SetComposingIndices(event.target, 0, 0);
  });
}

void TextInputSystem::PostCreateInit(Entity e, HashValue type, const Def* def) {
  if (type != kTextInputDefHash) {
    LOG(DFATAL)
        << "Invalid type passed to PostCreateInit. Expecting TextInputDef!";
    return;
  }
  auto* const input = inputs_.Get(e);
  if (!input) {
    LOG(DFATAL) << "No input provided!";
    return;
  }
  auto* const data = ConvertDef<TextInputDef>(def);
  if (data->hint_color()) {
    MathfuVec4FromFbColor(data->hint_color(), &(input->hint_color));
  } else {
    input->hint_color = registry_->Get<RenderSystem>()->GetDefaultColor(e);
  }

  UpdateText(e);
}

void TextInputSystem::Destroy(Entity e) {
  inputs_.Destroy(e);
  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  dispatcher_system->Disconnect<TextReadyEvent>(e, this);
  dispatcher_system->Disconnect<AabbChangedEvent>(e, this);
  dispatcher_system->Disconnect<ClickEvent>(e, this);
}

void TextInputSystem::Activate(Entity e) {
  auto input = inputs_.Get(e);

  Deactivate();

  if (input) {
    UpdateText(e);
    // Update active_input_ after UpdateText to avoid sending TextChangedEvent.
    active_input_ = e;

    if (!caret_animator_) {
      caret_animator_.reset(new PeriodicFunction());
      const Entity caret = input->caret_entity;
      caret_animator_->Set(std::chrono::milliseconds(kCaretAnimationTimeMs),
                           [this, caret]() { ToggleCaretVisibility(caret); });
    }
  }
}

void TextInputSystem::Deactivate() {
  if (active_input_ != kNullEntity) {
    caret_animator_.reset(nullptr);
    Entity e = active_input_;
    active_input_ = kNullEntity;
    // Wont send TextChangedEvent because active_input_ is null.
    UpdateText(e);
  }
}

void TextInputSystem::AdvanceFrame(const Clock::duration& delta_time) {
  auto input_manager = registry_->Get<InputManager>();
  if (!input_manager->IsConnected(InputManager::kKeyboard)) {
    return;
  }

  auto input = inputs_.Get(active_input_);
  if (!input) {
    return;
  }

  auto keys = input_manager->GetPressedKeys(InputManager::kKeyboard);
  for (const std::string& key : keys) {
    if (key == InputManager::kKeyBackspace) {
      input->text.Backspace();
    } else if (key == InputManager::kKeyReturn) {
      input->text.ClearComposingRegion();
      AcceptText(active_input_);
    } else {
      input->text.Insert(key);
    }
  }

  if (!keys.empty()) {
    UpdateText(active_input_);
  }

  if (caret_animator_) {
    caret_animator_->AdvanceFrame(delta_time);
  }
}

void TextInputSystem::AcceptText(Entity e) {
  auto input = inputs_.Get(e);
  if (!input) {
    return;
  }

  if (input->deactivate_on_accept) {
    Deactivate();
  }

  auto dispatcher = registry_->Get<DispatcherSystem>();
  if (dispatcher) {
    const TextEnteredEvent event(e, input->text.str());
    dispatcher->Send(e, event);
  }
}

void TextInputSystem::SendTextChangedEvent() {
  auto input = inputs_.Get(active_input_);
  if (!input) {
    return;
  }

  const TextChangedEvent event(active_input_, input->text.str());
  // Send an event so apps can respond to text changes.
  SendEvent(registry_, active_input_, event);
}

std::string TextInputSystem::GetText(Entity e) const {
  const TextInput* input = inputs_.Get(e);
  return input ? input->text.str() : "";
}

std::string TextInputSystem::GetText() const { return GetText(active_input_); }

void TextInputSystem::SetText(Entity e, const std::string& text) {
  TextInput* input = inputs_.Get(e);
  if (!input) {
    return;
  }

  if (input->text.str() != text) {
    input->text.SetText(text);
    SetCaretIndex(e, input->text.CharSize());
    UpdateText(e);
  }
}

void TextInputSystem::SetText(const std::string& text) {
  SetText(active_input_, text);
}

std::string TextInputSystem::CharAt(Entity e, size_t index) const {
  const TextInput* input = inputs_.Get(e);
  if (!input) {
    return "";
  }

  return input->text.CharAt(index);
}

std::string TextInputSystem::CharAt(size_t index) const {
  return CharAt(active_input_, index);
}

void TextInputSystem::Commit(const std::string& text) {
  TextInput* input = inputs_.Get(active_input_);
  if (!input) {
    return;
  }

  input->text.CommitOrInsert(text);
  UpdateText(active_input_);
}

void TextInputSystem::MoveCaretToEnd(Entity e) {
  TextInput* input = inputs_.Get(e);
  if (!input) {
    return;
  }

  SetCaretIndex(e, input->text.CharSize());
}

size_t TextInputSystem::GetCaretPosition() const {
  const TextInput* input = inputs_.Get(active_input_);
  if (!input) {
    return -1;
  }
  return input->text.GetCaretPosition();
}

void TextInputSystem::SetCaretPosition(size_t index) {
  SetCaretIndex(active_input_, index);
}

void TextInputSystem::GetComposingIndices(Entity e, size_t* start_index,
                                          size_t* end_index) const {
  const TextInput* input = inputs_.Get(e);
  if (!input) {
    return;
  }

  input->text.GetComposingRegion(start_index, end_index);
}

void TextInputSystem::GetComposingIndices(size_t* start_index,
                                          size_t* end_index) const {
  GetComposingIndices(active_input_, start_index, end_index);
}

void TextInputSystem::SetComposingIndices(Entity e, size_t start_index,
                                          size_t end_index) {
  TextInput* input = inputs_.Get(e);
  if (!input) {
    return;
  }

  input->text.SetComposingRegion(start_index, end_index);

  UpdateComposingIndicator(e);
}

void TextInputSystem::SetComposingIndices(size_t start_index,
                                          size_t end_index) {
  SetComposingIndices(active_input_, start_index, end_index);
}

void TextInputSystem::ClearComposingRegion(Entity e) {
  SetComposingIndices(e, 0, 0);
}

void TextInputSystem::ClearComposingRegion() {
  ClearComposingRegion(active_input_);
}

void TextInputSystem::UpdateComposingIndicator(Entity e) {
  TextInput* input = inputs_.Get(e);
  if (!input) {
    return;
  }

  if (input->composing_entity == kNullEntity) {
    return;
  }

  auto* transform_system = registry_->Get<TransformSystem>();

  size_t start_index, end_index;
  input->text.GetComposingRegion(&start_index, &end_index);

  if (end_index - start_index > 0) {
    transform_system->Enable(input->composing_entity);

    // TODO(b/33593470) support deformation in TextInputSystem
    // This applies to more than just the composing indicator,
    // including the cursor rendering and click handling.
    auto composing_mesh_fn = [this, e, input, start_index,
                              end_index](MeshData* mesh) {
      auto* render_system = registry_->Get<RenderSystem>();
      const std::vector<mathfu::vec3>* caret_positions =
          render_system->GetCaretPositions(e);

      if (!caret_positions) {
        return;
      }

      VertexP v;
      const float half_thickness = input->composing_thickness * .5f;

      SetPosition(&v, (*caret_positions)[start_index]);
      v.y = -input->composing_distance + half_thickness;
      mesh->AddVertex(v);

      v.y = -input->composing_distance - half_thickness;
      mesh->AddVertex(v);

      SetPosition(&v, (*caret_positions)[end_index]);
      v.y = -input->composing_distance + half_thickness;
      mesh->AddVertex(v);

      v.y = -input->composing_distance - half_thickness;
      mesh->AddVertex(v);

      mesh->AddIndex(0);
      mesh->AddIndex(1);
      mesh->AddIndex(2);
      mesh->AddIndex(2);
      mesh->AddIndex(1);
      mesh->AddIndex(3);
    };

    registry_->Get<RenderSystem>()->UpdateDynamicMesh(
        input->composing_entity, MeshData::PrimitiveType::kTriangles,
        VertexP::kFormat, 4, 6, composing_mesh_fn);
  } else {
    transform_system->Disable(input->composing_entity);
  }
}

std::string TextInputSystem::GetHint(Entity e) const {
  const TextInput* input = inputs_.Get(e);
  return input ? input->hint : "";
}

void TextInputSystem::SetHint(Entity e, const std::string& hint) {
  TextInput* input = inputs_.Get(e);
  if (!input) {
    return;
  }

  if (input->hint != hint) {
    input->hint = hint;
    if (input->text.empty()) UpdateText(e);
  }
}

void TextInputSystem::UpdateText(Entity e) {
  TextInput* input = inputs_.Get(e);
  if (!input) {
    return;
  }

  if (e == active_input_) {
    SendTextChangedEvent();

    // When text is updated, the cursor blink animation should reset.  This is a
    // common idiom and helps the user track the cursor while things change.
    if (caret_animator_) {
      caret_animator_->ResetTimer();
    }
  }

  auto render_system = registry_->Get<RenderSystem>();
  // Show hint if the text is empty.
  if (input->text.empty()) {
    render_system->SetText(e, input->hint);
    render_system->SetColor(e, input->hint_color);
  } else {
    auto* const string_preprocessor = registry_->Get<StringPreprocessor>();
    if (string_preprocessor) {
      render_system->SetText(
          e,
          StringPreprocessor::kLiteralStringPrefixString + input->text.str());
    } else {
      render_system->SetText(e, input->text.str());
    }
    render_system->SetColor(e, render_system->GetDefaultColor(e));
  }
}

void TextInputSystem::SetCaretIndex(Entity e, size_t index) {
  TextInput* input = inputs_.Get(e);
  if (!input) {
    return;
  }
  input->text.SetCaretPosition(index);
  UpdateCaret(e);
}

void TextInputSystem::UpdateCaret(Entity e) {
  TextInput* input = inputs_.Get(e);
  if (!input) {
    return;
  }

  auto transform_system = registry_->Get<TransformSystem>();
  if (e != active_input_) {
    transform_system->Disable(input->caret_entity);
    return;
  }

  auto* render_system = registry_->Get<RenderSystem>();
  const std::vector<mathfu::vec3>* caret_positions =
      render_system->GetCaretPositions(e);
  if (!caret_positions || caret_positions->empty() ||
      input->text.GetCaretPosition() >= caret_positions->size() ||
      input->caret_entity == kNullEntity) {
    return;
  }

  Sqt sqt;
  sqt.translation = (*caret_positions)[input->text.GetCaretPosition()];
  transform_system->SetSqt(input->caret_entity, sqt);
  transform_system->Enable(input->caret_entity);
}

void TextInputSystem::ToggleCaretVisibility(Entity e) {
  auto transform_system = registry_->Get<TransformSystem>();
  if (transform_system->IsEnabled(e)) {
    transform_system->Disable(e);
  } else {
    transform_system->Enable(e);
  }
}

void TextInputSystem::UpdatePosition(Entity e) {
  auto transform_system = registry_->Get<TransformSystem>();
  Entity parent = transform_system->GetParent(e);

  const Sqt* sqt = transform_system->GetSqt(e);
  const Aabb* parent_aabb = transform_system->GetAabb(parent);
  const Aabb* entity_aabb = transform_system->GetAabb(e);
  if (!sqt || !entity_aabb || !parent_aabb) {
    return;
  }

  const mathfu::vec2 parent_size =
      parent_aabb->max.xy() - parent_aabb->min.xy();
  const mathfu::vec2 entity_size =
      entity_aabb->max.xy() - entity_aabb->min.xy();

  // Change the X coord of the text relative to the parent clip entity.
  Sqt new_sqt = *sqt;
  if (entity_size.x < parent_size.x) {
    new_sqt.translation.x = parent_aabb->min.x;
  } else {
    new_sqt.translation.x = parent_aabb->max.x - entity_size.x;
  }

  // Account for caret width - otherwise the caret isn't fully displayed when
  // in the first/left position.
  TextInput* input = inputs_.Get(e);
  if (input && (input->caret_entity != kNullEntity)) {
    const Aabb* caret_aabb = transform_system->GetAabb(input->caret_entity);
    if (caret_aabb) {
      const float caret_width = caret_aabb->max.x - caret_aabb->min.x;
      new_sqt.translation.x = new_sqt.translation.x + caret_width;
    }
  }

  transform_system->SetSqt(e, new_sqt);
}

bool TextInputSystem::HasActiveInput() const {
  return active_input_ != kNullEntity;
}

void TextInputSystem::Insert(const char* utf8_cstr) {
  if (!utf8_cstr || !utf8_cstr[0]) {
    return;
  }
  TextInput* input = inputs_.Get(active_input_);
  if (!input) {
    return;
  }
  input->text.Insert(utf8_cstr);
  UpdateText(active_input_);
}

void TextInputSystem::Insert(const std::string& utf8_str) {
  Insert(utf8_str.c_str());
}

bool TextInputSystem::Backspace() {
  TextInput* input = inputs_.Get(active_input_);
  if (!input) {
    return false;
  }
  if (input->text.Backspace()) {
    UpdateText(active_input_);
    return true;
  }
  return false;
}

}  // namespace lull
