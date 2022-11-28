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

#ifndef REDUX_TOOLS_ANIM_PIPELINE_ANIM_CURVE_H_
#define REDUX_TOOLS_ANIM_PIPELINE_ANIM_CURVE_H_

#include <cmath>
#include <optional>

#include "absl/types/span.h"
#include "redux/engines/animation/common.h"
#include "redux/tools/anim_pipeline/tolerances.h"

namespace redux::tool {

// Convert derivative to its angle in x/y space.
//  derivative 0 ==> angle 0
//  derivative 1 ==> angle 45 degrees
//  derivative +inf ==> angle 90 degrees
//  derivative -2 ==> angle -63.4 degrees
// Returns Angle, in radians, >= -pi and <= pi
inline float DerivativeAngle(float derivative) { return std::atan(derivative); }

// Represents the curve for a single animation channel.
class AnimCurve {
 public:
  // A single point on the curve.
  struct Node {
    Node(float time_ms, float value, float derivative = 0.0f)
        : time_ms(time_ms), value(value), derivative(derivative) {}

    bool operator==(const Node& rhs) const {
      return time_ms == rhs.time_ms && value == rhs.value &&
             derivative == rhs.derivative;
    }

    bool NearlyEqual(const Node& rhs, float value_tolerance,
                     float derivative_angle_tolerance) const {
      const float time_diff = time_ms - rhs.time_ms;
      const float value_diff = value - rhs.value;
      const float angle_diff = DerivativeAngle(derivative - rhs.derivative);
      return time_diff == 0.0f && std::fabs(value_diff) < value_tolerance &&
             std::fabs(angle_diff) < derivative_angle_tolerance;
    }

    float time_ms = 0.0f;
    float value = 0.0f;
    float derivative = 0.0f;
  };

  using CurveSegment = absl::Span<const Node>;

  AnimCurve(AnimChannelType type, size_t reserve_size);

  // Returns the type of the curve.
  AnimChannelType Type() const { return type_; }

  // Adds a node to the curve.
  void AddNode(float time_in_ms, float value);

  // Processes the nodes to create a "final" curve.
  void Finish(const Tolerances& tolerances);

  // Shifts all the nodes in the curve by the given time value.
  void ShiftTime(float time_in_ms);

  // Extends the length of the curve by adding a flat line the end of the curve.
  void ExtendToTime(float time_in_ms);

  // Returns the full list of nodes that make up the curve.
  CurveSegment Nodes() const { return nodes_; }

  // If the curve represents a constant, "flat" line, then this function returns
  // the value of the line, otherwise returns nullopt (if the curve is, in fact,
  // curvy).
  //
  // The tolerance is used to determine if small variations in the values can be
  // ignored.
  std::optional<float> GetConstValue(const Tolerances& tolerances) const;

 private:
  // Calculates derivatives based on the times/values of neighbouring data
  // points on the curve.  This function will recalculate the derivative values
  // of all nodes in the curve.
  void GenerateDerivatives();

  // Regenerates the curve using a minimal set of nodes. The tolerance value
  // is used to determine if the newly generated curve is "good enough" to
  // replace the existing curve.
  void Minimize(float tolerance);

  // Removes any nodes in the curve that are redundant.
  void PruneNodes(float tolerance, float derivative_angle_tolerance);

  AnimChannelType type_ = AnimChannelType::Invalid;
  std::vector<Node> nodes_;
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_ANIM_PIPELINE_ANIM_CURVE_H_
