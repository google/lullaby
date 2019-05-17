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

#ifndef LULLABY_SCRIPTS_MODEL_PIPELINE_TEXTURE_INFO_H_
#define LULLABY_SCRIPTS_MODEL_PIPELINE_TEXTURE_INFO_H_

#include <string>
#include <vector>
#include "lullaby/generated/material_def_generated.h"
#include "lullaby/generated/texture_def_generated.h"

namespace lull {
namespace tool {

using TextureDataPtr = std::shared_ptr<std::vector<uint8_t>>;

// Information about a single Texture.
struct TextureInfo {
  TextureInfo() {}

  // The actual source file location for this texture. This is mutable because
  // it is set after importing based on the results of the TextureLocator.
  mutable std::string basename;
  mutable std::string abs_path;

  // Optionally, the texture data can be stored in memory instead of a file.
  mutable TextureDataPtr data;

  std::vector<MaterialTextureUsage> usages = {MaterialTextureUsage_BaseColor};
  TextureWrap wrap_s = TextureWrap_Repeat;
  TextureWrap wrap_t = TextureWrap_Repeat;
  bool premultiply_alpha = false;
  bool generate_mipmaps = false;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_SCRIPTS_MODEL_PIPELINE_TEXTURE_INFO_H_
