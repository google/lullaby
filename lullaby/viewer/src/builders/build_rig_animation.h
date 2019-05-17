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

#ifndef LULLABY_VIEWER_SRC_BUILDERS_BUILD_RIG_ANIMATION_H_
#define LULLABY_VIEWER_SRC_BUILDERS_BUILD_RIG_ANIMATION_H_

#include <string>
#include "lullaby/util/registry.h"

namespace lull {
namespace tool {

// Builds the |target| .motiveanim file in the |out_dir|, creating it from a
// source asset file (eg. fbx) as necessary.
bool BuildRigAnimation(Registry* registry, const std::string& target,
                       const std::string& out_dir);

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_VIEWER_SRC_BUILDERS_BUILD_RIG_ANIMATION_H_
