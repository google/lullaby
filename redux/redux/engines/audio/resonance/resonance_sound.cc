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

#include "redux/engines/audio/resonance/resonance_sound.h"

#include "redux/engines/audio/resonance/resonance_audio_engine.h"

namespace redux {

ResonanceSound::ResonanceSound(SoundType type, Id id,
                               ResonanceAudioEngine* engine)
    : Sound(type), id_(id), engine_(engine) {}

ResonanceSound::~ResonanceSound() {
  if (engine_) {
    Stop();
  }
}

bool ResonanceSound::IsValid() const { return engine_ != nullptr; }

void ResonanceSound::Resume() {
  engine_->ResumeSound(id_);
  playing_ = true;
}

void ResonanceSound::Pause() {
  engine_->PauseSound(id_);
  playing_ = false;
}

void ResonanceSound::Stop() {
  if (engine_) {
    engine_->StopSound(id_);
    playing_ = false;
    engine_ = nullptr;
  }
}

void ResonanceSound::SetVolume(float volume) {
  return engine_->SetSoundVolume(id_, volume);
}

bool ResonanceSound::IsPlaying() const { return playing_; }

void ResonanceSound::SetTransform(const Transform& transform) {
  if (type_ == SoundType::Point) {
    const vec3& pos = transform.translation;
    const quat& rot = transform.rotation;
    engine_->SetSoundObjectPosition(id_, pos);
    engine_->SetSoundObjectRotation(id_, rot);
  } else if (type_ == SoundType::Field) {
    const quat& rot = transform.rotation;
    engine_->SetSoundfieldRotation(id_, rot);
  }
}

void ResonanceSound::SetDirectivitiy(float alpha, float order) {
  engine_->SetSoundObjectDirectivity(id_, alpha, order);
}

void ResonanceSound::SetDistanceRolloffModel(DistanceRolloffModel rolloff,
                                             float min_distance,
                                             float max_distance) {
  engine_->SetSoundObjectDistanceRolloffModel(id_, rolloff, min_distance,
                                              max_distance);
}

inline ResonanceSound* Upcast(Sound* ptr) {
  return static_cast<ResonanceSound*>(ptr);
}

inline const ResonanceSound* Upcast(const Sound* ptr) {
  return static_cast<const ResonanceSound*>(ptr);
}

bool Sound::IsValid() const { return Upcast(this)->IsValid(); }
void Sound::Resume() { Upcast(this)->Resume(); }
void Sound::Pause() { Upcast(this)->Pause(); }
void Sound::Stop() { Upcast(this)->Stop(); }
void Sound::SetVolume(float volume) { Upcast(this)->SetVolume(volume); }
bool Sound::IsPlaying() const { return Upcast(this)->IsPlaying(); }
void Sound::SetTransform(const Transform& transform) {
  Upcast(this)->SetTransform(transform);
}
void Sound::SetDirectivitiy(float alpha, float order) {
  Upcast(this)->SetDirectivitiy(alpha, order);
}
void Sound::SetDistanceRolloffModel(DistanceRolloffModel rolloff,
                                    float min_distance, float max_distance) {
  Upcast(this)->SetDistanceRolloffModel(rolloff, min_distance, max_distance);
}

}  // namespace redux
