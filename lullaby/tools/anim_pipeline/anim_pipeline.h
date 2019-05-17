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

#ifndef LULLABY_TOOLS_ANIM_PIPELINE_ANIM_PIPELINE_H_
#define LULLABY_TOOLS_ANIM_PIPELINE_ANIM_PIPELINE_H_

#include <functional>
#include <string>
#include <unordered_map>

#include "lullaby/util/common_types.h"
#include "lullaby/tools/anim_pipeline/animation.h"
#include "lullaby/tools/anim_pipeline/import_options.h"

namespace lull {
namespace tool {

/// Performs the controlling logic of importing data, processing it with
/// additional properties, and exporting it to a MotiveAnim object.
class AnimPipeline {
 public:
  struct ExportedAnimation {
    const std::string name;
    std::unique_ptr<Animation> anim;
    const ByteArray motive_anim;
  };

  AnimPipeline() {}

  /// Function that imports an asset into Animations.
  using ImportFn = std::function<std::vector<Animation>(
      const std::string& filename, const ImportOptions& opts)>;

  /// Registers a specific asset file type (based on its extension) with a
  /// function that can be used to import that asset into a Model object.
  void RegisterImporter(ImportFn importer, string_view extension);

  /// Returns true if this pipeline can a file with given extension.
  bool CanImport(const std::string& extension);

  /// Imports an asset using the specified import options. Returns true if
  /// import is successful, false otherwise. If true, use GetMotiveAnim() to
  /// retrieve the binary animation data.
  bool Import(const std::string& filename, const ImportOptions& opts);

  /// Return the number of resulting animations.  This will always be 1, unless
  /// options.import_clips is true.
  const int GetExportCount() const { return exported_animations_.size(); }

  const ExportedAnimation& GetExport(int index) const {
    return exported_animations_[index];
  }

 private:
  std::unordered_map<std::string, ImportFn> importers_;
  std::vector<ExportedAnimation> exported_animations_;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_ANIM_PIPELINE_ANIM_PIPELINE_H_
