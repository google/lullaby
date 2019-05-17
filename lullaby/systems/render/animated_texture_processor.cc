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

#include "lullaby/systems/render/animated_texture_processor.h"

#include "lullaby/generated/flatbuffers/material_def_generated.h"
#include "lullaby/systems/render/texture_factory.h"

namespace lull {

AnimatedTextureProcessor::AnimatedTextureProcessor(Registry* registry)
    : registry_(registry),
      decode_queue_(1 /* worker threads */) {}

void AnimatedTextureProcessor::Animate(const TexturePtr& texture,
                                       AnimatedImagePtr animated_image) {
  if (animated_image == nullptr) {
    LOG(ERROR) << "Failed to initialize animated texture decoder";
    return;
  }

  auto anim_texture = std::make_shared<AnimatedTexture>();
  anim_texture->texture = texture;
  anim_texture->animated_image = std::move(animated_image);
  anim_texture->latest_frame_show_time = time_line_;

  decode_queue_.Enqueue(anim_texture,
                        [&](AnimatedTexturePtr* req) { DecodeFrame(*req); });
}

void AnimatedTextureProcessor::OnAdvanceFrame(Clock::duration delta_time) {
  time_line_ += delta_time;

  // Dequeue any completed tasks from the decoding thread and push into
  // ready_to_upload_ priority queue.
  AnimatedTexturePtr anim_texture;
  while (decode_queue_.Dequeue(&anim_texture)) {
    ready_to_upload_.push(anim_texture);
  }

  // Pop off ready frames whose timestamp has passed
  auto* texture_factory = registry_->Get<TextureFactory>();
  while (!ready_to_upload_.empty()) {
    // If next ready frame is in the future, we're done.
    anim_texture = ready_to_upload_.top();
    if (anim_texture == nullptr ||
        anim_texture->latest_frame_show_time > time_line_) {
      break;
    }
    ready_to_upload_.pop();

    // Texture is no longer being used.
    auto texture = anim_texture->texture.lock();
    if (texture == nullptr) {
      continue;
    }
    // Upload texture data to GL.
    // TODO: Investigate a more efficient means of updating the
    // texture. For example, on Android, we could use an external texture and
    // decode directly to the GL texture memory.
    texture_factory->UpdateTexture(TexturePtr(texture),
                                   std::move(anim_texture->latest_frame));

    // Push onto decoding thread for next update.
    decode_queue_.Enqueue(anim_texture, [&](AnimatedTexturePtr* anim_texture) {
      DecodeFrame(*anim_texture);
    });
  }
}

void AnimatedTextureProcessor::DecodeFrame(AnimatedTexturePtr anim_texture) {
  anim_texture->latest_frame = anim_texture->animated_image->DecodeNextFrame();
  anim_texture->latest_frame_show_time +=
      anim_texture->animated_image->GetCurrentFrameDuration();
}

}  // namespace lull
