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

#ifndef REDUX_TOOLS_MODEL_PIPELINE_MATERIAL_H_
#define REDUX_TOOLS_MODEL_PIPELINE_MATERIAL_H_

#include <string>

#include "absl/container/flat_hash_map.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/var/var.h"
#include "redux/tools/model_pipeline/texture_info.h"

namespace redux::tool {

// Collection of properties and texture that describes a material to be applied
// to a single Surface on the mesh.
struct Material {
  Material() = default;

  std::string name;
  absl::flat_hash_map<std::string, Var> properties;
  absl::flat_hash_map<std::string, TextureInfo> textures;
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_MODEL_PIPELINE_MATERIAL_H_
