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

#ifndef LULLABY_SYSTEMS_RENDER_ANIMATED_TEXTURE_PROCESSOR_H_
#define LULLABY_SYSTEMS_RENDER_ANIMATED_TEXTURE_PROCESSOR_H_

#include <memory>
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

#include "lullaby/modules/render/image_decode.h"
#include "lullaby/systems/render/texture.h"
#include "lullaby/util/async_processor.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/registry.h"

namespace lull {

// Manages decoding animated textures like WebP files and updating the
// corresponding texture instance.
class AnimatedTextureProcessor {
 public:
  explicit AnimatedTextureProcessor(Registry* registry);
  AnimatedTextureProcessor(const AnimatedTextureProcessor&) = delete;
  AnimatedTextureProcessor& operator=(const AnimatedTextureProcessor&) = delete;

  void OnAdvanceFrame(Clock::duration delta_time);

  void Animate(const TexturePtr& texture, AnimatedImagePtr animated_image);

 private:
  using WeakTexturePtr = std::weak_ptr<Texture>;
  struct AnimatedTexture {
    std::string raw_data;
    WeakTexturePtr texture;

    // Handle for interacting with the underlying image format decoder.
    AnimatedImagePtr animated_image;

    // Decoded frame data
    ImageData latest_frame;
    Clock::time_point latest_frame_show_time;
  };
  using AnimatedTexturePtr = std::shared_ptr<AnimatedTexture>;

  // Used to sort animated textures in a priority queue by time to next frame.
  // Those frames that should be shown sooner are at the front of the queue.
  // Both inputs must not be null.
  struct CompareAnimatedTexturePtr {
    bool operator()(const AnimatedTexturePtr& a,
                    const AnimatedTexturePtr& b) const {
      return a->latest_frame_show_time > b->latest_frame_show_time;
    }
  };

  // Called on background decoding thread.
  void DecodeFrame(AnimatedTexturePtr texture);

  Registry* registry_;
  Clock::time_point time_line_;

  // Queue to process images on background decoding thread.
  AsyncProcessor<AnimatedTexturePtr> decode_queue_;

  // Priority queue of textures with next frame ready, ordered by timestamp of
  // when to display the next frame.
  std::priority_queue<AnimatedTexturePtr, std::vector<AnimatedTexturePtr>,
                      CompareAnimatedTexturePtr>
      ready_to_upload_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::AnimatedTextureProcessor);

#endif  // LULLABY_SYSTEMS_RENDER_ANIMATED_TEXTURE_PROCESSOR_H_
