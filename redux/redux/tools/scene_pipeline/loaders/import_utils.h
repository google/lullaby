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

#ifndef REDUX_TOOLS_SCENE_PIPELINE_LOADERS_IMPORT_UTILS_H_
#define REDUX_TOOLS_SCENE_PIPELINE_LOADERS_IMPORT_UTILS_H_

#include <string_view>

#include "redux/tools/scene_pipeline/buffer.h"
#include "redux/tools/scene_pipeline/image.h"
#include "redux/tools/scene_pipeline/loaders/import_options.h"
#include "redux/tools/scene_pipeline/scene.h"

namespace redux::tool {

// Loads an image from the given path and adds it to the scene.
ImageIndex LoadImageIntoScene(Scene* scene, const ImportOptions& opts,
                              std::string_view path);

// Decodes an image from the encoded byte data and adds it to the scene.
ImageIndex DecodeImageIntoScene(Scene* scene, const ImportOptions& opts,
                                ByteSpan data);

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SCENE_PIPELINE_LOADERS_IMPORT_UTILS_H_
