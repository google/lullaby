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

#ifndef LULLABY_TOOLS_ANIM_PIPELINE_EXPORT_H_
#define LULLABY_TOOLS_ANIM_PIPELINE_EXPORT_H_

#include "lullaby/util/common_types.h"
#include "lullaby/tools/anim_pipeline/animation.h"

namespace lull {
namespace tool {

/// Exports the animation to a MotiveAnim binary object.
ByteArray ExportAnimation(const Animation& animation);

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_ANIM_PIPELINE_EXPORT_H_
