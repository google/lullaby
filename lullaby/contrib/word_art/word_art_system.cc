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

#include "lullaby/contrib/word_art/word_art_system.h"

#include <cctype>
#include <utility>

#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "lullaby/generated/dispatcher_def_generated.h"
#include "lullaby/events/render_events.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/audio/audio_system.h"
#include "lullaby/systems/model_asset/model_asset_system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/util/filename.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/util/random_number_generator.h"
#include "lullaby/generated/transform_def_generated.h"
#include "mathfu/constants.h"
#include "mathfu/io.h"

namespace lull {

const auto kWordArtDefHash = ConstHash("WordArtDef");

namespace {

// The min and max animation delay for characters is scaled linearly
// between 5 and 30 characters.  The max delay is used for the shortest
// string, and the min delay is used for the longest string.  Thus the
// confusing and seemingly backwards min/max labels.
constexpr int kMaxDelayChars = 5;
constexpr int kMinDelayChars = 30;

// Max number of lines for a given word art entity. After this number of lines
// the text will be truncated with an ellipsis.
constexpr int kMaxLineCount = 2;
constexpr char kEllipsis[] = "...";

float smoothstep(float x) {
  x = mathfu::Clamp(x, 0.0f, 1.0f);
  return x * x * x * (x * (x * 6 - 15) + 10);
}

float cfit(float x, float s1, float s2, float d1, float d2) {
  float lerp = mathfu::Clamp((x - s1) / (s2 - s1), 0.0f, 1.0f);
  return d1 + lerp * (d2 - d1);
}

float smoothcfit(float x, float s1, float s2, float d1, float d2) {
  float lerp = smoothstep((x - s1) / (s2 - s1));
  return d1 + lerp * (d2 - d1);
}

// Returns audio file name tagged as audio, so it can be found in custom
// locations.
std::string TaggedAudioFile(const std::string& filename) {
  return "audio:audio/" + filename;
}

// Returns the desired audio load type based on the provided play_audio
// settings.
AudioLoadType GetLoadType(const WordArtPlayAudioDefT& play_audio) {
  if (play_audio.stream) {
    return AudioLoadType::AudioLoadType_Stream;
  }
  return AudioLoadType::AudioLoadType_Preload;
}

// Returns the desired audio playback type based on the provided play_audio
// settings.
AudioPlaybackType GetPlaybackType(const WordArtPlayAudioDefT& play_audio) {
  if (play_audio.stream || play_audio.loop) {
    return AudioPlaybackType::AudioPlaybackType_PlayWhenReady;
  }
  return AudioPlaybackType::AudioPlaybackType_PlayIfReady;
}

}  // namespace

WordArtSystem::WordArtSystem(
    Registry* registry,
    CharacterMeshFilenameFormat character_mesh_filename_format)
    : System(registry),
      components_(16),
      character_mesh_filename_format_(character_mesh_filename_format) {
  RegisterDef<WordArtDefT>(this);
  RegisterDependency<AnimationSystem>(this);
  RegisterDependency<AudioSystem>(this);
  RegisterDependency<RenderSystem>(this);
  RegisterDependency<TransformSystem>(this);
  RegisterDependency<ModelAssetSystem>(this);

  auto* binder = registry->Get<FunctionBinder>();
  if (binder) {
    binder->RegisterMethod("lull.WordArt.SetText", &WordArtSystem::SetText);
  }
}

WordArtSystem::~WordArtSystem() {
  auto* binder = registry_->Get<FunctionBinder>();
  if (binder) {
    binder->UnregisterFunction("lull.WordArt.SetText");
  }
}

void WordArtSystem::CreateComponent(Entity entity, const Blueprint& blueprint) {
  WordArtDefT word_art_def;
  if (blueprint.Read(&word_art_def)) {
    auto* component = components_.Emplace(entity);
    component->model_asset_def = reinterpret_cast<const WordArtDef*>(
        blueprint.GetLegacyDefData())->model_asset_def();
    component->character_width = &character_width_;
    component->mesh_base = word_art_def.mesh_base;
    component->mesh_extension =
        (word_art_def.mesh_extension.empty() ? ".fplmesh"
                                             : word_art_def.mesh_extension);
    component->line_height = word_art_def.line_height;
    component->character_pad = word_art_def.character_pad;
    component->max_width = word_art_def.max_width;
    component->selection_bounding_box_offset =
        word_art_def.selection_bounding_box_offset;
    component->place_behavior = word_art_def.place_behavior;
    component->tap_behavior = word_art_def.tap_behavior;
    component->idle_behavior = word_art_def.idle_behavior;
    component->color_select_behavior = word_art_def.color_select_behavior;
    component->sync_idle = word_art_def.sync_idle;
    component->color = Color4f::ToVec4(word_art_def.color);
    component->color_uniform_name = word_art_def.color_uniform_name;
    component->color_uniform_size = word_art_def.color_uniform_size;
    component->character_mesh_filename_format = character_mesh_filename_format_;

    // Bootstrap character widths.  We only need to do this the first time we
    // load a font, but it should be fast enough that it doesn't matter if we
    // do it every time.
    auto prefix = GetDirectoryFromFilename(word_art_def.mesh_base);
    for (const auto& glyph_info : word_art_def.glyph_info) {
      character_width_[JoinPath(prefix, glyph_info.glyph)] = glyph_info.width;
    }

    LoadAudio(component->place_behavior);
    LoadAudio(component->tap_behavior);
    LoadAudio(component->idle_behavior);

    auto* dispatcher_system = registry_->Get<DispatcherSystem>();
    DCHECK(dispatcher_system);
    if (!word_art_def.place_behavior.event.event.empty()) {
      dispatcher_system->ConnectEvent(
          entity, word_art_def.place_behavior.event,
          [=](const EventWrapper& event) { HandleDropEvent(entity); });
    }
    if (!word_art_def.tap_behavior.event.event.empty()) {
      dispatcher_system->ConnectEvent(
          entity, word_art_def.tap_behavior.event,
          [=](const EventWrapper& event) { HandleTapEvent(entity); });
    }
  }
}

void WordArtSystem::Destroy(Entity entity) { components_.Destroy(entity); }

void WordArtSystem::AdvanceFrame(Clock::duration delta_time) {
  scheduled_processor_.Tick(delta_time);
  UpdateAnimations();
}

bool WordArtSystem::HasWordArt(Entity entity) const {
  return components_.Get(entity) != nullptr;
}

std::vector<Entity> WordArtSystem::SetText(Entity entity,
                                           const std::string& text) {
  std::vector<Entity> result;

  auto* component = components_.Get(entity);
  if (!component) {
    LOG(ERROR) << "No WordArtComponent on entity " << entity;
    return result;
  }

  if (text == component->text) {
    // Populate result with the characters we already have.
    result.reserve(component->characters.size());
    for (const auto& c : component->characters) {
      result.push_back(c.entity);
    }
    return result;
  }

  auto* transform_system = registry_->Get<TransformSystem>();
  DCHECK(transform_system);
  auto* animation_system = registry_->Get<AnimationSystem>();
  DCHECK(animation_system);
  auto* render_system = registry_->Get<RenderSystem>();
  DCHECK(render_system);
  auto* model_asset_system = registry_->Get<ModelAssetSystem>();
  DCHECK(model_asset_system);
  auto* rng = registry_->Get<RandomNumberGenerator>();
  DCHECK(rng);

  component->text = text;

  // TODO: Preserve existing character entities rather than deleting and
  // rebuilding everything with each change.
  auto entity_factory = registry_->Get<EntityFactory>();
  for (const auto& c : component->characters) {
    entity_factory->Destroy(c.entity);
  }
  component->characters.clear();

  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  DCHECK(dispatcher_system);
  const float space_width = component->GetSpaceWidth();
  std::vector<std::string> lines = GetWrappedLines(*component, entity);

  component->text_bounds.x = 0.0f;
  component->text_bounds.y = lines.size() * component->line_height;
  float offset_y =
      static_cast<float>((lines.size() - 1)) * component->line_height;
  for (const auto& text : lines) {
    if (text.empty()) {
      continue;
    }
    float place_time_offset = 0.0f;
    float string_width = component->GetStringWidth(text);
    component->text_bounds.x = std::max(component->text_bounds.x, string_width);
    float offset_x = -0.5f * string_width;
    for (char c : text) {
      auto mesh_file = component->GetCharacterMeshFile(c);
      if (!mesh_file.empty()) {
        const float half_char_width = 0.5f * component->GetCharacterWidth(c);
        // Increment by half of the character width before placement and the
        // other half after.  This compensates for character meshes being
        // centered on local origin.
        offset_x += half_char_width;

        WordArtComponent::Character character;
        character.entity = entity_factory->Create();
        Sqt sqt;
        sqt.translation = mathfu::vec3(offset_x, offset_y, 0.0f);
        transform_system->Create(character.entity, sqt);
        model_asset_system->CreateModel(character.entity, mesh_file,
                                        component->model_asset_def);
        character.char_color = component->color;
        SetCharacterColor(*component, character.entity, component->color);
        component->characters.push_back(character);
        result.push_back(character.entity);

        offset_x += half_char_width;
        offset_x += space_width * component->character_pad;

        // Cue placement animation only if the component hasn't yet been placed.
        // Per discussion with UX, letters added via the keyboard do not play
        // the place animation.
        if (!component->has_been_placed &&
            component->place_behavior.play_anim_file &&
            !component->place_behavior.play_anim_file->files.empty()) {
          const auto& play_anim_file =
              component->place_behavior.play_anim_file.value();
          const float place_delay = cfit(
              text.size(), kMaxDelayChars, kMinDelayChars,
              play_anim_file.max_delay_s, play_anim_file.min_delay_s);
          if (play_anim_file.randomize_delay) {
            place_time_offset = rng->GenerateUniform(
              play_anim_file.max_delay_s, play_anim_file.min_delay_s);
          }
          int anim_index = 0;
          if (play_anim_file.files.size() > 1) {
            anim_index = rng->GenerateUniform(
                0, static_cast<int>(play_anim_file.files.size() - 1));
          }
          auto anim = animation_system->LoadAnimation(
              play_anim_file.files[anim_index]);

          PlaybackParameters params;
          params.speed = 1.0f;
          params.blend_time_s = 0.0f;
          params.looping = false;
          params.start_delay_s = place_time_offset;
          component->channel = play_anim_file.animation_channel;
          animation_system->PlayAnimation(character.entity, component->channel,
                                          anim, params);
          animation_system->SetPlaybackRate(character.entity,
                                            component->channel, 0.0f);
          place_time_offset += place_delay;
        } else {
          // Cue idle animation for newly created characters.  This avoids the
          // characters from popping to the animated location on the next
          // AdvanceFrame.
          SetUpAnimations(component->idle_behavior,
                          static_cast<int>(component->characters.size()) - 1,
                          component);
        }

        transform_system->AddChild(entity, character.entity);
      } else if (c == ' ') {
        offset_x += space_width;
      }
    }
    offset_y -= component->line_height;
  }

  return result;
}

std::string WordArtSystem::WordArtComponent::GetCharacterMeshFile(
    char character) const {
  char char_a;
  char char_z;
  switch (character_mesh_filename_format) {
    case kLowerCaseSuffixes:
      character = static_cast<char>(std::tolower(character));
      char_a = 'a';
      char_z = 'z';
      break;
    case kUpperCaseSuffixes:
    default:
      character = static_cast<char>(std::toupper(character));
      char_a = 'A';
      char_z = 'Z';
  }
  std::string suffix;
  if ((character >= char_a && character <= char_z) ||
      (character >= '0' && character <= '9')) {
    suffix = std::string(1, character);
  } else {
    switch (character) {
      case '\'': suffix = "apostrophe"; break;
      case '@': suffix = "at"; break;
      case ',': suffix = "comma"; break;
      case '!': suffix = "exclamation"; break;
      case '#': suffix = "hashtag"; break;
      case '%': suffix = "percent"; break;
      case '.': suffix = "period"; break;
      case '?': suffix = "question"; break;
      case '-': suffix = "dash"; break;
      default: return std::string();
    }
  }
  // TODO: Figure out why absl::StrCat wouldn't compile on android.
  return mesh_base + "_" + suffix + mesh_extension;
}

float WordArtSystem::WordArtComponent::GetStringWidth(
    absl::string_view str) const {
  const float space_width = GetSpaceWidth();
  float width = 0.0f;
  for (char c : str) {
    if (c == ' ') {
      width += space_width;
    } else if (!GetCharacterMeshFile(c).empty()) {
      width += GetCharacterWidth(c);
      width += space_width * character_pad;
    }
  }

  return width;
}

float WordArtSystem::WordArtComponent::GetCharacterWidth(char character) const {
  std::string mesh_file = GetCharacterMeshFile(character);
  if (mesh_file.empty()) {
    return 0.0f;
  }
  return (*character_width)[mesh_file];
}

float WordArtSystem::WordArtComponent::GetSpaceWidth() const {
  constexpr float kSpaceScale = 0.65f;
  return GetCharacterWidth('0') * kSpaceScale;
}

void WordArtSystem::SetColor(Entity entity, const mathfu::vec4& color,
                             const std::string& color_name, bool animate) {
  auto* component = components_.Get(entity);
  DCHECK(component);
  if (!component->color_select_behavior.color_select) {
    return;
  }

  component->color = color;
  for (const auto& color_map_entry :
       component->color_select_behavior.color_select.value().color_map) {
    if (color_map_entry.text_color.compare(color_name) == 0) {
      component->color = Color4f::ToVec4(color_map_entry.mapped_color);
    }
  }

  if (animate) {
    SetUpAnimations(component->color_select_behavior, -1, component);
  } else {
    StaggerAnimations(
        -1, 0,
        [=](Clock::time_point start_time, WordArtComponent::Character* c) {
          auto* component = components_.Get(entity);
          if (component) {
            c->color_fade.start_time = start_time;
            c->color_fade.end_time = start_time + DurationFromSeconds(0);
            c->color_fade.start_color =
                GetCharacterColor(*component, c->entity);
            c->color_fade.end_color = component->color;
          }
        },
        component);
  }
}

mathfu::vec2 WordArtSystem::GetTextBounds(Entity entity) const {
  auto* component = components_.Get(entity);
  if (component) {
    return component->text_bounds;
  } else {
    LOG(ERROR) << "No WordArtComponent on entity " << entity;
    return mathfu::kZeros2f;
  }
}

mathfu::vec3 WordArtSystem::GetSelectionBoundingBoxOffset(Entity entity) const {
  auto* component = components_.Get(entity);
  if (component) {
    return component->selection_bounding_box_offset;
  } else {
    LOG(ERROR) << "No WordArtComponent on entity " << entity;
    return mathfu::kZeros3f;
  }
}

void WordArtSystem::HandleDropEvent(Entity entity) {
  auto* animation_system = registry_->Get<AnimationSystem>();
  DCHECK(animation_system);

  auto* component = components_.Get(entity);
  DCHECK(component);

  if (component->has_been_placed) {
    return;
  }

  for (const auto& c : component->characters) {
    animation_system->SetPlaybackRate(c.entity, component->channel, 1.0f);
  }

  SetUpAnimations(component->place_behavior, -1, component);

  component->has_been_placed = true;
  component->is_paused = false;
}

void WordArtSystem::HandleTapEvent(Entity entity) {
  auto* component = components_.Get(entity);
  DCHECK(component);

  SetUpAnimations(component->tap_behavior, -1, component);
}

void WordArtSystem::StaggerAnimations(int character_index, float stagger_delay,
    const WordArtSystem::SetUpAnimationFunc& setup_func,
    WordArtComponent* component) {
  int start_index = 0;
  int end_index = static_cast<int>(component->characters.size()) - 1;
  if (character_index >=0) {
    start_index = end_index = character_index;
  }

  auto entity = component->GetEntity();
  const auto now = Clock::now();
  float delay = 0.0f;
  for (int i = start_index; i <= end_index; ++i) {
    // Increment the sync_count and then decrement after the delay has passed to
    // allow animations with sync_idle enabled to know when they can begin.
    ++component->sync_count;
    // Schedule animation with a staggered delay.
    scheduled_processor_.Add(
        [=]() {
          auto* component = components_.Get(entity);
          if (component) {
            --component->sync_count;
            if (static_cast<size_t>(i) < component->characters.size()) {
              setup_func(now, &component->characters[i]);
            }
          }
        },
        DurationFromSeconds(delay));
    delay += stagger_delay;
  }
}

void WordArtSystem::SetUpAnimations(const WordArtBehaviorDefT& behavior,
                                    int character_index,
                                    WordArtComponent* component) {
  if (component->color_select_behavior.color_select && behavior.color_change) {
    LOG(INFO) << "Ignoring color_change behavior since there is a "
                 "color_select behavior defined";
    return;
  }
  if (behavior.color_change) {
    const auto& color_change = behavior.color_change.value();
    auto color_size = color_change.colors.size();
    component->color_index = (component->color_index + 1) % color_size;
    component->color =
        Color4f::ToVec4(color_change.colors[component->color_index]);
    auto fade_duration = DurationFromSeconds(color_change.fade_s);
    auto end_color = component->color;
    auto delay = cfit(component->text.size(), kMaxDelayChars, kMinDelayChars,
                      color_change.max_delay_s, color_change.min_delay_s);
    auto entity = component->GetEntity();

    // Note:  any code executed in the lambda's passed to StaggerAnimations will
    // run at a later time.  Make sure that any objects referenced within the
    // lambdas are still valid.
    StaggerAnimations(
        character_index, delay,
        [=](Clock::time_point start_time, WordArtComponent::Character* c) {
          auto* component = components_.Get(entity);
          if (component) {
            c->color_fade.start_time = start_time;
            c->color_fade.end_time = start_time + fade_duration;
            c->color_fade.start_color =
                GetCharacterColor(*component, c->entity);
            c->color_fade.end_color = end_color;
          }
        },
        component);
  }

  if (behavior.color_scale) {
    const auto& color_scale = behavior.color_scale.value();
    const float scale_range = color_scale.max_scale - color_scale.min_scale;
    const auto scale_duration =
        DurationFromSeconds(color_scale.animation.size() * color_scale.rate_s);
    float scale = color_scale.max_scale;
    std::vector<float> scale_values;
    for (const char c : color_scale.animation) {
      if (c >= 'a' && c <= 'z') {
        scale = static_cast<float>(c - 'a') / 25.0f * (scale_range) +
                color_scale.min_scale;
      }
      scale_values.push_back(scale);
    }
    auto delay = cfit(component->text.size(), kMaxDelayChars, kMinDelayChars,
                      color_scale.max_delay_s, color_scale.min_delay_s);

    StaggerAnimations(character_index, delay,
        [=](Clock::time_point start_time, WordArtComponent::Character *c) {
          c->color_scale.start_time = start_time;
          c->color_scale.end_time = start_time + scale_duration;
          c->color_scale.scale_values = scale_values;
        }, component);
  }

  if (behavior.play_anim_file) {
    auto* animation_system = registry_->Get<AnimationSystem>();
    DCHECK(animation_system);
    auto* rng = registry_->Get<RandomNumberGenerator>();
    DCHECK(rng);
    component->channel = behavior.play_anim_file->animation_channel;

    StaggerAnimations(
        character_index, 0.0f,
        [=](Clock::time_point start_time, WordArtComponent::Character* c) {
          if (animation_system->TimeRemaining(c->entity, component->channel) >
              0.0f) {
            return;
          }
          int anim_index = 0;
          if (behavior.play_anim_file->files.size() > 1) {
            anim_index = rng->GenerateUniform(
                0, static_cast<int>(behavior.play_anim_file->files.size() - 1));
          }
          auto anim = animation_system->LoadAnimation(
              behavior.play_anim_file->files[anim_index]);

          PlaybackParameters params;
          params.speed = 1.0f;
          params.blend_time_s = 0.0f;
          params.looping = false;
          params.start_delay_s = 0.0f;
          animation_system->PlayAnimation(c->entity, component->channel, anim,
                                          params);
        },
        component);
  }

  if (behavior.color_select) {
    const auto& color_select = behavior.color_select.value();
    auto fade_duration = DurationFromSeconds(color_select.fade_s);
    auto end_color = component->color;
    auto delay = cfit(component->text.size(), kMaxDelayChars, kMinDelayChars,
                      color_select.max_delay_s, color_select.min_delay_s);
    auto entity = component->GetEntity();
    StaggerAnimations(
        character_index, delay,
        [=](Clock::time_point start_time, WordArtComponent::Character* c) {
          auto* component = components_.Get(entity);
          if (component) {
            c->color_fade.start_time = start_time;
            c->color_fade.end_time = start_time + fade_duration;
            c->color_fade.start_color =
                GetCharacterColor(*component, c->entity);
            c->color_fade.end_color = end_color;
          }
        },
        component);
  }

  if (behavior.play_audio) {
    PlayAudio(*behavior.play_audio, component);
  }
}

void WordArtSystem::WordArtComponent::ColorFade::Apply(
    const Clock::time_point& now, mathfu::vec4* result) {
  const auto zero = Clock::time_point();
  if (end_time == zero) {
    return;
  }
  if (now < start_time) {
    return;
  }

  if (now >= end_time) {
    *result = end_color;
    end_time = zero;
  } else {
    float lerp_amount = smoothcfit(
        SecondsFromDuration(now.time_since_epoch()),
        SecondsFromDuration(start_time.time_since_epoch()),
        SecondsFromDuration(end_time.time_since_epoch()), 0.0f, 1.0f);
    *result = mathfu::Lerp(start_color, end_color, lerp_amount);
  }
}

void WordArtSystem::WordArtComponent::ColorScale::Apply(
    const Clock::time_point& now, mathfu::vec4* result) {
  const auto zero = Clock::time_point();
  if (end_time == zero) {
    return;
  }
  if (now < start_time) {
    return;
  }

  float scale = 1.0f;
  if (now >= end_time) {
    scale = scale_values.back();
    end_time = zero;
  } else {
    int max_index = static_cast<int>(scale_values.size()) - 1;
    float lerp =
        cfit(SecondsFromDuration(now.time_since_epoch()),
             SecondsFromDuration(start_time.time_since_epoch()),
             SecondsFromDuration(end_time.time_since_epoch()), 0.0f, max_index);
    auto low_index = static_cast<int>(std::floor(lerp));
    int high_index = std::min(low_index + 1, max_index);
    float low_value = scale_values[low_index];
    float high_value = scale_values[high_index];
    lerp = lerp - low_index;
    scale = lerp * high_value + (1.0f - lerp) * low_value;
  }
  *result *= scale;
}

void WordArtSystem::UpdateAnimations() {
  auto* animation_system = registry_->Get<AnimationSystem>();
  DCHECK(animation_system);

  const auto now = Clock::now();
  for (auto& component : components_) {
    if (component.is_paused) {
      continue;
    }
    bool start_sync_idle = component.sync_idle;
    for (size_t i = 0; i < component.characters.size(); ++i) {
      auto& c = component.characters[i];
      bool start_idle = true;

      mathfu::vec4 frame_color = c.char_color;
      c.color_fade.Apply(now, &frame_color);
      if (c.color_fade.IsActive()) {
        start_idle = false;
      }
      c.char_color = frame_color;
      c.color_scale.Apply(now, &frame_color);
      if (c.color_scale.IsActive()) {
        start_idle = false;
      }

      if (frame_color != c.last_color) {
        SetCharacterColor(component, c.entity, frame_color);
        c.last_color = frame_color;
      }

      if (animation_system->TimeRemaining(c.entity, component.channel) > 0) {
        start_idle = false;
      }

      if (component.sync_idle) {
        if (!start_idle) {
          start_sync_idle = false;
        }
      } else if (start_idle) {
        SetUpAnimations(component.idle_behavior, static_cast<int>(i),
                        &component);
      }
    }
    // Sync idle components must wait until their sync_count is zero to ensure
    // no other animations are actively running.
    if (start_sync_idle && (component.sync_count == 0)) {
      SetUpAnimations(component.idle_behavior, -1, &component);
    }
  }
}

void WordArtSystem::LoadAudio(const WordArtBehaviorDefT& behavior) {
  if (behavior.play_audio) {
    if (behavior.play_audio->loop && behavior.play_audio->per_character) {
      LOG(ERROR) << "Cannot play looping audio per character.";
      return;
    }
    auto* audio_system = registry_->Get<AudioSystem>();
    DCHECK(audio_system);
    AudioLoadType load_type = GetLoadType(*behavior.play_audio);
    for (const auto& file : behavior.play_audio->files) {
      audio_system->LoadSound(TaggedAudioFile(file), load_type);
    }
  }
}

void WordArtSystem::PlayAudio(const WordArtPlayAudioDefT& play_audio,
                              WordArtComponent* component) {
  auto* audio_system = registry_->Get<AudioSystem>();
  DCHECK(audio_system);
  auto* rng = registry_->Get<RandomNumberGenerator>();
  DCHECK(rng);
  if (play_audio.loop && component->has_looping_audio) {
    return;
  }
  if (play_audio.files.empty()) {
    return;
  }

  size_t count = component->characters.size();
  if (!play_audio.per_character) {
    count = 1;
  }
  if (count == 0) {
    return;
  }

  if (play_audio.loop) {
    component->has_looping_audio = true;
  }

  int file_index = 0;
  auto num_files = static_cast<int>(play_audio.files.size());
  if (play_audio.sequence == WordArtAudioSequence_SequenceFromRandom) {
    file_index = rng->GenerateUniform(0, num_files - 1);
  }

  auto delay = cfit(component->text.size(), kMaxDelayChars, kMinDelayChars,
                    play_audio.max_delay_s, play_audio.min_delay_s);
  float time_offset = 0.0f;
  for (size_t i = 0; i < count; ++i) {
    if (play_audio.sequence == WordArtAudioSequence_Random) {
      file_index = rng->GenerateUniform(0, num_files - 1);
    }

    HashValue audio_file = Hash(TaggedAudioFile(play_audio.files[file_index]));
    Entity entity = component->GetEntity();
    bool loop = play_audio.loop;
    float volume = play_audio.volume;
    AudioPlaybackType playback_type = GetPlaybackType(play_audio);

    scheduled_processor_.Add(
        [=]() {
          if (components_.Get(entity)) {
            PlaySoundParameters params;
            params.playback_type = playback_type;
            params.loop = loop;
            params.volume = volume;
            audio_system->Play(entity, audio_file, params);
          }
        },
        DurationFromSeconds(time_offset));

    file_index = (file_index + 1) % num_files;
    time_offset += delay;
  }
}

void WordArtSystem::SetCharacterColor(const WordArtComponent& component,
                                      Entity entity,
                                      const mathfu::vec4& color) {
  auto* render_system = registry_->Get<RenderSystem>();
  DCHECK(render_system);

  if (component.color_uniform_name.empty() ||
      component.color_uniform_name == "color") {
    render_system->SetColor(entity, color);
  } else {
    render_system->SetUniform(entity, component.color_uniform_name.c_str(),
                              &color[0], component.color_uniform_size);
  }
}

mathfu::vec4 WordArtSystem::GetCharacterColor(const WordArtComponent& component,
                                              Entity entity) {
  auto* render_system = registry_->Get<RenderSystem>();
  DCHECK(render_system);

  mathfu::vec4 result(0.0f, 0.0f, 0.0f, 1.0f);
  if (component.color_uniform_name.empty() ||
      component.color_uniform_name == "color") {
    render_system->GetColor(entity, &result);
  } else {
    render_system->GetUniform(entity, component.color_uniform_name.c_str(),
                              component.color_uniform_size, &result[0]);
  }

  return result;
}

std::vector<std::string> WordArtSystem::GetWrappedLines(
    const WordArtComponent& component, Entity entity) {
  auto* transform_system = registry_->Get<TransformSystem>();
  DCHECK(transform_system);

  std::vector<std::string> lines = absl::StrSplit(component.text, '\n');
  if (component.max_width <= 0) {
    return lines;
  }

  const float space_width = component.GetSpaceWidth();
  const float ellipsis_width = component.GetStringWidth(kEllipsis);
  std::vector<std::string> wrapped_lines;
  for (const auto& line : lines) {
    absl::string_view::size_type line_start = 0;
    float line_width = 0.0f;

    absl::string_view::size_type last_word_separator = 0;
    float current_word_width = 0.0f;

    size_t ellipsis_pos = 0;
    for (absl::string_view::size_type i = 0; i < line.size(); i++) {
      char c = line[i];
      float char_width;
      if (c == ' ') {
        char_width = space_width;
        current_word_width = 0.0f;
        last_word_separator = i;
      } else {
        char_width = component.GetCharacterWidth(c) +
                     space_width * component.character_pad;
        current_word_width += char_width;
      }

      const bool is_last_line = wrapped_lines.size() == kMaxLineCount - 1;
      const float max_line_width = component.max_width;
      const float new_line_width = line_width + char_width;
      if (is_last_line && ellipsis_pos == 0 &&
          new_line_width + ellipsis_width > max_line_width) {
        ellipsis_pos = i;
      }
      if (i > 0 && new_line_width > max_line_width) {
        // Start a new line at either the last space/hyphen in the current line
        // (if one exists), or the current character (if it's a one-word line).
        bool break_at_separator =
            !is_last_line && last_word_separator > line_start;
        const size_t word_end = break_at_separator ? last_word_separator : i;
        const size_t line_end = is_last_line ? ellipsis_pos : word_end;
        std::string wrapped_line =
            line.substr(line_start, line_end - line_start);
        absl::StripAsciiWhitespace(&wrapped_line);

        if (is_last_line) {
          wrapped_lines.push_back(absl::StrCat(wrapped_line, kEllipsis));
          break;
        } else {
          wrapped_lines.push_back(wrapped_line);
        }

        line_start = line_end;
        if (break_at_separator) {
          line_width = current_word_width;
        } else {
          line_width = char_width;
          current_word_width = char_width;
        }
      } else {
        line_width += char_width;
      }

      // If this char is a hyphen, start keeping track of a new word to allow
      // wrapping at hyphens. This must be done *after* the width check above in
      // case the hyphen itself causes overflow onto the next line.
      if (c == '-') {
        current_word_width = 0.0f;
        last_word_separator = i + 1;
      }

      if (i == line.size() - 1) {
        // Append the last (unwrapped) line.
        std::string last_line =
            line.substr(line_start, line.size() - line_start);
        absl::StripAsciiWhitespace(&last_line);
        if (!last_line.empty()) {
          wrapped_lines.push_back(last_line);
        }
      }
    }
    if (wrapped_lines.size() == kMaxLineCount) {
      break;
    }
  }

  return wrapped_lines;
}

}  // namespace lull
