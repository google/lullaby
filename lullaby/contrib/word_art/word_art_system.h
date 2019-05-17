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

#ifndef LULLABY_CONTRIB_WORD_ART_WORD_ART_SYSTEM_H_
#define LULLABY_CONTRIB_WORD_ART_WORD_ART_SYSTEM_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "absl/strings/string_view.h"
#include "lullaby/generated/dispatcher_def_generated.h"
#include "lullaby/modules/animation_channels/render_channels.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/file/asset.h"
#include "lullaby/util/entity.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/scheduled_processor.h"
#include "lullaby/generated/model_asset_def_generated.h"
#include "lullaby/generated/render_def_generated.h"
#include "lullaby/generated/word_art_def_generated.h"

namespace lull {

/// The WordArtSystem is used to create entities that are built of child
/// character entities for displaying user-entered text strings.
class WordArtSystem : public System {
 public:
  // Defines how the character mesh filenames are formatted.
  enum CharacterMeshFilenameFormat {
    // The mesh filenames use lower-case letter suffixes, e.g:
    // "simple_a.lullmodel" for the letter "a".
    kLowerCaseSuffixes,

    // The mesh filenames use upper-case letter suffixes, e.g:
    // "simple_A.lullmodel" for the letter "a".
    kUpperCaseSuffixes
  };

  WordArtSystem(Registry* registry,
                CharacterMeshFilenameFormat character_mesh_filename_format =
                    kLowerCaseSuffixes);
  ~WordArtSystem() override;

  void CreateComponent(Entity entity, const Blueprint& blueprint) override;

  void Destroy(Entity entity) override;

  void AdvanceFrame(Clock::duration delta_time);

  bool HasWordArt(Entity entity) const;

  // SetText should only return an empty vector if the entity does not have a
  // WordArtComponent.
  std::vector<Entity> SetText(Entity entity, const std::string& text);

  // Triggers the color_change_behavior.color_select with the given named color.
  void SetColor(Entity entity, const mathfu::vec4& color,
                const std::string& color_name, bool animate);

  // Returns the vertical and horizontal dimensions of the currently rendered
  // text.
  mathfu::vec2 GetTextBounds(Entity entity) const;

  // Returns an offset that should be applied to the selection bounding box for
  // this word art entity.
  mathfu::vec3 GetSelectionBoundingBoxOffset(Entity entity) const;

 private:
  struct WordArtComponent : public Component {
    explicit WordArtComponent(Entity entity) : Component(entity) {}

    const ModelAssetDef* model_asset_def;
    std::unordered_map<std::string, float>* character_width;
    std::string mesh_base;
    std::string mesh_extension;
    float line_height;
    float character_pad;
    float max_width;
    mathfu::vec3 selection_bounding_box_offset;
    WordArtBehaviorDefT place_behavior;
    WordArtBehaviorDefT tap_behavior;
    WordArtBehaviorDefT idle_behavior;
    WordArtBehaviorDefT color_select_behavior;
    HashValue channel;
    bool sync_idle;
    CharacterMeshFilenameFormat character_mesh_filename_format;

    // Track color change animation progress.
    struct ColorFade {
      Clock::time_point start_time;
      Clock::time_point end_time;
      mathfu::vec4 start_color;
      mathfu::vec4 end_color;

      void Apply(const Clock::time_point& now, mathfu::vec4* result);
      bool IsActive() const {return end_time != Clock::time_point();}
    };

    // Track color scale animation progress.
    struct ColorScale {
      Clock::time_point start_time;
      Clock::time_point end_time;
      std::vector<float> scale_values;

      void Apply(const Clock::time_point& now, mathfu::vec4* result);
      bool IsActive() const {return end_time != Clock::time_point();}
    };

    struct Character {
      Entity entity;
      // The color this character should be if no animations are active.
      mathfu::vec4 char_color = mathfu::vec4(0.0f, 0.0f, 0.0f, 1.0f);
      // The last color that was set in Lullaby for this character.
      mathfu::vec4 last_color = mathfu::vec4(-1.0f);

      ColorFade color_fade;
      ColorScale color_scale;
    };

    std::string GetCharacterMeshFile(char character) const;
    float GetStringWidth(absl::string_view str) const;
    float GetCharacterWidth(char character) const;
    float GetSpaceWidth() const;

    std::vector<Character> characters;
    std::string text;
    std::string color_uniform_name;
    int color_uniform_size;
    int color_index = -1;
    mathfu::vec4 color = mathfu::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    bool has_been_placed = false;
    bool is_paused = true;
    mathfu::vec2 text_bounds;

    // Only the first looping sound for the component is played.
    bool has_looping_audio = false;

    // Cause sync_idle to wait until sync_count is 0.
    int sync_count = 0;
  };

  void HandleDropEvent(Entity entity);
  void HandleTapEvent(Entity entity);

  // Sets up color animation parameters for the specified entity and character.
  using SetUpAnimationFunc =
      std::function<void(Clock::time_point start_time,
                         WordArtComponent::Character* c)>;
  void SetUpAnimations(const WordArtBehaviorDefT& behavior,
                       int character_index, WordArtComponent* component);
  // Loops over characters in component, calling setup_func on each, staggering
  // the start time for each animation by stagger_delta.
  // If character_index >=0, then it references a single character to update
  // instead.
  void StaggerAnimations(int character_index, float stagger_delay,
                         const SetUpAnimationFunc& setup_func,
                         WordArtComponent* component);
  // Updates the color for any character that is currently in an active
  // animation state.
  void UpdateAnimations();

  // Loads audio files referenced in the provided behavior definition.
  void LoadAudio(const WordArtBehaviorDefT& behavior);

  // Schedules audio files to be played based on the provided play_audio
  // definition.
  void PlayAudio(const WordArtPlayAudioDefT& play_audio,
                 WordArtComponent* component);

  // Sets the color in RenderSystem for the given component and entity.  By
  // default, RenderSystem::SetColor is called, but this may be overridden in
  // the component def to RenderSystem::SetUniform on a different uniform
  // instead.
  void SetCharacterColor(const WordArtComponent& component, Entity entity,
                         const mathfu::vec4& color);
  // Returns the RenderSystem color of the given component and entity, as set by
  // SetCharacterColor() above.
  mathfu::vec4 GetCharacterColor(const WordArtComponent& component,
                                 Entity entity);

  // Returns a list of strings representing the lines of text to render. Line
  // breaks are inserted at literal '\n' chars, and if the component's
  // |max_width| attribute is defined additional line breaks are inserted to
  // fit within the bounds. Also adds an ellipsis to the last line if the text
  // would exceed the maximum number of lines.
  std::vector<std::string> GetWrappedLines(const WordArtComponent& component,
                                           Entity entity);

  ComponentPool<WordArtComponent> components_;
  ScheduledProcessor scheduled_processor_;

  // A cache mapping character mesh file path to bbox width (size in x).
  std::unordered_map<std::string, float> character_width_;

  CharacterMeshFilenameFormat character_mesh_filename_format_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::WordArtSystem);

#endif  // LULLABY_CONTRIB_WORD_ART_WORD_ART_SYSTEM_H_
