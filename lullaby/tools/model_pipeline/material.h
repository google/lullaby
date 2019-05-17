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

#ifndef LULLABY_TOOLS_MODEL_PIPELINE_MATERIAL_H_
#define LULLABY_TOOLS_MODEL_PIPELINE_MATERIAL_H_

#include <string>
#include <unordered_map>
#include "lullaby/util/typeid.h"
#include "lullaby/util/variant.h"
#include "lullaby/tools/model_pipeline/texture_info.h"

namespace lull {
namespace tool {

// Collection of properties and texture that describes a material to be applied
// to a single Surface on the mesh.
struct Material {
  Material() {}

  std::string name;
  std::unordered_map<std::string, Variant> properties;
  std::unordered_map<std::string, TextureInfo> textures;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_MODEL_PIPELINE_MATERIAL_H_
