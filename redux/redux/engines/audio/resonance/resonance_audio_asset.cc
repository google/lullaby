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

#include "redux/engines/audio/resonance/resonance_audio_asset.h"

#include "redux/modules/audio/audio_reader.h"

namespace redux {

ResonanceAudioAsset::ResonanceAudioAsset(Id id, bool stream_into_memory)
    : AudioAsset(id),
      stream_into_memory_(stream_into_memory),
      status_(Status::kWaitingForReader) {}

void ResonanceAudioAsset::SetStatus(Status status) {
  {
    std::lock_guard<std::mutex> lock(status_change_mutex_);
    status_ = status;
  }
  status_change_conditional_.notify_all();
}

bool ResonanceAudioAsset::IsValid() const {
  return status_.load() == ResonanceAudioAsset::Status::kLoadedInMemory ||
         status_.load() == ResonanceAudioAsset::Status::kReadyForStreaming;
}

void ResonanceAudioAsset::SetAudioReader(std::unique_ptr<AudioReader> reader) {
  if (status_.load() == Status::kLoadedInMemory) {
    // Audio data is loaded into memory, so we don't need a reader.
    return;
  }
  if (reader == nullptr) {
    SetStatus(Status::kInvalid);
    return;
  }

  CHECK(reader_ == nullptr) << "Reader already set!";
  reader_ = std::move(reader);
  planar_data_.reset();
  SetStatus(Status::kReadyForStreaming);
}

void ResonanceAudioAsset::SetAudioPlanarData(
    std::unique_ptr<AudioPlanarData> planar_data) {
  if (planar_data == nullptr) {
    SetStatus(Status::kInvalid);
    return;
  }

  CHECK(planar_data_ == nullptr) << "Audio data already set!";
  planar_data_ = std::move(planar_data);
  reader_.reset();
  SetStatus(Status::kLoadedInMemory);
}

std::unique_ptr<AudioReader> ResonanceAudioAsset::AcquireReader() {
  return std::move(reader_);
}

bool ResonanceAudioAsset::IsActivelyStreaming() const {
  return status_.load() == Status::kReadyForStreaming && reader_ == nullptr;
}

void ResonanceAudioAsset::WaitForInitialization() {
  std::unique_lock<std::mutex> lock(status_change_mutex_);
  status_change_conditional_.wait(lock, [this]() {
    return status_.load() != ResonanceAudioAsset::Status::kWaitingForReader;
  });
}
}  // namespace redux
