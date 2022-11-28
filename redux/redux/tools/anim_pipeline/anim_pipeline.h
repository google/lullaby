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

#ifndef REDUX_TOOLS_ANIM_PIPELINE_ANIM_PIPELINE_H_
#define REDUX_TOOLS_ANIM_PIPELINE_ANIM_PIPELINE_H_

#include <memory>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "redux/modules/base/data_container.h"
#include "redux/tools/anim_pipeline/animation.h"
#include "redux/tools/anim_pipeline/import_options.h"
#include "redux/tools/common/log_utils.h"

namespace redux::tool {

// Performs the controlling logic of importing data, processing it with
// additional properties, and exporting it to a ReduxAnim object.
class AnimPipeline {
 public:
  AnimPipeline(Logger& log) : log_(log) {}

  // Function that imports an asset into an Animation.
  using ImportFn =
      std::function<AnimationPtr(std::string_view, const ImportOptions&)>;

  // Registers a specific asset file type (based on its extension) with a
  // function that can be used to import that asset into a Animation object.
  void RegisterImporter(ImportFn importer, std::string_view extension);

  // Imports animation data using the specified options and returns a
  // DataContainer storing the AnimAssetDef binary.
  DataContainer Build(std::string_view uri, const ImportOptions& opts);

 private:
  // Returns the importer for the given resource.
  ImportFn GetImporter(std::string_view uri) const;

  absl::flat_hash_map<std::string, ImportFn> importers_;
  Logger& log_;
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_ANIM_PIPELINE_ANIM_PIPELINE_H_
