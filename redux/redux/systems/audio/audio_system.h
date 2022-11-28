/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#ifndef REDUX_SYSTEMS_AUDIO_AUDIO_SYSTEM_H_
#define REDUX_SYSTEMS_AUDIO_AUDIO_SYSTEM_H_

#include "absl/container/flat_hash_map.h"
#include "redux/engines/audio/audio_engine.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/ecs/system.h"
#include "redux/systems/audio/sound_def_generated.h"
#include "redux/systems/transform/transform_system.h"

namespace redux {

class AudioSystem : public System {
 public:
  explicit AudioSystem(Registry* registry);
  ~AudioSystem() override;

  void OnRegistryInitialize();

  // Play a sound on an Entity based on the hash of the sound name. This assumes
  // the sound asset has been loaded by calling LoadSound().
  using SoundPlaybackParams = AudioEngine::SoundPlaybackParams;
  void Play(Entity entity, HashValue sound, const SoundPlaybackParams& params);
  void Play(Entity entity, std::string_view uri,
            const SoundPlaybackParams& params);

  // Stop playing the specified sound on the Entity.
  void Stop(Entity entity);
  void Stop(Entity entity, HashValue sound);

  // Pauses a sound on the specified Entity. If no sound is specified, pauses
  // all sounds on the Entity.
  void Pause(Entity entity);
  void Pause(Entity entity, HashValue sound);

  // Resumes a sound on the specified Entity. If no sound is specified, resumes
  // all sounds on the Entity.
  void Resume(Entity entity);
  void Resume(Entity entity, HashValue sound);

  // Sets the volume for an Entity. If no sound is specified, sets the volume
  // for all sounds on the Entity. If a sound is specified and exists, sets the
  // volume for only that sound.
  void SetGlobalVolume(Entity entity, float volume);
  void SetVolume(Entity entity, HashValue sound, float volume);

  // Get the volume for an Entity. If the entity is the AudioListener, gets the
  // global volume. If no sound is specified, arbitrarily gets the volume of the
  // first sound on the Entity (to support volume animations). If a sound is
  // specified and exists, gets the volume for that sound.
  float GetGlobalVolume(Entity entity) const;
  float GetVolume(Entity entity, HashValue sound) const;

  // Sets the sound directivity pattern for a specific sound object on |entity|.
  // |alpha| is a weighting balance between a figure 8 pattern and
  // omnidirectional pattern for source emission. Its range is [0, 1], with a
  // value of 0.5 results in a cardioid pattern. |order| is applied to computed
  // directivity. Higher values will result in narrower and sharper directivity
  // patterns. Its range is [1, inf).
  void SetDirectivity(Entity entity, HashValue sound, float alpha, float order);

  // Sets the distance attenuation for a specific sound object on |entity|.
  // |method| specifies the rolloff method. |min_distance| and |max_distance|
  // specifies the distances at which attentuation begin and end.
  using DistanceRolloffModel = Sound::DistanceRolloffModel;
  void SetDistanceRolloffModel(Entity entity, HashValue sound,
                               DistanceRolloffModel model, float min_distance,
                               float max_distance);

  // Sets the position of the listener for spatial audio sources.
  void SetListenerTransform(const mat4& listener_transform);

  // Update positions for all audio sources in the world.
  void PrepareToRender();

  // Add/Remove Entities.
  void AddSoundFromSoundDef(Entity entity, const SoundDef& def);

 private:
  struct Sound {
    SoundPtr sound;
    float volume = 1.0f;
  };

  struct SoundComponent {
    absl::flat_hash_map<HashValue, Sound> sounds;
    bool enabled = true;
    float volume = 1.0f;
    // cache the most recent transform since it is expensive to update
    Transform transform;
  };

  void OnDestroy(Entity entity) override;
  void OnEnable(Entity entity) override;
  void OnDisable(Entity entity) override;

  void CollectGarbage(Entity entity);

  template <typename Fn>
  void ForAllSounds(Entity entity, const Fn& fn);

  template <typename Fn>
  void ForSound(Entity entity, HashValue sound_id, const Fn& fn);

  AudioEngine* engine_ = nullptr;
  absl::flat_hash_map<Entity, SoundComponent> components_;
  TransformSystem::TransformFlags transform_flag_;
};
}  // namespace redux

REDUX_SETUP_TYPEID(redux::AudioSystem);

#endif  // REDUX_SYSTEMS_AUDIO_AUDIO_SYSTEM_H_
