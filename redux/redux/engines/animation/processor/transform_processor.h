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

#ifndef REDUX_ENGINES_ANIMATION_PROCESSOR_TRANSFORM_PROCESSOR_H_
#define REDUX_ENGINES_ANIMATION_PROCESSOR_TRANSFORM_PROCESSOR_H_

#include "redux/engines/animation/animation_clip.h"
#include "redux/engines/animation/common.h"
#include "redux/engines/animation/motivator/spline_motivator.h"
#include "redux/engines/animation/motivator/transform_motivator.h"
#include "redux/engines/animation/processor/anim_processor.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/math/transform.h"

namespace redux {

class TransformProcessor : public AnimProcessor {
 public:
  explicit TransformProcessor(AnimationEngine* engine)
      : AnimProcessor(engine) {}

  TransformMotivator AllocateMotivator(int dimensions = 1);

  int Priority() const override { return 2; }

  void AdvanceFrame(absl::Duration delta_time) override;

  const Transform& Value(Motivator::Index index) const;

  void BlendTo(Motivator::Index index,
               const std::vector<AnimationChannel>& anim,
               const AnimationPlayback& playback);

  void SetPlaybackRate(Motivator::Index index, float playback_rate);

  void SetRepeating(Motivator::Index index, bool repeat);

  absl::Duration TimeRemaining(Motivator::Index index) const;

 private:
  class TransformOp {
   public:
    explicit TransformOp(AnimChannelType type);

    TransformOp(const TransformOp&) = delete;
    TransformOp& operator=(const TransformOp&) = delete;

    TransformOp(TransformOp&&) = default;
    TransformOp& operator=(TransformOp&&) = default;

    void CloneFrom(const TransformOp& rhs);

    void BlendToValue(float value, const AnimationPlayback& playback,
                      AnimationEngine* engine);
    void BlendToSpline(const CompactSpline* spline,
                       const AnimationPlayback& playback,
                       AnimationEngine* engine);

    void NegateIfQuaternionOp();  // useful for blending quaternions

    void SetRepeating(bool repeat);
    void SetPlaybackRate(float playback_rate);

    bool IsSettled(float value) const;
    AnimChannelType Type() const { return type_; }

    float Value() const;
    float Velocity() const;
    absl::Duration TimeRemaining() const;

   private:
    AnimChannelType type_ = AnimChannelType::Invalid;
    float const_value_ = 0.f;
    SplineMotivator motivator_;
  };

  struct TransformData {
    TransformData() = default;
    TransformData(const TransformData&) = delete;
    TransformData& operator=(const TransformData&) = delete;
    TransformData(TransformData&&) = default;
    TransformData& operator=(TransformData&&) = default;

    Transform transform;
    std::vector<TransformOp> ops;
  };

  // Ensures that the current quaternion values are close to the initial values
  // in `ops`. This function should be called prior to blending to `ops` to
  // ensure quaternion blends work.
  void AlignQuaternionOps(TransformData& data,
                          const std::vector<AnimationChannel>& anim);

  bool SupportsCloning() override { return true; }

  Motivator::Index NumIndices() const;
  void SetNumIndices(Motivator::Index num_indices) override;
  void CloneIndices(Motivator::Index dest, Motivator::Index src, int dimensions,
                    AnimationEngine* engine) override;
  void ResetIndices(Motivator::Index index, int dimensions) override;
  void MoveIndices(Motivator::Index old_index, Motivator::Index new_index,
                   int dimensions) override;

  TransformData& Data(Motivator::Index index);
  const TransformData& Data(Motivator::Index index) const;

  std::vector<TransformData> data_;
};
}  // namespace redux

REDUX_SETUP_TYPEID(redux::TransformProcessor);

#endif  // REDUX_ENGINES_ANIMATION_PROCESSOR_TRANSFORM_PROCESSOR_H_
