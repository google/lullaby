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

#ifndef LULLABY_TOOLS_ANIM_PIPELINE_TOLERANCES_H_
#define LULLABY_TOOLS_ANIM_PIPELINE_TOLERANCES_H_

namespace lull {
namespace tool {

/// Amount the output curves are allowed to deviate from the input curves.
struct Tolerances {
  /// Amount output scale curves can deviate, unitless.  Default is half a
  /// percent.
  float scale = 0.005f;

  /// Amount output rotate curves can deviate, in radians.  Default is 0.5
  /// degrees in radians.
  float rotate = 0.00873f;

  /// Amount output translate curves can deviate, in scene's distance units.
  /// Default is arbitrary and should probably depend on model size.
  float translate = 0.01f;

  /// Amount output quaternion curves can deviate, unitless.  Default is
  /// arbitrary.
  float quaternion = 0.0005f;

  /// Amount derivative--converted to an angle in x/y--can deviate, in radians.
  /// Default is 0.5 degrees in radians.
  float derivative_angle = 0.00873f;

  /// Amount derivative--converted to an angle in x/y--can deviate, in radians,
  /// This value used when determining if an animation repeats or not.  Default
  /// is 10 degrees in radians.
  float repeat_derivative_angle = 0.1745f;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_ANIM_PIPELINE_TOLERANCES_H_
