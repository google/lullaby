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

#ifndef REDUX_ENGINES_ANIMATION_PROCESSOR_SPLINE_PROCESSOR_H_
#define REDUX_ENGINES_ANIMATION_PROCESSOR_SPLINE_PROCESSOR_H_

#include <vector>

#include "redux/engines/animation/common.h"
#include "redux/engines/animation/motivator/spline_motivator.h"
#include "redux/engines/animation/processor/anim_processor.h"
#include "redux/engines/animation/spline/bulk_spline_evaluator.h"
#include "redux/engines/animation/spline/compact_spline.h"
#include "redux/modules/base/typeid.h"

namespace redux {

class SplineProcessor : public AnimProcessor {
 public:
  explicit SplineProcessor(AnimationEngine* engine) : AnimProcessor(engine) {}

  SplineMotivator AllocateMotivator(int dimensions = 1);

  void SetSplines(Motivator::Index index, int dimensions,
                  const CompactSpline* splines,
                  const AnimationPlayback& playback);

  void SetTargets(Motivator::Index index, int dimensions, const float* values,
                  const float* velocities, absl::Duration time);

  void AdvanceFrame(absl::Duration delta_time) override;

  // Accessors to allow the user to get and set simluation values.
  const float* Values(Motivator::Index index) const;

  void Velocities(Motivator::Index index, int dimensions, float* out) const;

  void Directions(Motivator::Index index, int dimensions, float* out) const;

  void TargetValues(Motivator::Index index, int dimensions, float* out) const;

  void TargetVelocities(Motivator::Index index, int dimensions,
                        float* out) const;

  void Differences(Motivator::Index index, int dimensions, float* out) const;

  absl::Duration TimeRemaining(Motivator::Index index, int dimensions) const;

  absl::Duration SplineTime(Motivator::Index index) const;

  void Splines(Motivator::Index index, int dimensions,
               const CompactSpline** splines) const;

  void SetSplineTime(Motivator::Index index, int dimensions,
                     absl::Duration time);

  void SetSplinePlaybackRate(Motivator::Index index, int dimensions,
                             float playback_rate);

  void SetSplineRepeating(Motivator::Index index, int dimensions, bool repeat);

  bool Settled(Motivator::Index index, int dimensions, float max_difference,
               float max_velocity) const;

 protected:
  void CreateSplineToTarget(Motivator::Index index, float value, float velocity,
                            absl::Duration time);

  bool SupportsCloning() override;

  void CloneIndices(Motivator::Index dst, Motivator::Index src, int dimensions,
                    AnimationEngine* engine) override;

  void ResetIndices(Motivator::Index index, int dimensions) override;

  void MoveIndices(Motivator::Index old_index, Motivator::Index new_index,
                   int dimensions) override;

  void SetNumIndices(Motivator::Index num_indices) override;

  CompactSplinePtr AllocateSpline(CompactSplineIndex max_nodes);
  void FreeSplineForIndex(Motivator::Index index);

  // Hold index-specific data, for example a pointer to the spline allocated
  // from 'spline_pool_'.
  std::vector<CompactSplinePtr> data_;

  // Holds unused splines. When we need another local spline (because we're
  // supplied with target values but not the actual curve to get there),
  // try to recycle an old one from this pool first.
  std::vector<CompactSplinePtr> spline_pool_;

  // Perform the spline evaluation, over time. Indices in 'interpolator_'
  // are the same as the Motivator::Index values in this class.
  BulkSplineEvaluator interpolator_;
};
}  // namespace redux

REDUX_SETUP_TYPEID(redux::SplineProcessor);

#endif  // REDUX_ENGINES_ANIMATION_PROCESSOR_SPLINE_PROCESSOR_H_
