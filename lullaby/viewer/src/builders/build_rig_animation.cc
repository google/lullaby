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

#include "lullaby/viewer/src/builders/build_rig_animation.h"

#include "lullaby/viewer/src/file_manager.h"
#include "lullaby/util/filename.h"
#include "motive/src/anim_pipeline/anim_pipeline.h"

namespace lull {
namespace tool {

bool BuildRigAnimation(Registry* registry, const std::string& target,
                       const std::string& out_dir) {
  const std::string name = RemoveDirectoryAndExtensionFromFilename(target);

  if (GetExtensionFromFilename(target) != ".motiveanim") {
    LOG(ERROR) << "Target file must be a .motiveanim file: " << target;
    return false;
  }

  auto* file_manager = registry->Get<FileManager>();

  motive::AnimPipelineArgs args;
  args.fbx_file = file_manager->GetRealPath(name + ".fbx");
  args.output_file = out_dir + name + ".motiveanim";

  fplutil::Logger log;
  const int result = motive::RunAnimPipeline(args, log);
  if (result != 0) {
    LOG(ERROR) << "Error building animation: " << target;
    return false;
  }
  return true;
}

}  // namespace tool
}  // namespace lull
