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

#ifndef LULLABY_SYSTEMS_TEXT_FLATUI_TEXT_COMPONENT_H_
#define LULLABY_SYSTEMS_TEXT_FLATUI_TEXT_COMPONENT_H_

#include <string>
#include <vector>

#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/systems/text/flatui/font.h"
#include "lullaby/systems/text/flatui/text_buffer.h"
#include "lullaby/util/math.h"
#include "lullaby/generated/text_def_generated.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

// TextComponent contains the data for rendering an entity's text using the
// RenderSystem.  This is a private class.
struct TextComponent : Component {
  std::string text;
  FontPtr font = nullptr;
  TextBufferPtr buffer = nullptr;
  bool loading_buffer = false;
  float edge_softness = 0.3f;
  TextBufferParams text_buffer_params;
  std::string link_text_blueprint;
  std::string link_underline_blueprint;
  std::string caret_blueprint;
  std::vector<Entity> plain_entities;
  std::vector<Entity> link_entities;
  Entity underline_entity = kNullEntity;
  Dispatcher::ScopedConnection on_hidden;
  Dispatcher::ScopedConnection on_unhidden;

  explicit TextComponent(Entity entity) : Component(entity) {}
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_TEXT_FLATUI_TEXT_COMPONENT_H_
