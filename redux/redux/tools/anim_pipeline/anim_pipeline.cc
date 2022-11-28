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

#include "redux/tools/anim_pipeline/anim_pipeline.h"

#include "redux/modules/base/filepath.h"
#include "redux/tools/anim_pipeline/export.h"

namespace redux::tool {

static std::string ToLower(std::string_view str) {
  std::string result(str);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return tolower(c); });
  return result;
}

void AnimPipeline::RegisterImporter(ImportFn importer,
                                    std::string_view extension) {
  importers_[ToLower(extension)] = std::move(importer);
}

AnimPipeline::ImportFn AnimPipeline::GetImporter(std::string_view uri) const {
  std::string ext = ToLower(GetExtension(uri));
  auto iter = importers_.find(ext);
  return iter != importers_.end() ? iter->second : nullptr;
}

DataContainer AnimPipeline::Build(std::string_view uri,
                                  const ImportOptions& opts) {
  ImportFn importer = GetImporter(uri);
  CHECK(importer) << "Unable to find importer for: " << uri;

  AnimationPtr anim = importer(uri, opts);
  CHECK(anim) << "Unable to import animation: " << uri;

  if (!opts.preserve_start_time) {
    anim->ShiftTime(-anim->MinAnimatedTimeMs());
  }
  if (!opts.stagger_end_times) {
    anim->ExtendChannelsToTime(anim->MaxAnimatedTimeMs());
  }
  return ExportAnimation(anim, log_);
}

}  // namespace redux::tool
