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

#ifndef LULLABY_SYSTEMS_AUDIO_AUDIO_SYSTEM_H_
#define LULLABY_SYSTEMS_AUDIO_AUDIO_SYSTEM_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "mathfu/glsl_mappings.h"
#include "lullaby/generated/audio_playback_types_generated.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/util/entity.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/systems/audio/play_sound_parameters.h"
#include "lullaby/systems/audio/sound_asset.h"
#include "lullaby/systems/audio/sound_asset_manager.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/math.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/string_view.h"
#include "lullaby/util/typeid.h"
#include "lullaby/util/unordered_vector_map.h"
#include "vr/gvr/capi/include/gvr_audio.h"
#include "vr/gvr/capi/include/gvr_types.h"

namespace lull {

struct AudioEnvironmentDef;
struct AudioListenerDef;
struct AudioResponseDef;
struct AudioSourceDef;

class AudioSystem : public System {
 public:
  explicit AudioSystem(Registry* registry);
  AudioSystem(Registry* registry, std::unique_ptr<gvr::AudioApi> api);

  ~AudioSystem() override;

  void Initialize() override;

  // Associate audio playback ability with the Entity using the specified
  // ComponentDef.
  void Create(Entity e, HashValue type, const Def* def) override;

  // Stop playing all sounds on the Entity.
  void Destroy(Entity e) override;

  // Play a sound on an Entity based on the hash of the sound name. This assumes
  // the sound asset has been loaded by calling LoadSound().
  void Play(Entity e, HashValue sound_hash, const PlaySoundParameters& params);

  // Stop playing the specified sound on the Entity.
  void Stop(Entity e, HashValue key);

  // Pauses a sound on the specified Entity. If no sound is specified, pauses
  // all sounds on the Entity.
  void Pause(Entity e, HashValue sound = 0);

  // Resumes a sound on the specified Entity. If no sound is specified, resumes
  // all sounds on the Entity.
  void Resume(Entity e, HashValue sound = 0);

  // Track the externally managed sound |id| on |entity|. Non-lifetime
  // properties, such as volume and transform, will be managed by the
  // AudioSystem. Callers should call Untrack() before the tracked sound is
  // stopped.
  void Track(Entity entity, gvr::AudioSourceId id, HashValue key, float volume,
             AudioSourceType source_type);

  // Stop tracking the specified sound on the Entity.
  void Untrack(Entity e, HashValue key);

  // Update positions for all audio sources in the world.
  void Update();

  // Mute or unmute all audio. Unmute will restore the master volume that was
  // set prior to muting.
  void SetMute(bool muted);

  // True if all audio is muted, otherwise false.
  bool IsMute() const;

  // Sets the master volume. If the system is currently muted, this call will
  // unmute the system. Setting a master volume of 0 is not equivalent to muting
  // the system.
  void SetMasterVolume(float volume);

  // Gets the master volume, ignoring any mute state.
  float GetMasterVolume() const;

  // Load a sound from |filename|. |type| denotes how this sound should be
  // retrieved and played. |e| denotes an entity to send an AudioLoadedEvent to
  // when the sound is ready to be played.
  void LoadSound(const std::string& filename, AudioLoadType type,
                 Entity e = kNullEntity);

  // Unload an existing sound for |filename|. If the sound is currently in use,
  // the memory will be freed as soon as playback stops. If the sound is
  // currently loading, it will be unloaded when loading completes.
  void UnloadSound(const std::string& filename);

  // Returns true if |e| has any active sounds, which are either playing or set
  // up to play in the next Update().
  bool HasSound(Entity e) const;

  // Return a list of all of the active sounds for |e|. This function is
  // is intended for use by the editor, and may be removed in the future.
  std::vector<string_view> GetSounds(Entity e) const;

  // Sets the volume for an Entity. If the Entity is the AudioListener, sets the
  // master volume. If no sound is specified, sets the volume for all sounds on
  // the Entity. If a sound is specified and exists, sets the volume for only
  // that sound.
  void SetVolume(Entity e, float volume, HashValue sound = 0);

  // Get the volume for an Entity. If the entity is the AudioListener, gets the
  // master volume. If no sound is specified, arbitrarily gets the volume of the
  // first sound on the Entity (to support volume animations). If a sound is
  // specified and exists, gets the volume for that sound.
  float GetVolume(Entity e, HashValue sound = 0) const;

  // Get the current AudioListener Entity.
  Entity GetCurrentListener() const;

  // Set the audio environment to that in AudioEnvironment component of |e|.
  // Disables the audio environment if |e| is the null entity.
  void SetEnvironment(Entity e);

