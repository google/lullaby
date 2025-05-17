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

#ifndef REDUX_TOOLS_SCENE_PIPELINE_LOADERS_IMPORT_OPTIONS_H_
#define REDUX_TOOLS_SCENE_PIPELINE_LOADERS_IMPORT_OPTIONS_H_

#include <functional>
#include <string_view>
#include <vector>

#include "redux/tools/scene_pipeline/buffer.h"
#include "redux/tools/scene_pipeline/image.h"

namespace redux::tool {

// Configuration options for loading (importing) a Scene from external formats.
struct ImportOptions {
  // Loads a binary file from the given path into a Buffer. A default
  // FileLoader is provided in `std_load_file.h`.
  using FileLoader = std::function<Buffer(std::string_view)>;
  FileLoader file_loader;

  // A decoded image and its pixel data.
  struct DecodedImage {
    // Information about the image that was decoded.
    Image image;

    // The pixel data for the image. This may be multiple buffers, e.g. in the
    // case of cubemaps.
    std::vector<Buffer> buffers;
  };

  // Decodes an image from the given bytes. A default ImageDecoder is provided
  // in `stb_image_decoder.h`.
  using ImageDecoder = std::function<DecodedImage(ByteSpan)>;
  ImageDecoder image_decoder;
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SCENE_PIPELINE_LOADERS_IMPORT_OPTIONS_H_
