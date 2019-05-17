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

#include "lullaby/tools/anim_pipeline/anim_pipeline.h"

#include "lullaby/util/filename.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/tools/anim_pipeline/export.h"

namespace lull {
namespace tool {

void AnimPipeline::RegisterImporter(ImportFn importer, string_view extension) {
  importers_[std::string(extension)] = std::move(importer);
}

bool AnimPipeline::CanImport(const std::string& extension) {
  auto iter = importers_.find(extension);
  if (iter == importers_.end()) {
    return false;
  }
  return true;
}

bool AnimPipeline::Import(const std::string& filename,
                          const ImportOptions& opts) {
  std::string extension = GetExtensionFromFilename(filename);
  // Convert extension to lower case (e.g. .FBX -> .fbx).
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](unsigned char c) { return tolower(c); });
  auto iter = importers_.find(extension);
  if (iter == importers_.end()) {
    LOG(ERROR) << "No matching importer for '" << extension << "'.";
    return false;
  }

  std::vector<Animation> anims = iter->second(filename, opts);

  for (auto& anim : anims) {
    // Force the animation to start from 0 if requested.
    if (!opts.preserve_start_time) {
      anim.ShiftTime(-anim.MinAnimatedTime());
    }

    // Force all animation channels to be the same length if requested.
    if (!opts.stagger_end_times) {
      anim.ExtendChannelsToTime(anim.MaxAnimatedTime());
    }
  }
  if (opts.import_clips) {
    for (auto& anim : anims) {
      exported_animations_.push_back(
          ExportedAnimation{anim.GetName(),
                            MakeUnique<Animation>(anim),
                            ExportAnimation(anim)});
    }
  } else {
    exported_animations_.push_back(
        ExportedAnimation{anims[0].GetName(),
                          MakeUnique<Animation>(anims[0]),
                          ExportAnimation(anims[0])});
  }
  return true;
}

}  // namespace tool
}  // namespace lull
