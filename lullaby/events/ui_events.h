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

#ifndef LULLABY_EVENTS_UI_EVENTS_H_
#define LULLABY_EVENTS_UI_EVENTS_H_

#include <string>

#include "lullaby/modules/ecs/entity.h"
#include "lullaby/util/typeid.h"

namespace lull {

struct ButtonClickEvent {
  ButtonClickEvent() {}
  ButtonClickEvent(Entity e, HashValue id) : target(e), id(id) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
    archive(&id, Hash("id"));
  }

  Entity target = kNullEntity;
  HashValue id = 0;
};

struct TextChangedEvent {
  TextChangedEvent() {}
  TextChangedEvent(Entity e, const std::string& text) : target(e), text(text) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
    archive(&text, Hash("text"));
  }

  Entity target = kNullEntity;
  std::string text;
};

struct TextEnteredEvent {
  TextEnteredEvent() {}
  TextEnteredEvent(Entity e, const std::string& text) : target(e), text(text) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
    archive(&text, Hash("text"));
  }

  Entity target = kNullEntity;
  std::string text;
};

struct SliderEvent {
  SliderEvent() {}
  SliderEvent(Entity e, HashValue id, float value)
      : target(e), id(id), value(value) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
    archive(&id, Hash("id"));
    archive(&value, Hash("value"));
  }

  Entity target = kNullEntity;
  HashValue id = 0;
  float value = 0.f;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ButtonClickEvent);
LULLABY_SETUP_TYPEID(lull::TextChangedEvent);
LULLABY_SETUP_TYPEID(lull::TextEnteredEvent);
LULLABY_SETUP_TYPEID(lull::SliderEvent);

#endif  // LULLABY_EVENTS_UI_EVENTS_H_
