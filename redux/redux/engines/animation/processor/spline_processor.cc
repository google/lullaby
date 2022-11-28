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

#include "redux/engines/animation/processor/spline_processor.h"

#include "redux/engines/animation/common.h"

namespace redux {

inline float DurationToSplineTime(absl::Duration duration) {
  return static_cast<float>(absl::ToDoubleMilliseconds(duration));
}

inline absl::Duration SplineTimeToDuration(float time) {
  return absl::Milliseconds(time);
}

inline SplinePlayback AsSplinePlayback(const AnimationPlayback& anim) {
  SplinePlayback spline;
  spline.playback_rate = anim.playback_rate;
  spline.blend_x = DurationToSplineTime(anim.blend_time);
  spline.start_x = DurationToSplineTime(anim.start_time);
  spline.y_offset = anim.value_offset;
  spline.y_scale = anim.value_scale;
  spline.repeat = anim.repeat;
  return spline;
}

SplineMotivator SplineProcessor::AllocateMotivator(int dimensions) {
  SplineMotivator motivator;
  AllocateMotivatorIndices(&motivator, dimensions);
  return motivator;
}

void SplineProcessor::SetSplines(Motivator::Index index, int dimensions,
                                 const CompactSpline* splines,
                                 const AnimationPlayback& playback) {
  // Return the local splines to the spline pool. We use external splines now.
  for (int i = index; i < index + dimensions; ++i) {
    FreeSplineForIndex(i);
  }

  // Initialize spline to follow way points.
  // Snaps the current value and velocity to the way point's start value
  // and velocity.
  interpolator_.SetSplines(index, dimensions, splines,
                           AsSplinePlayback(playback));
}

void SplineProcessor::SetTargets(Motivator::Index index, int dimensions,
                                 const float* values, const float* velocities,
                                 absl::Duration time) {
  for (int i = 0; i < dimensions; ++i) {
    CreateSplineToTarget(index + i, values[i], velocities[i], time);
  }
}

void SplineProcessor::AdvanceFrame(absl::Duration delta_time) {
  Defragment();
  auto spline_time = DurationToSplineTime(delta_time);
  interpolator_.AdvanceFrame(spline_time);
}

// Accessors to allow the user to get and set simluation values.
const float* SplineProcessor::Values(Motivator::Index index) const {
  return interpolator_.Ys(index);
}

void SplineProcessor::Velocities(Motivator::Index index, int dimensions,
                                 float* out) const {
  interpolator_.Derivatives(index, dimensions, out);
}

void SplineProcessor::Directions(Motivator::Index index, int dimensions,
                                 float* out) const {
  interpolator_.DerivativesWithoutPlayback(index, dimensions, out);
}

void SplineProcessor::TargetValues(Motivator::Index index, int dimensions,
                                   float* out) const {
  interpolator_.EndYs(index, dimensions, out);
}

void SplineProcessor::TargetVelocities(Motivator::Index index, int dimensions,
                                       float* out) const {
  interpolator_.EndDerivatives(index, dimensions, out);
}

void SplineProcessor::Differences(Motivator::Index index, int dimensions,
                                  float* out) const {
  interpolator_.YDifferencesToEnd(index, dimensions, out);
}

absl::Duration SplineProcessor::TimeRemaining(Motivator::Index index,
                                              int dimensions) const {
  float greatest = std::numeric_limits<float>::min();
  for (int i = 0; i < dimensions; ++i) {
    const float spline_time =
        interpolator_.EndX(index + i) - interpolator_.X(index + i);
    greatest = std::max(greatest, spline_time);
  }
  return SplineTimeToDuration(greatest);
}

absl::Duration SplineProcessor::SplineTime(Motivator::Index index) const {
  return SplineTimeToDuration(interpolator_.X(index));
}

void SplineProcessor::Splines(Motivator::Index index, int dimensions,
                              const CompactSpline** splines) const {
  interpolator_.Splines(index, dimensions, splines);
}

void SplineProcessor::SetSplineTime(Motivator::Index index, int dimensions,
                                    absl::Duration time) {
  interpolator_.SetXs(index, dimensions, DurationToSplineTime(time));
}

void SplineProcessor::SetSplinePlaybackRate(Motivator::Index index,
                                            int dimensions,
                                            float playback_rate) {
  interpolator_.SetPlaybackRates(index, dimensions, playback_rate);
}

void SplineProcessor::SetSplineRepeating(Motivator::Index index, int dimensions,
                                         bool repeat) {
  interpolator_.SetRepeating(index, dimensions, repeat);
}

bool SplineProcessor::Settled(Motivator::Index index, int dimensions,
                              float max_difference, float max_velocity) const {
  for (int i = 0; i < dimensions; ++i) {
    float difference = 0.f;
    interpolator_.YDifferencesToEnd(index + i, 1, &difference);
    if (std::fabs(difference) > max_difference) {
      return false;
    }

    float velocity = 0.f;
    interpolator_.Derivatives(index + i, 1, &velocity);
    if (std::fabs(velocity) > max_velocity) {
      return false;
    }
  }
  return true;
}

void SplineProcessor::CreateSplineToTarget(Motivator::Index index, float value,
                                           float velocity,
                                           absl::Duration time) {
  // If the first node specifies time=0 or there is no valid data in the
  // interpolator, we want to override the current values with the values
  // specified in the first node.
  const bool override_current =
      time == absl::ZeroDuration() || !interpolator_.Valid(index);

  // TODO(b/65298927):  It seems that the animation pipeline can produce data
  // that is out of range.  Instead of just using |value| directly, if
  // the interpolator is doing modular arithmetic, normalize the y value to
  // the modulator's range.
  const Interval& modular_range = interpolator_.ModularRange(index);

  const float node_y =
      modular_range.Size() > -0.f
          ? NormalizeWildValueWithinInterval(modular_range, value)
          : value;
  const float start_y =
      override_current ? node_y : interpolator_.NormalizedY(index);

  float velocity_at_index = 0.f;
  if (!override_current) {
    Velocities(index, 1, &velocity_at_index);
  }

  const float start_derivative =
      override_current ? velocity : velocity_at_index;

  CompactSpline* spline = data_[index].get();
  if (spline == nullptr) {
    // The default number of nodes is enough.
    data_[index] = AllocateSpline(CompactSpline::kDefaultMaxNodes);
    spline = data_[index].get();
  }

  Interval y_range;
  if (interpolator_.ModularArithmetic(index)) {
    // For modular splines, we need to expand the spline's y-range to match
    // the number of nodes in the spline. It's possible for the spline to jump
    // up the entire range every node, so the range has to be broad enough
    // to hold it all.
    //
    // Note that we only normalize the first value of the spline, and
    // subsequent values are allowed to curve out of the normalized range.
    y_range = interpolator_.ModularRange(index).Scaled(1.f);
  } else {
    // Add some buffer to the y-range to allow for intermediate nodes
    // that go above or below the supplied nodes.
    static constexpr float kYRangeBufferPercent = 1.2f;

    // Calculate the union of the y ranges in the target, then expand it a
    // little to allow for intermediate nodes that jump slightly beyond the
    // union's range.
    y_range = Interval(std::min(value, start_y), std::max(value, start_y));
    y_range = y_range.Scaled(kYRangeBufferPercent);
  }

  const float spline_time = DurationToSplineTime(time);
  const float x_granularity = CompactSpline::RecommendXGranularity(spline_time);
  spline->Init(y_range, x_granularity);
  spline->AddNode(0.0f, start_y, start_derivative);

  float y = value;
  if (!override_current) {
    // Use modular arithmetic for ranged values.
    if (modular_range.Size() > 0.f) {
      const float target_y = NormalizeWildValueWithinInterval(modular_range, y);
      const float x = target_y - start_y;
      const float length = modular_range.Size();
      const float adjustment = x <= modular_range.min  ? length
                               : x > modular_range.max ? -length
                                                       : 0.f;
      y = start_y + x + adjustment;
    }
    spline->AddNode(spline_time, y, velocity, kAddWithoutModification);
  }

  // Point the interpolator at the spline we just created. Always start our
  // spline at time 0.
  interpolator_.SetSplines(index, 1, spline, SplinePlayback());
}

bool SplineProcessor::SupportsCloning() { return true; }

void SplineProcessor::CloneIndices(Motivator::Index dst, Motivator::Index src,
                                   int dimensions, AnimationEngine* engine) {
  interpolator_.CopyIndices(
      dst, src, dimensions,
      [this](Motivator::Index index, const CompactSpline* src_spline) {
        CompactSplinePtr dest_spline = AllocateSpline(src_spline->max_nodes());
        *dest_spline = *src_spline;
        data_[index] = std::move(dest_spline);
        return data_[index].get();
      });
}

void SplineProcessor::ResetIndices(Motivator::Index index, int dimensions) {
  // Clear reference to this spline.
  interpolator_.ClearSplines(index, dimensions);

  // Return splines to the pool of splines.
  for (Motivator::Index i = index; i < index + dimensions; ++i) {
    FreeSplineForIndex(i);
  }
}

void SplineProcessor::MoveIndices(Motivator::Index old_index,
                                  Motivator::Index new_index, int dimensions) {
  for (int i = 0; i < dimensions; ++i) {
    using std::swap;
    swap(data_[new_index + i], data_[old_index + i]);
    data_[old_index + i].reset();
  }
  interpolator_.MoveIndices(old_index, new_index, dimensions);
}

void SplineProcessor::SetNumIndices(Motivator::Index num_indices) {
  data_.resize(num_indices);
  interpolator_.SetNumIndices(num_indices);
}

CompactSplinePtr SplineProcessor::AllocateSpline(CompactSplineIndex max_nodes) {
  // Return a spline from the pool. Eventually we'll reach a high water mark
  // and we will stop allocating new splines. The returned spline must have
  // enough nodes.
  for (size_t i = 0; i < spline_pool_.size(); ++i) {
    if (spline_pool_[i]->max_nodes() >= max_nodes) {
      using std::swap;
      swap(spline_pool_[i], spline_pool_.back());
      CompactSplinePtr spline = std::move(spline_pool_.back());
      spline_pool_.pop_back();
      return spline;
    }
  }

  // Create a spline with enough nodes otherwise.
  return CompactSpline::Create(max_nodes);
}

void SplineProcessor::FreeSplineForIndex(Motivator::Index index) {
  if (data_[index]) {
    using std::swap;
    spline_pool_.emplace_back();
    swap(data_[index], spline_pool_.back());
  }
}
}  // namespace redux
