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

#include "redux/tools/anim_pipeline/anim_curve.h"

#include <cmath>

#include "redux/engines/animation/spline/cubic_curve.h"

namespace redux::tool {

AnimCurve::AnimCurve(AnimChannelType type, size_t reserve_size) : type_(type) {
  if (reserve_size > 0) {
    nodes_.reserve(reserve_size);
  }
}

void AnimCurve::AddNode(float time, float value) {
  nodes_.emplace_back(time, value);
}

void AnimCurve::ShiftTime(float time_in_ms) {
  for (Node& n : nodes_) {
    n.time_ms += time_in_ms;
  }
}

void AnimCurve::ExtendToTime(float time_in_ms) {
  // Ignore empty or constant channels.
  if (nodes_.size() <= 1) {
    return;
  }

  const Node last = nodes_.back();

  // Ignore channels that are already long enough.
  if (last.time_ms >= time_in_ms) {
    return;
  }
  // Append a point with 0 derivative at the back, if required.
  // This ensures that the extra segment is a flat line.
  if (last.derivative != 0.0f) {
    nodes_.emplace_back(last.time_ms, last.value, 0.0f);
  }
  nodes_.emplace_back(time_in_ms, last.value, 0.0f);
}

void AnimCurve::Finish(const Tolerances& tolerances) {
  const float tolerance = ToleranceForOp(tolerances, type_);
  GenerateDerivatives();
  Minimize(tolerance);
  PruneNodes(tolerance, tolerances.derivative_angle);

  // If, after all that work, we end up with a "flat" curve, we can remove all
  // the extraneous nodes.
  auto const_value = GetConstValue(tolerances);
  if (const_value) {
    if (*const_value == ChannelDefaultValue(type_)) {
      nodes_.clear();
    } else {
      nodes_.erase(nodes_.begin() + 1, nodes_.end());
    }
  }
}

std::optional<float> AnimCurve::GetConstValue(
    const Tolerances& tolerances) const {
  if (nodes_.empty()) {
    return ChannelDefaultValue(type_);
  } else if (nodes_.size() == 1) {
    return nodes_.front().value;
  }

  const float tolerance = ToleranceForOp(tolerances, type_);

  // Scan the entire curve for anything that indicates that it is a non-const
  // curve.
  const float first_value = nodes_[0].value;
  for (size_t i = 1; i < nodes_.size(); ++i) {
    // A value in the curve differs from the initial value, so the curve is
    // not constant.
    const float test_value = nodes_[i].value;
    if (std::fabs(test_value - first_value) > tolerance) {
      return std::nullopt;
    }
  }
  return first_value;
}

void AnimCurve::GenerateDerivatives() {
  if (nodes_.empty()) {
    return;
  }

  // Generate a list of tangents between each pair of nodes_.
  std::vector<float> tangents;
  if (nodes_.size() == 1) {
    tangents.push_back(0.0f);
  } else {
    tangents.reserve(nodes_.size() - 1);
    for (size_t i = 1; i < nodes_.size(); ++i) {
      const float dx = nodes_[i].time_ms - nodes_[i - 1].time_ms;
      const float dy = nodes_[i].value - nodes_[i - 1].value;
      const float tangent = (dx > 0.0f) ? (dy / dx) : 0.0f;
      tangents.push_back(tangent);
    }
  }

  // TODO: insert duplicate nodes for disjoint tangents or values

  for (size_t i = 0; i < nodes_.size(); ++i) {
    if (i == 0) {
      nodes_[i].derivative = tangents.front();
    } else if (i == nodes_.size() - 1) {
      nodes_[i].derivative = tangents.back();
    } else {
      const float left = tangents[i - 1];
      const float right = tangents[i];
      // If the curve is disjoint (ie. two values at the same time), then
      // do not use the tangent from the disjoint side.
      if (nodes_[i].time_ms == nodes_[i + 1].time_ms) {
        nodes_[i].derivative = left;
      } else if (nodes_[i].time_ms == nodes_[i - 1].time_ms) {
        nodes_[i].derivative = right;
      } else {
        nodes_[i].derivative = (left + right) * 0.5f;
      }
    }
  }
}

void AnimCurve::Minimize(float tolerance) {
  std::vector<Node> minimal_nodes;

  // Break the curve down into segments and process them depth-first so that the
  // resulting nodes are in chronological order.
  std::vector<CurveSegment> segments;
  segments.emplace_back(nodes_);
  while (!segments.empty()) {
    CurveSegment segment = segments.back();
    segments.pop_back();

    // Create cubic that covers the entire range from time_start ~ time_end.
    // The cubic `c` is shifted to the left, to start at 0 instead of
    // time_start.
    // This is to maintain floating-point precision.
    const float time_start = segment.front().time_ms;
    const float time_end = segment.back().time_ms;
    const float time_width = time_end - time_start;
    const CubicCurve c(
        CubicInit(segment.front().value, segment.front().derivative,
                  segment.back().value, segment.back().derivative, time_width));

    // Find the worst intermediate val in for this cubic.
    // That is, the index into `s.vals` where the cubic evaluation is most
    // inaccurate.
    float worst_diff = 0.0f;
    size_t worst_idx = 0;
    for (size_t i = 1; i < segment.size() - 1; ++i) {
      const float cubic_val = c.Evaluate(segment[i].time_ms - time_start);
      const float curve_val = segment[i].value;
      const float diff_val = fabs(cubic_val - curve_val);
      if (diff_val > worst_diff) {
        worst_idx = i;
        worst_diff = diff_val;
      }
    }

    // If the cubic is off by a lot, divide the curve into two curves at the
    // worst time. Note that the recursion will end, at worst, when
    // segment.count == 2.
    if (worst_idx > 0 && worst_diff > tolerance) {
      // Push the "end" segment on first so that the "start" segment is
      // processed first, resulting in a depth-first search.
      segments.emplace_back(segment.begin() + worst_idx,
                            segment.size() - worst_idx);
      segments.emplace_back(segment.begin(), worst_idx + 1);
    } else {
      // Otherwise, the generated cubic is good enough, so record it.

      // Only push the start node if it differs from the previously pushed end
      // node. Most of the time it will be the same.
      const bool start_matches_prev =
          !minimal_nodes.empty() && minimal_nodes.back() == segment.front();
      if (!start_matches_prev) {
        minimal_nodes.push_back(segment.front());
      }
      minimal_nodes.push_back(segment.back());
    }
  }

  nodes_ = std::move(minimal_nodes);
}

// Returns true if all the nodes between the start node and end node of the
// given segment are redundant (i.e. the curve would still generate the same
// values within the given tolerances without those nodes).
bool AreIntermediateNodesRedundant(AnimCurve::CurveSegment segment,
                                   float value_tolerance,
                                   float derivative_angle_tolerance) {
  // If the start and end nodes occur at the same time and are equal, then
  // ignore everything inbetween them.
  const AnimCurve::Node& start = segment.front();
  const AnimCurve::Node& end = segment.back();
  if (start.NearlyEqual(end, value_tolerance, derivative_angle_tolerance)) {
    return true;
  }

  // Construct cubic curve `c` that skips all the intermediate nodes.
  const float cubic_width = end.time_ms - start.time_ms;
  const CubicCurve c(CubicInit(start.value, start.derivative, end.value,
                               end.derivative, cubic_width));

  // For each intermediate node, check if the cubic `c` is close.
  for (size_t i = 1; i < segment.size() - 1; ++i) {
    // Evaluate `c` at the time of `mid`.
    const AnimCurve::Node& mid = segment[i];
    const float mid_time = static_cast<float>(mid.time_ms - start.time_ms);
    const float mid_val = c.Evaluate(mid_time);
    const float mid_derivative = c.Derivative(mid_time);

    // If the mid point is on the curve, it's redundant.
    const float derivative_angle_error =
        DerivativeAngle(mid_derivative - mid.derivative);
    const bool mid_on_c =
        std::fabs(mid_val - mid.value) < value_tolerance &&
        std::fabs(derivative_angle_error) < derivative_angle_tolerance;
    if (!mid_on_c) {
      return false;
    }
  }

  // All mid points are redundant.
  return true;
}

void AnimCurve::PruneNodes(float tolerance, float derivative_angle_tolerance) {
  // For every node try to prune as many redunant nodes that come after it.
  // A node is redundant if the spline evaluates to the similar value even if
  // it doesn't exists (note: here "same value" means within `tolerances`).
  std::vector<bool> prune(nodes_.size(), false);
  for (size_t i = 0; i < nodes_.size();) {
    size_t next_i = i + 1;
    for (size_t j = i + 2; j < nodes_.size(); ++j) {
      const CurveSegment segment(&nodes_[i], j - i + 1);
      const bool redundant = AreIntermediateNodesRedundant(
          segment, tolerance, derivative_angle_tolerance);
      if (redundant) {
        prune[j - 1] = true;
        next_i = j;
      }
    }
    i = next_i;
  }

  // Remove all nodes marked to be pruned.
  size_t write = 0;
  for (size_t read = 0; read < nodes_.size(); ++read) {
    if (!prune[read]) {
      nodes_[write] = nodes_[read];
      ++write;
    }
  }
  nodes_.erase(nodes_.begin() + write, nodes_.end());
}

}  // namespace redux::tool
