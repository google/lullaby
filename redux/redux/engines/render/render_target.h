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

#ifndef REDUX_ENGINES_RENDER_RENDER_TARGET_H_
#define REDUX_ENGINES_RENDER_RENDER_TARGET_H_

#include <memory>

#include "redux/modules/graphics/enums.h"
#include "redux/modules/graphics/image_data.h"

namespace redux {

// The "pixels" onto which rendering operations take place.
class RenderTarget {
 public:
  virtual ~RenderTarget() = default;

  RenderTarget(const RenderTarget&) = delete;
  RenderTarget& operator=(const RenderTarget&) = delete;

  // Returns the dimensions of the render target.
  vec2i GetDimensions() const;

  // Returns the contents of the render buffer.
  ImageData GetFrameBufferData();

 protected:
  RenderTarget() = default;
};

using RenderTargetPtr = std::shared_ptr<RenderTarget>;

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_RENDER_TARGET_H_
