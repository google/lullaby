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

#include "redux/engines/audio/resonance/audio_stream_manager.h"

#include "resonance_audio/utils/task_thread_pool.h"
#include "resonance_audio/utils/threadsafe_fifo.h"

namespace redux {

// Number of streaming worker threads available for asynchronous decoding.
static const unsigned int kNumStreamingWorkerThreads = 64;

// Maximum number of streaming requests in stream renderer pointer queue.
static const size_t kMaxStreamFifoElements = 64;

AudioStreamManager::AudioStreamManager()
    : streaming_thread_running_(false),
      stream_renderer_ptr_fifo_(kMaxStreamFifoElements, nullptr) {}

void AudioStreamManager::Start() {
  stream_renderer_ptr_fifo_.EnableBlockingSleepUntilMethods(true);
  if (!streaming_thread_running_.load()) {
    streaming_thread_running_ = true;
    streaming_thread_ = std::thread([this]() { StreamingThread(); });
  }
}

void AudioStreamManager::Stop() {
  stream_renderer_ptr_fifo_.EnableBlockingSleepUntilMethods(false);
  if (streaming_thread_running_.load()) {
    streaming_thread_running_ = false;
    streaming_thread_.join();
  }

  while (stream_renderer_ptr_fifo_.Size() > 0) {
    auto renderer = PopFromStreamRendererFifo();
    if (renderer) {
      renderer->SetPrestockServicePending(false);
    }
  }
}

bool AudioStreamManager::AddAudioStreamRenderer(RendererPtr renderer) {
  const SourceId source_id = renderer->GetSourceId();
  const auto result = renderers_.emplace(source_id, std::move(renderer));
  return result.second;
}

AudioStreamRenderer* AudioStreamManager::GetAudioStreamRenderer(
    SourceId source_id) const {
  const auto iter = renderers_.find(source_id);
  if (iter != renderers_.end()) {
    return iter->second.get();
  }
  return nullptr;
}

void AudioStreamManager::Render(std::vector<SourceId>* disabled_renderer_ids) {
  if (disabled_renderer_ids) {
    disabled_renderer_ids->clear();
    disabled_renderer_ids->reserve(renderers_.size());
  }

  for (auto iter = renderers_.begin(); iter != renderers_.end();) {
    RendererPtr& renderer = iter->second;
    // Check whether this Renderer requires a block of processing to continue
    // supplying its stream of data to Resonance. If so, schedule an
    // asynchronous task to do so.
    if (streaming_thread_running_.load() &&
        renderer->IsPrestockServiceNeeded()) {
      if (!stream_renderer_ptr_fifo_.Full()) {
        renderer->SetPrestockServicePending(true);
        const bool pushed = PushToStreamRendererFifo(renderer);
        DCHECK(pushed);
      } else {
        renderer->SetPrestockServicePending(false);
        LOG(WARNING) << "Overflow of asynchronous restock requests. Is the "
                     << "decoder thread blocked?";
      }
    }

    if (renderer->Render()) {
      ++iter;
    } else {
      // Remove any renderers that are no longer providing audio buffers.
      if (disabled_renderer_ids) {
        disabled_renderer_ids->push_back(iter->first);
      }
      renderers_.erase(iter++);
    }
  }
}

void AudioStreamManager::StreamingThread() {
  vraudio::TaskThreadPool worker_thread_pool;
  if (!worker_thread_pool.StartThreadPool(kNumStreamingWorkerThreads)) {
    LOG(ERROR) << "Could not start worker threads";
    return;
  }

  while (streaming_thread_running_.load()) {
    while (streaming_thread_running_.load() &&
           !worker_thread_pool.WaitUntilWorkerBecomesAvailable()) {
    }
    while (streaming_thread_running_.load() &&
           !stream_renderer_ptr_fifo_.SleepUntilOutputObjectIsAvailable()) {
    }

    RendererPtr renderer = PopFromStreamRendererFifo();
    if (renderer != nullptr) {
      if (streaming_thread_running_.load()) {
        const auto task = [renderer]() { renderer->ServicePrestock(); };
        const bool worker_assigned = worker_thread_pool.RunOnWorkerThread(task);
        DCHECK(worker_assigned);
      } else {
        renderer->SetPrestockServicePending(false);
      }
    }
  }
}

bool AudioStreamManager::PushToStreamRendererFifo(
    const RendererPtr& audio_stream_renderer) {
  auto* input_stream_renderer = stream_renderer_ptr_fifo_.AcquireInputObject();
  if (input_stream_renderer == nullptr) {
    return false;
  }
  *input_stream_renderer = audio_stream_renderer;
  stream_renderer_ptr_fifo_.ReleaseInputObject(input_stream_renderer);
  return true;
}

std::shared_ptr<AudioStreamRenderer>
AudioStreamManager::PopFromStreamRendererFifo() {
  auto* output_stream_renderer =
      stream_renderer_ptr_fifo_.AcquireOutputObject();
  if (output_stream_renderer == nullptr) {
    return nullptr;
  }
  RendererPtr output = *output_stream_renderer;
  output_stream_renderer->reset();
  stream_renderer_ptr_fifo_.ReleaseOutputObject(output_stream_renderer);
  return output;
}

}  // namespace redux
