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

#ifndef LULLABY_TOOLS_GLTF_CONVERTER_GLTF_CONVERTER_H_
#define LULLABY_TOOLS_GLTF_CONVERTER_GLTF_CONVERTER_H_

#include <functional>
#include "lullaby/util/common_types.h"
#include "lullaby/util/span.h"
#include "lullaby/util/string_view.h"

namespace lull {
namespace tool {

// Converts the provided GLTF data (ie. the .gltf json file contents) into a
// binary blob representing a GLB file.  The load_fn can be used to load
// additional files the GLTF references (eg. the bin file or textures).
ByteArray GltfToGlb(Span<uint8_t> gltf,
                    const std::function<ByteArray(string_view)>& load_fn);

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_GLTF_CONVERTER_GLTF_CONVERTER_H_
