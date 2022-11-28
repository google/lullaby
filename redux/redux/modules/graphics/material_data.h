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

#ifndef REDUX_MODULES_GRAPHICS_MATERIAL_DATA_H_
#define REDUX_MODULES_GRAPHICS_MATERIAL_DATA_H_

#include <string>
#include <vector>

#include "redux/modules/graphics/enums.h"
#include "redux/modules/graphics/texture_usage.h"
#include "redux/modules/var/var_table.h"

namespace redux {

// Information about a Material that can be used for rendering.
struct MaterialData {
  MaterialData() = default;

  // Textures and their intended usages.
  struct TextureData {
    TextureUsage usage;
    std::string texture;
  };

  std::string shading_model;
  std::vector<TextureData> textures;
  VarTable properties;
};

}  // namespace redux

#endif  // REDUX_MODULES_GRAPHICS_MATERIAL_DATA_H_
