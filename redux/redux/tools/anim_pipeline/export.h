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

#ifndef REDUX_TOOLS_ANIM_PIPELINE_EXPORT_H_
#define REDUX_TOOLS_ANIM_PIPELINE_EXPORT_H_

#include "redux/modules/base/data_container.h"
#include "redux/tools/anim_pipeline/animation.h"
#include "redux/tools/common/log_utils.h"

namespace redux::tool {

// Generates a DataContainer storing an AnimAssetDef binary object from the
// provided animation.
DataContainer ExportAnimation(AnimationPtr anim, Logger& log);

}  // namespace redux::tool

#endif  // REDUX_TOOLS_ANIM_PIPELINE_EXPORT_H_
