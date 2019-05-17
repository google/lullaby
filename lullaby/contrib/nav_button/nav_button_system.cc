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

#include "lullaby/contrib/nav_button/nav_button_system.h"

#include "lullaby/modules/animation_channels/render_channels.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/text/text_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/logging.h"
#include "lullaby/generated/nav_button_def_generated.h"

namespace lull {
namespace {
constexpr HashValue kSetTextVariantHash = ConstHash("text");
constexpr HashValue kSetLiteralVariantHash = ConstHash("literal");
constexpr HashValue kSetIconVariantHash = ConstHash("icon");
constexpr HashValue kNavButtonDefHash = ConstHash("NavButtonDef");

}  // namespace

NavButtonSystem::NavButtonSystem(Registry* registry)
    : System(registry), buttons_(1) {
  RegisterDef<NavButtonDefT>(this);
  RegisterDependency<AnimationSystem>(this);
  RegisterDependency<DispatcherSystem>(this);
  RegisterDependency<RenderSystem>(this);
  RegisterDependency<TransformSystem>(this);
}

void NavButtonSystem::Create(Entity entity, HashValue type, const Def* def) {
  if (type != kNavButtonDefHash) {
    LOG(DFATAL) << "Invalid type passed to Create. Expecting NavButtonDef!";
    return;
  }
  NavButton* button = buttons_.Emplace(entity);
  const auto* data = ConvertDef<NavButtonDef>(def);

  auto* render_system = registry_->Get<RenderSystem>();
  auto* text_system = registry_->Get<TextSystem>();
  auto* transform_system = registry_->Get<TransformSystem>();

  if (data->background_blueprint()) {
    button->background = transform_system->CreateChild(
        entity, data->background_blueprint()->c_str());

    if (data->background()) {
      render_system->SetTexture(button->background, 0,
                                data->background()->c_str());
    }
    if (data->background_color_hex()) {
      MathfuVec4FromFbColorHex(data->background_color_hex()->c_str(),
                               &button->background_color);
      render_system->SetColor(button->background, button->background_color);
      render_system->SetDefaultColor(button->background,
                                     button->background_color);
    }
    if (data->background_hover_color_hex()) {
      MathfuVec4FromFbColorHex(data->background_hover_color_hex()->c_str(),
                               &button->background_hover_color);
    }
  }

  if (data->icon_blueprint()) {
    button->icon =
        transform_system->CreateChild(entity, data->icon_blueprint()->c_str());
    if (data->icon()) {
      render_system->SetTexture(button->icon, 0, data->icon()->c_str());
    }
    if (data->icon_shader()) {
      render_system->SetShader(button->icon, data->icon_shader()->c_str());
    }
    if (data->icon_color_hex()) {
      MathfuVec4FromFbColorHex(data->icon_color_hex()->c_str(),
                               &button->icon_color);
      render_system->SetColor(button->icon, button->icon_color);
      render_system->SetDefaultColor(button->icon, button->icon_color);
    }
    if (data->icon_hover_color_hex()) {
      MathfuVec4FromFbColorHex(data->icon_hover_color_hex()->c_str(),
                               &button->icon_hover_color);
    }
  }

  if (data->label_blueprint()) {
    button->label =
        transform_system->CreateChild(entity, data->label_blueprint()->c_str());
    if (data->text()) {
      text_system->SetText(button->label, data->text()->str());
    }
    if (data->label_shader()) {
      render_system->SetShader(button->label, data->label_shader()->c_str());
    }
    if (data->label_color_hex()) {
      MathfuVec4FromFbColorHex(data->label_color_hex()->c_str(),
                               &button->label_color);
      render_system->SetColor(button->label, button->label_color);
      render_system->SetDefaultColor(button->label, button->label_color);
    }
    if (data->label_hover_color_hex()) {
      MathfuVec4FromFbColorHex(data->label_hover_color_hex()->c_str(),
                               &button->label_hover_color);
    }
    if (data->set_text_events()) {
      auto response = [text_system, entity, this](const EventWrapper& event) {
        if (NavButton* button = buttons_.Get(entity)) {
          const std::string* text =
              event.GetValue<std::string>(kSetTextVariantHash);
          if (text != nullptr) {
            bool literal =
                event.GetValueWithDefault<bool>(kSetLiteralVariantHash, false);
            if (literal) {
              text_system->SetText(
                  button->label,
                  *text,
                  TextSystemPreprocessingModes::kPreprocessingModeNone);
            } else {
              text_system->SetText(button->label, *text);
            }
          }
        }
      };
      ConnectEventDefs(registry_, entity, data->set_text_events(), response);
    }
    if (data->set_icon_events()) {
      auto response = [render_system, entity, this](const EventWrapper& event) {
        if (NavButton* button = buttons_.Get(entity)) {
          const std::string* icon =
              event.GetValue<std::string>(kSetIconVariantHash);
          if (icon != nullptr) {
            render_system->SetTexture(button->icon, 0, *icon);
          }
        }
      };
      ConnectEventDefs(registry_, entity, data->set_icon_events(), response);
    }
  }

  button->start_hover_duration =
      std::chrono::milliseconds(data->start_hover_duration_millis());
  button->stop_hover_duration =
      std::chrono::milliseconds(data->stop_hover_duration_millis());

  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  dispatcher_system->Connect(
      entity, this,
      [this](const StartHoverEvent& event) { OnStartHover(event); });
  dispatcher_system->Connect(entity, this, [this](const StopHoverEvent& event) {
    OnStopHover(event);
  });
}

void NavButtonSystem::OnStartHover(const StartHoverEvent& event) {
  if (NavButton* button = buttons_.Get(event.target)) {
    auto* animation_system = registry_->Get<AnimationSystem>();
    auto* dispatcher_system = registry_->Get<DispatcherSystem>();
    if (button->icon) {
      dispatcher_system->Send(button->icon, event);
    }
    if (button->background) {
      dispatcher_system->Send(button->background, event);
      if (button->background_hover_color != button->background_color) {
        animation_system->SetTarget(button->background,
                                    UniformChannel::kColorChannelName,
                                    &button->background_hover_color[0], 4,
                                    button->start_hover_duration);
      }
    }
    if (button->icon) {
      dispatcher_system->Send(button->icon, event);
      if (button->icon_hover_color != button->icon_color) {
        animation_system->SetTarget(
            button->icon, UniformChannel::kColorChannelName,
            &button->icon_hover_color[0], 4, button->start_hover_duration);
      }
    }
    if (button->label) {
      dispatcher_system->Send(button->label, event);
      if (button->label_hover_color != button->label_color) {
        animation_system->SetTarget(
            button->label, UniformChannel::kColorChannelName,
            &button->label_hover_color[0], 4, button->start_hover_duration);
      }
    }
  }
}

void NavButtonSystem::OnStopHover(const StopHoverEvent& event) {
  if (NavButton* button = buttons_.Get(event.target)) {
    auto* animation_system = registry_->Get<AnimationSystem>();
    auto* dispatcher_system = registry_->Get<DispatcherSystem>();
    if (button->icon) {
      dispatcher_system->Send(button->icon, event);
    }
    if (button->background) {
      dispatcher_system->Send(button->background, event);
      if (button->background_hover_color != button->background_color) {
        animation_system->SetTarget(
            button->background, UniformChannel::kColorChannelName,
            &button->background_color[0], 4, button->stop_hover_duration);
      }
    }
    if (button->icon) {
      dispatcher_system->Send(button->icon, event);
      if (button->icon_hover_color != button->icon_color) {
        animation_system->SetTarget(
            button->icon, UniformChannel::kColorChannelName,
            &button->icon_color[0], 4, button->start_hover_duration);
      }
    }
    if (button->label) {
      dispatcher_system->Send(button->label, event);
      if (button->label_hover_color != button->label_color) {
        animation_system->SetTarget(
            button->label, UniformChannel::kColorChannelName,
            &button->label_color[0], 4, button->stop_hover_duration);
      }
    }
  }
}

void NavButtonSystem::Destroy(Entity entity) {
  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  dispatcher_system->Disconnect<StopHoverEvent>(entity, this);
  dispatcher_system->Disconnect<StartHoverEvent>(entity, this);
  buttons_.Destroy(entity);
}

Entity NavButtonSystem::GetLabelEntity(Entity entity) {
  NavButton* button = buttons_.Get(entity);
  return button ? button->label : lull::kNullEntity;
}

}  // namespace lull
