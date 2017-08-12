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

#ifndef LULLABY_SYSTEMS_TEXT_INPUT_TEXT_INPUT_SYSTEM_H_
#define LULLABY_SYSTEMS_TEXT_INPUT_TEXT_INPUT_SYSTEM_H_

#include "lullaby/events/ui_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/systems/text_input/edit_text.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/periodic_function.h"
#include "lullaby/util/utf8_string.h"

namespace lull {

// The TextInputSystem listens for input from the input
// manager and updates assocated Entity display text.
// Right now the system is very rudimentary.
// TODO(b/27169628) make the system more sophisticated.
class TextInputSystem : public System {
 public:
  explicit TextInputSystem(Registry* registry);

  void Create(Entity e, HashValue type, const Def* def) override;

  void PostCreateInit(Entity e, HashValue type, const Def* def) override;

  void Destroy(Entity e) override;

  // Call once per frame. Queries InputManager for currently pressed keys and
  // updates the display text accordingly.
  void AdvanceFrame(const Clock::duration& delta_time);

  // Sets specified text input component to be active.  The active input listens
  // for keyboard or voice inputs.
  void Activate(Entity e);

  // Clears any active text input entity. No-op if no active text input entity.
  void Deactivate();

  // Gets the text currently displayed in the given input field.
  std::string GetText(Entity e) const;

  // Gets the text of the current active input field. Returns empty string
  // if no active input field.
  std::string GetText() const;

  // Gets UTF8 character at a given index.
  std::string CharAt(Entity e, size_t index) const;
  std::string CharAt(size_t index) const;

  // Sets the text to be displayed in the active input field.
  void SetText(Entity e, const std::string& text);

  // Sets the text to the current active input field. No-op if no active text
  // input entity.
  void SetText(const std::string& text);

  // Commits composing text.  If the composing region is empty,
  // inserts the text where the caret is at.
  void Commit(const std::string& text);

  // Gets the text currently used as hint.
  std::string GetHint(Entity e) const;

  // Sets the text to be used as hint.
  void SetHint(Entity e, const std::string& hint);

  // Moves the caret to the last caret index.
  void MoveCaretToEnd(Entity e);

  // Gets caret position of current active input field. Returns -1 if no
  // active input field.
  size_t GetCaretPosition() const;

  // Sets caret position of current active input field. No-op if no active
  // input field.
  void SetCaretPosition(size_t index);

  // Gets the highlight start and end index.
  void GetComposingIndices(
      Entity e, size_t* start_index, size_t* end_index) const;

  // Gets composing region of current active input field. No-op if no active
  // input field.
  void GetComposingIndices(size_t* start_index, size_t* end_index) const;

  // Sets the start and end indices for a range of text that is the current
  // word being composed (and linked to any android keyboard suggestions).
  // The characters highlighted will include start_index through end_index - 1,
  // so that if start_index and end_index match, nothing will be highlighted.
  void SetComposingIndices(Entity e, size_t start_index, size_t end_index);

  // Sets composing region for current active input field. No-op if no active
  // input field.
  void SetComposingIndices(size_t start_index, size_t end_index);

  // Clear the composing region and suggestions. Text that was in composing
  // region remains intact.
  void ClearComposingRegion(Entity e);

  // Clear the composing region. No-op if no active input field.
  void ClearComposingRegion();

  // Makes the current text the accepted text and broadcasts the appropriate
  // events.
  void AcceptText(Entity e);

  // Returns true if there is active input field currrently.
  bool HasActiveInput() const;

  // Inserts given text at where the caret is. No-op if no active input field.
  void Insert(const char* utf8_cstr);
  void Insert(const std::string& utf8_str);

  // Updates text when backspace is pressed. Typically the character before
  // where the caret is will be deleted. No-op if no active input field.
  // Returns true if some text has been deleted, false otherwise.
  bool Backspace();

 private:
  // Indicates the position at end of the text.
  static constexpr int kSelectionEnd = -1;

  struct TextInput : Component {
    explicit TextInput(Entity e);

    bool deactivate_on_accept = false;
    EditText text;
    std::string hint;
    mathfu::vec4 hint_color;
    Entity caret_entity = kNullEntity;
    Entity composing_entity = kNullEntity;
    float composing_distance = 0.f;
    float composing_thickness = 0.f;
    bool is_clipped = false;
    Dispatcher::ScopedConnection text_ready_connection;
    Dispatcher::ScopedConnection aabb_changed_connection;
  };

  void SendTextChangedEvent();
  void UpdateText(Entity e);
  void SetCaretIndex(Entity e, size_t index);
  void UpdateCaret(Entity e);
  void UpdateComposingIndicator(Entity e);
  void UpdatePosition(Entity e);
  void ToggleCaretVisibility(Entity e);

  Entity active_input_;
  ComponentPool<TextInput> inputs_;

  // Blinks the cursor when text input is active.
  std::unique_ptr<PeriodicFunction> caret_animator_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::TextInputSystem);

#endif  // LULLABY_SYSTEMS_TEXT_INPUT_TEXT_INPUT_SYSTEM_H_
