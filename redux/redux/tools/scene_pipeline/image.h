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

#ifndef REDUX_TOOLS_SCENE_PIPELINE_IMAGE_H_
#define REDUX_TOOLS_SCENE_PIPELINE_IMAGE_H_

#include <vector>

#include "redux/tools/scene_pipeline/buffer_view.h"
#include "redux/tools/scene_pipeline/index.h"

namespace redux::tool {

// Data for a single image in the scene to be used as a texture.
struct Image {
  // The raw pixel data for the image. A mipmapped image will have a different
  // buffer for each mipmap level.
  std::vector<BufferView> pixels;

  // Dimensions of largest mipmap level. Images with mipmaps are assumed to be
  // power-of-two and each subsequent mipmap level is half the size of the
  // previous level.
  int width = 0;
  int height = 0;
  int channels_per_pixel = 0;  // e.g. RGB=3, RGBA=4
  int bytes_per_channel = 0;   // e.g. 8-bit = 1, 16-bit = 2
  int num_faces = 0;  // 1 = normal, 6 = cubemap
};

using ImageIndex = Index<Image>;

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SCENE_PIPELINE_IMAGE_H_
