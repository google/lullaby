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

#ifndef REDUX_TOOLS_SCENE_PIPELINE_LOADERS_GLTF_UTILS_H_
#define REDUX_TOOLS_SCENE_PIPELINE_LOADERS_GLTF_UTILS_H_

#include <string>

#include "redux/tools/scene_pipeline/drawable.h"
#include "redux/tools/scene_pipeline/sampler.h"
#include "redux/tools/scene_pipeline/type_id.h"
#include "cgltf.h"

namespace redux::tool {

// Returns the size (in bytes) of the given Type.
int TypeSize(TypeId type);

// Maps a cgltf attribute type to a redux::tool Type.
TypeId AttributeType(cgltf_type type);

// Maps a cgltf component type to a redux::tool Type.
TypeId ComponentType(cgltf_component_type component_type);

// Maps a cgltf primitive type to a redux::tool PrimitiveType.
Drawable::PrimitiveType PrimitiveType(cgltf_primitive_type type);

// Maps a cgltf attribute name to a redux::tool VertexBuffer attribute name.
std::string AttributeName(cgltf_attribute_type type);

// Maps a cgltf alpha mode to a redux::tool Material::AlphaMode.
int AlphaMode(cgltf_alpha_mode mode);

// Maps a OpenGL texture filter to a redux::tool Sampler::Filter.
Sampler::Filter TextureMinFilter(int filter);

// Maps a OpenGL texture filter to a redux::tool Sampler::Filter.
Sampler::Filter TextureMagFilter(int filter);

// Maps a OpenGL texture wrap mode to a redux::tool Sampler::WrapMode.
Sampler::WrapMode TextureWrapMode(int wrap_mode);

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SCENE_PIPELINE_LOADERS_GLTF_UTILS_H_
