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

#include "lullaby/tools/anim_pipeline/anim_curve.h"

namespace lull {
namespace tool {

AnimCurve::AnimCurve() : type(motive::kInvalidMatrixOperation) {}

AnimCurve::AnimCurve(motive::MatrixOperationType type, size_t reserve_size)
    : type(type) {
  if (reserve_size > 0) {
    times.reserve(reserve_size);
    values.reserve(reserve_size);
    derivatives.reserve(reserve_size);
  }
}

void AnimCurve::AddNode(float time, float value, float derivative) {
  times.push_back(time);
  values.push_back(value);
  derivatives.push_back(derivative);
}

static float DetermineTimeForValue(float v0, float t0, float v1, float t1,
                                   float target) {
  const float rel_time = v0 != v1 ? ((v0 - target) / (v0 - v1)) : 0.f;
  const float mid = t0 + (rel_time * (t1 - t0));
  return mid;
}

void AnimCurve::AdjustForModularAngles(float threshold) {
  if (!motive::RotateOp(type)) {
    return;
  }

  const float kEpsilon = 0.01f;
  threshold = (2.f * mathfu::kPi) - threshold;

  for (size_t i = values.size() - 1; i > 1; --i) {
    const float curr = values[i];
    const float prev = values[i - 1];
    const float t0 = times[i - 1];
    const float t1 = times[i];

    if (curr > prev + threshold) {
      const float v0 = prev + (2.f * mathfu::kPi);
      const float v1 = curr;
      const float time = DetermineTimeForValue(v0, t0, v1, t1, mathfu::kPi);

      values.insert(values.begin() + i, mathfu::kPi - kEpsilon);
      values.insert(values.begin() + i, -mathfu::kPi + kEpsilon);
      times.insert(times.begin() + i, time);
      times.insert(times.begin() + i, time);
      // Add dummy derivate entries so that GenerateDerivates() doesn't fail.
      derivatives.insert(derivatives.begin() + i, 0.f);
      derivatives.insert(derivatives.begin() + i, 0.f);
    } else if (prev > curr + threshold) {
      const float v0 = prev;
      const float v1 = curr + (2.f * mathfu::kPi);
      const float time = DetermineTimeForValue(v0, t0, v1, t1, mathfu::kPi);

      values.insert(values.begin() + i, -mathfu::kPi + kEpsilon);
      values.insert(values.begin() + i, mathfu::kPi - kEpsilon);
      times.insert(times.begin() + i, time);
      times.insert(times.begin() + i, time);
      // Add dummy derivate entries so that GenerateDerivates() doesn't fail.
      derivatives.insert(derivatives.begin() + i, 0.f);
      derivatives.insert(derivatives.begin() + i, 0.f);
    }
  }
}

void AnimCurve::GenerateDerivatives() {
  if (derivatives.empty()) {
    return;
  }

  // Generate a list of tangents between each pair of nodes.
  std::vector<float> tangents;
  if (values.size() == 1) {
    tangents.push_back(0.f);
  } else {
    tangents.reserve(values.size() - 1);
    for (size_t i = 1; i < values.size(); ++i) {
      const float dx = times[i] - times[i - 1];
      const float dy = values[i] - values[i - 1];
      const float tangent = (dx > 0.f) ? (dy / dx) : 0.f;
      tangents.push_back(tangent);
    }
  }

  // TODO: insert duplicate nodes for disjoint tangents or values

  for (size_t i = 0; i < values.size(); ++i) {
    if (i == 0) {
      derivatives[i] = tangents.front();
    } else if (i == values.size() - 1) {
      derivatives[i] = tangents.back();
    } else {
      const float left = tangents[i - 1];
      const float right = tangents[i];
      // If the curve is disjoint (ie. two values at the same time), then
      // do not use the tangent from the disjoint side.
      if (times[i] == times[i + 1]) {
        derivatives[i] = left;
      } else if (times[i] == times[i - 1]) {
        derivatives[i] = right;
      } else {
        derivatives[i] = (left + right) * 0.5f;
      }
    }
  }
}

Optional<float> AnimCurve::GetConstValue(Tolerances tolerances) const {
  if (values.empty()) {
    return motive::OperationDefaultValue(type);
  }

  float tolerance = 0.1f;
  if (motive::RotateOp(type)) {
    tolerance = tolerances.rotate;
  } else if (motive::TranslateOp(type)) {
    tolerance = tolerances.translate;
  } else if (motive::ScaleOp(type)) {
    tolerance = tolerances.scale;
  } else if (motive::QuaternionOp(type)) {
    tolerance = tolerances.quaternion;
  }

  // Scan the entire curve for anything that indicates that it is a non-const
  // curve.
  const float first_value = values[0];
  for (size_t i = 1; i < values.size(); ++i) {
    // A value in the curve differs from the initial value, so the curve is
    // not constant.
    const float test_value = values[i];
    if (fabs(test_value - first_value) > tolerance) {
      return NullOpt;
    }
  }
  return first_value;
}

}  // namespace tool
}  // namespace lull
