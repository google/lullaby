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

#include "redux/tools/scene_pipeline/loaders/import_utils.h"

#include <string_view>
#include <utility>

#include "redux/tools/scene_pipeline/buffer.h"
#include "redux/tools/scene_pipeline/image.h"
#include "redux/tools/scene_pipeline/loaders/import_options.h"
#include "redux/tools/scene_pipeline/scene.h"
#include "absl/log/check.h"

namespace redux::tool {

ImageIndex LoadImageIntoScene(Scene* scene, const ImportOptions& opts,
                              std::string_view path) {
  Buffer file = opts.file_loader(path);
  return DecodeImageIntoScene(scene, opts, file.span());
}

ImageIndex DecodeImageIntoScene(Scene* scene, const ImportOptions& opts,
                                ByteSpan data) {
  ImportOptions::DecodedImage decoded_image = opts.image_decoder(data);
  CHECK_EQ(decoded_image.image.pixels.size(), decoded_image.buffers.size());

  ImageIndex image_index(scene->images.size());

  // We need to move the buffers from `decoded_image` into the scene. This also
  // requires us to update the buffer indices in the Image to point to the
  // buffers in the scene instead.
  Image& image = scene->images.emplace_back(std::move(decoded_image.image));
  for (int i = 0; i < decoded_image.buffers.size(); ++i) {
    image.pixels[i].buffer_index = BufferIndex(scene->buffers.size());
    scene->buffers.emplace_back(std::move(decoded_image.buffers[i]));
  }

  return image_index;
}

}  // namespace redux::tool
