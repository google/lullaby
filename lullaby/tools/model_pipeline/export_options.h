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

#ifndef LULLABY_TOOLS_MODEL_PIPELINE_EXPORT_OPTIONS_H_
#define LULLABY_TOOLS_MODEL_PIPELINE_EXPORT_OPTIONS_H_

namespace lull {
namespace tool {

// Options that control the export process.
struct ExportOptions {
  // If true all dependent textures will be embedded within the lullmodel.  If
  // false all dependent textures will be copied to the same folder as the
  // lullmodel.
  bool embed_textures = true;

  // If true paths embeded within the lullmodel will use paths relative to the
  // lullmodel itself.
  bool relative_path = false;

  // If true modify the 'name' field on textures to be a unique identifier.
  // This allows them to be remapped and addressed at runtime
  bool unique_texture_names = false;

  // If true, attempt a saving-throw on untextured materials by performing
  // a texture lookup based on the surface name.
  bool look_for_unlinked_textures = false;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_MODEL_PIPELINE_EXPORT_OPTIONS_H_