  // Sets the sound directivity pattern for a specific sound object on |entity|.
  // |alpha| is a weighting balance between a figure 8 pattern and
  // omnidirectional pattern for source emission. Its range is [0, 1], with a
  // value of 0.5 results in a cardioid pattern. |order| is applied to computed
  // directivity. Higher values will result in narrower and sharper directivity
  // patterns. Its range is [1, inf).
  void SetSoundObjectDirectivity(Entity entity, HashValue key, float alpha,
                                 float order);

  // Sets the distance attenuation for a specific sound object on |entity|.
  // |method| specifies the rolloff method. |min_distance| and |max_distance|
  // specifies the distances at which attentuation begin and end.
  void SetSoundObjectDistanceRolloffMethod(Entity entity, HashValue key,
                                           gvr::AudioRolloffMethod method,
                                           float min_distance,
                                           float max_distance);

 private:
  using SourceId = gvr::AudioSourceId;
  static const SourceId kInvalidSourceId = gvr::kInvalidSourceId;

  struct Sound {
    SourceId id = kInvalidSourceId;
    SoundAssetWeakPtr asset_handle;
    PlaySoundParameters params;
    bool paused = false;
  };

  struct AudioListener : Component {
    explicit AudioListener(Entity e) : Component(e), initial_volume(1.0) {}
    float initial_volume;
  };

  struct AudioSource : Component {
    explicit AudioSource(Entity e) : Component(e) {}
    std::unordered_map<HashValue, Sound> sounds;
    Sqt sqt;
    // Pausing and resuming occur in OnEnabled/DisableEvent, but the events may
    // not propagate before an Update() call. Track enabled state based on
    // the events and use it in place of  TransformSystem::IsEnabled() outside
    // of Play().
    bool enabled = true;
  };

  struct AudioEnvironment : Component {
    explicit AudioEnvironment(Entity e) : Component(e) {}
    mathfu::vec3 room_dimensions;
    float reflection_scalar;
    float reverb_brightness_modifier;
    float reverb_gain;
    float reverb_time;
    gvr::AudioMaterialName surface_material_wall;
    gvr::AudioMaterialName surface_material_ceiling;
    gvr::AudioMaterialName surface_material_floor;
  };

  void InitAudio();

  void CreateEnvironment(Entity e, const AudioEnvironmentDef* data);
  void CreateSource(Entity e, const AudioSourceDef* data);
  void CreateResponse(Entity entity, const AudioResponseDef* data);
  void CreateListener(Entity e, const AudioListenerDef* data);

  // Check and update the state of all of |sources|'s sounds. This includes
  // playing sounds that have just finished loading, updating the transforms of
  // sounds that are still playing, and cleaning up sounds that either failed
  // to play or are finished playing.
  void UpdateSource(AudioSource* source, const mathfu::mat4& world_from_entity);

  // Check if |source|'s previous SQT is different than |world_from_entity|. If
  // it is, update |sources|'s stored SQT and return true. Otherwise, return
  // false.
  bool UpdateSourceSqt(AudioSource* source,
                       const mathfu::mat4& world_from_entity);

  // Attempts to play |sound| using |sqt| as its transform. If the playback is
  // successful, the |sound|'s id will be set to the source id. If not, it will
  // be set to the invalid source id, and |asset| will be marked as "failed"
  // to prevent future playback attempts.
  void PlaySound(Sound* sound, const Sqt& sqt, const SoundAssetPtr& asset);

  // Pause or resume |sound| and update its tracked pause state.
  void PauseSound(Sound* sound);
  void ResumeSound(Sound* sound);

  // Update |sound|'s spatial rendering information to match |sqt|.
  void UpdateSoundTransform(Sound* sound, const Sqt& sqt);

  // Stop playing or tracking all sounds on the Entity.
  void StopAll(Entity entity);

  // Destroy |entity|'s component data and transform flag if it has no sounds
  // and no externally tracked sources.
  void TryDestroySource(Entity entity);

  // Returns true if the given |sound| has spatial directivity enabled.
  bool IsSpatialDirectivityEnabled(Sound* sound) const;

  // Returns true if the given |sound| has a custom distance rolloff method
  // enabled.
  bool IsDistanceRolloffMethodEnabled(Sound* sound) const;

  void OnEntityEnabled(Entity entity);
  void OnEntityDisabled(Entity entity);

  std::unique_ptr<SoundAssetManager> asset_manager_;
  ComponentPool<AudioSource> sources_;
  ComponentPool<AudioEnvironment> environments_;
  std::shared_ptr<gvr::AudioApi> audio_;
  AudioListener listener_;
  Entity current_environment_;
  std::mutex pause_mutex_;
  bool audio_running_;
  TransformSystem::TransformFlags transform_flag_;
  float master_volume_;
  bool muted_;

  AudioSystem(const AudioSystem&);
  AudioSystem& operator=(const AudioSystem&);
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::AudioSystem);

#endif  // LULLABY_SYSTEMS_AUDIO_AUDIO_SYSTEM_H_
