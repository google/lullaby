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

#ifndef LULLABY_TOOLS_ANIM_PIPELINE_IMPORT_OPTIONS_H_
#define LULLABY_TOOLS_ANIM_PIPELINE_IMPORT_OPTIONS_H_

#include "lullaby/generated/axis_system_generated.h"
#include "lullaby/tools/anim_pipeline/tolerances.h"

namespace lull {
namespace tool {

/// Options for importing animation data from a source asset.
struct ImportOptions {
  /// If true, allow animations to start at a non-zero time.
  bool preserve_start_time = false;

  /// If true, allow each channel to end at a different time.
  bool stagger_end_times = false;

  /// If true, never collapse scale channels into a single uniform scale
  /// channel.
  bool no_uniform_scale = false;

  /// If true, output curves using the scale-quaternion-translation
  /// representation of each bone's transform.
  bool sqt_animations = false;

  /// Amount the output curves are allowed to deviate from the input curves to
  /// assist animation compression. Differs by operation type.
  Tolerances tolerances;

  /// Which axes are up, front, and left.
  AxisSystem axis_system = AxisSystem_Unspecified;

  /// The number of cm equal to one unit. Used to alter coordinate systems on
  /// asset import and ignored if less than or equal to 0.
  float cm_per_unit = -1.f;

  /// Multiplier applied to the computed scale
  float scale_multiplier = 1.0f;

  /// If true, emit a AnimSetFb with the clips found in the source file, rather
  /// than the whole timeline exported as a single clip.
  bool import_clips = false;

  // Desire importer to be thread safe.  For example, for  ImportAsset,
  // turns off default singleton logger (which breaks thread safety), but
  // results in less verbose error messages. The value is defaulted to false for
  // backwards compatibility.  Note:  This flag currently affects only on
  // the Assimp importer and NOT the Fbx importer.
  bool desire_thread_safe = false;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_ANIM_PIPELINE_IMPORT_OPTIONS_H_
