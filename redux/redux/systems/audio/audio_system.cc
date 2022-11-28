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

#include "redux/systems/audio/audio_system.h"

#include "redux/modules/base/choreographer.h"
#include "redux/systems/transform/transform_system.h"

namespace redux {

AudioSystem::AudioSystem(Registry* registry) : System(registry) {
  RegisterDef(&AudioSystem::AddSoundFromSoundDef);
  RegisterDependency<TransformSystem>(this);
}

AudioSystem::~AudioSystem() {
  auto* transform_system = registry_->Get<TransformSystem>();
  if (transform_system && transform_flag_.Any()) {
    transform_system->ReleaseFlag(transform_flag_);
  }
}

void AudioSystem::OnRegistryInitialize() {
  engine_ = registry_->Get<AudioEngine>();
  CHECK(engine_);

  auto* transform_system = registry_->Get<TransformSystem>();
  transform_flag_ = transform_system->RequestFlag();

  auto choreo = registry_->Get<Choreographer>();
  if (choreo) {
    choreo->Add<&AudioSystem::PrepareToRender>(Choreographer::Stage::kRender)
        .Before<&AudioEngine::Update>();
  }
}

void AudioSystem::Play(Entity entity, std::string_view uri,
                       const SoundPlaybackParams& params) {
  engine_->LoadAudioAsset(uri, AudioEngine::kStreamIntoMemory);
  Play(entity, Hash(uri), params);
}

void AudioSystem::Play(Entity entity, HashValue sound,
                       const SoundPlaybackParams& params) {
  AudioAssetPtr asset = engine_->GetAudioAsset(sound);
  CHECK(asset);

  auto* transform_system = registry_->Get<TransformSystem>();
  transform_system->SetFlag(entity, transform_flag_);

  SoundComponent& c = components_[entity];
  Sound& s = c.sounds[sound];
  s.sound = engine_->PlaySound(asset, params);
  if (!IsEntityEnabled(entity)) {
    s.sound->Pause();
  }
}

void AudioSystem::Stop(Entity entity) {
  ForAllSounds(entity, [](Sound& s) { s.sound->Stop(); });
  CollectGarbage(entity);
}

void AudioSystem::Stop(Entity entity, HashValue sound) {
  ForSound(entity, sound, [](Sound& s) { s.sound->Stop(); });
  CollectGarbage(entity);
}

void AudioSystem::Resume(Entity entity) {
  ForAllSounds(entity, [](Sound& s) { s.sound->Resume(); });
}

void AudioSystem::Resume(Entity entity, HashValue sound) {
  ForSound(entity, sound, [](Sound& s) { s.sound->Resume(); });
}

void AudioSystem::Pause(Entity entity) {
  ForAllSounds(entity, [](Sound& s) { s.sound->Pause(); });
}

void AudioSystem::Pause(Entity entity, HashValue sound) {
  ForSound(entity, sound, [](Sound& s) { s.sound->Pause(); });
}

float AudioSystem::GetGlobalVolume(Entity entity) const {
  auto it = components_.find(entity);
  if (it != components_.end()) {
    return it->second.volume;
  }
  return 1.0f;
}

void AudioSystem::SetGlobalVolume(Entity entity, float volume) {
  auto it = components_.find(entity);
  if (it != components_.end()) {
    it->second.volume = volume;
    ForAllSounds(entity,
                 [=](Sound& s) { s.sound->SetVolume(s.volume * volume); });
  }
}

void AudioSystem::SetVolume(Entity entity, HashValue sound, float volume) {
  const float global_volume = GetGlobalVolume(entity);
  ForSound(entity, sound, [=](Sound& s) {
    s.sound->SetVolume(volume * global_volume);
    s.volume = volume;
  });
}

void AudioSystem::SetDirectivity(Entity entity, HashValue sound, float alpha,
                                 float order) {
  ForSound(entity, sound,
           [=](Sound& s) { s.sound->SetDirectivitiy(alpha, order); });
}

void AudioSystem::SetDistanceRolloffModel(Entity entity, HashValue sound,
                                          DistanceRolloffModel model,
                                          float min_distance,
                                          float max_distance) {
  ForSound(entity, sound, [=](Sound& s) {
    s.sound->SetDistanceRolloffModel(model, min_distance, max_distance);
  });
}

void AudioSystem::CollectGarbage(Entity entity) {
  auto component_iter = components_.find(entity);
  if (component_iter == components_.end()) {
    return;
  }

  SoundComponent& c = component_iter->second;
  for (auto iter = c.sounds.begin(); iter != c.sounds.end();) {
    if (!iter->second.sound->IsValid()) {
      c.sounds.erase(iter++);
    } else {
      ++iter;
    }
  }

  if (c.sounds.empty()) {
    components_.erase(component_iter);
    auto* transform_system = registry_->Get<TransformSystem>();
    transform_system->ClearFlag(entity, transform_flag_);
  }
}

void AudioSystem::SetListenerTransform(const mat4& listener_transform) {
  const auto transform = Transform(listener_transform);
  engine_->SetListenerTransform(transform.translation, transform.rotation);
}

void AudioSystem::PrepareToRender() {
  // An OnPauseThreadUnsafeEvent may turn off audio playback while updating,
  // which can incorrectly label some sounds as not running anymore.
  // std::lock_guard<std::mutex> lock(pause_mutex_);
  auto* transform_system = registry_->Get<TransformSystem>();
  for (auto& iter : components_) {
    const Transform transform = transform_system->GetTransform(iter.first);
    if (iter.second.transform.translation != transform.translation ||
        iter.second.transform.rotation != transform.rotation) {
      iter.second.transform = transform;
      for (auto& s : iter.second.sounds) {
        if (s.second.sound->IsPlaying()) {
          s.second.sound->SetTransform(transform);
        }
      }
    }
  }
}

void AudioSystem::OnEnable(Entity entity) { Resume(entity); }

void AudioSystem::OnDisable(Entity entity) { Pause(entity); }

void AudioSystem::OnDestroy(Entity entity) { Stop(entity); }

template <typename Fn>
void AudioSystem::ForSound(Entity entity, HashValue sound_id, const Fn& fn) {
  auto component_iter = components_.find(entity);
  if (component_iter == components_.end()) {
    return;
  }
  auto sound_iter = component_iter->second.sounds.find(sound_id);
  if (sound_iter == component_iter->second.sounds.end()) {
    return;
  }
  fn(sound_iter->second);
}

template <typename Fn>
void AudioSystem::ForAllSounds(Entity entity, const Fn& fn) {
  auto component_iter = components_.find(entity);
  if (component_iter == components_.end()) {
    return;
  }
  for (auto& [id, s] : component_iter->second.sounds) {
    fn(s);
  }
}

void AudioSystem::AddSoundFromSoundDef(Entity entity, const SoundDef& def) {
  SoundPlaybackParams params;
  params.looping = def.looping;
  params.volume = def.volume;
  Play(entity, def.uri, params);
}

}  // namespace redux
