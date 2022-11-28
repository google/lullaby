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

#ifndef REDUX_TOOLS_ANIM_PIPELINE_IMPORT_OPTIONS_H_
#define REDUX_TOOLS_ANIM_PIPELINE_IMPORT_OPTIONS_H_

#include "redux/tools/anim_pipeline/tolerances.h"
#include "redux/tools/common/axis_system.h"

namespace redux::tool {

// Options for importing animation data from a source asset.
struct ImportOptions {
  // If true, allow animations to start at a non-zero time.
  bool preserve_start_time = false;

  // If true, allow each channel to end at a different time.
  bool stagger_end_times = false;

  // Amount the output curves are allowed to deviate from the input curves to
  // assist animation compression.
  Tolerances tolerances;

  // Which axes are up, front, and left.
  AxisSystem axis_system = AxisSystem::Unspecified;

  // Defines the unit the pipeline expects positions to be in compared to
  // centimeters (the standard unit for FBX). For example, 100.0 would be in
  // contexts where world units are measured in meters, and 2.54 would be for
  // inches. Keep at 0 to leave asset units as-is.
  float cm_per_unit = 0.0f;

  // General scale multiplier applied to the animation.
  float scale_multiplier = 1.0f;

  // Desire importer to be thread safe.  For example, for  ImportAsset,
  // turns off default singleton logger (which breaks thread safety), but
  // results in less verbose error messages. The value is defaulted to false for
  // backwards compatibility.  Note:  This flag currently affects only on
  // the Assimp importer and NOT the Fbx importer.
  bool desire_thread_safe = false;
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_ANIM_PIPELINE_IMPORT_OPTIONS_H_
