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

#ifndef LULLABY_TOOLS_ANIM_PIPELINE_ANIM_CURVE_H_
#define LULLABY_TOOLS_ANIM_PIPELINE_ANIM_CURVE_H_

#include "motive/matrix_op.h"
#include "lullaby/util/optional.h"
#include "lullaby/tools/anim_pipeline/tolerances.h"

namespace lull {
namespace tool {

// Represents the curve for a single animation channel.
struct AnimCurve {
  AnimCurve();
  AnimCurve(motive::MatrixOperationType type, size_t reserve_size);

  // Adds a node to the curve.
  void AddNode(float time, float value, float derivative = 0.f);

  // For rotation curves, inserts additional nodes when the angle values appear
  // to change from pi to -pi (or vice versa).  The threshold is used to
  // determine if two existing nodes are "close enough" to each other to assume
  // that the curve should cross the pi-boundary rather than simply join the
  // two nodes.
  void AdjustForModularAngles(float threshold = 0.5f);

  // Calculates derivatives based on the times/values of neighbouring data
  // points on the curve.  This function will recalculate the derivative values
  // of all nodes in the curve.
  void GenerateDerivatives();

  // If the curve represents a constant, "flat" line, then this function returns
  // the value of the line, otherwise returns NullOpt (if the curve is, in fact,
  // curvy).
  //
  // The tolerance is used to determine if small variations in the values can be
  // ignored.
  Optional<float> GetConstValue(Tolerances tolerances) const;

  motive::MatrixOperationType type;
  std::vector<float> times;
  std::vector<float> values;
  std::vector<float> derivatives;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_ANIM_PIPELINE_ANIM_CURVE_H_
