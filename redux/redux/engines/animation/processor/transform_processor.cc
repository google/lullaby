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

#include "redux/engines/animation/processor/transform_processor.h"

#include "absl/time/time.h"
#include "redux/engines/animation/animation_clip.h"
#include "redux/engines/animation/animation_engine.h"
#include "redux/engines/animation/common.h"
#include "redux/engines/animation/processor/spline_processor.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/math/constants.h"

namespace redux {

static inline float StartValue(const AnimationChannel& channel) {
  if (channel.spline) {
    return channel.spline->StartY();
  } else if (channel.const_value) {
    return *channel.const_value;
  } else {
    return ChannelDefaultValue(channel.type);
  }
}

static void ApplyOp(AnimChannelType op, float value, Transform* transform) {
  switch (op) {
    case AnimChannelType::TranslateX:
      transform->translation.x = value;
      break;
    case AnimChannelType::TranslateY:
      transform->translation.y = value;
      break;
    case AnimChannelType::TranslateZ:
      transform->translation.z = value;
      break;
    case AnimChannelType::ScaleX:
      transform->scale.x = value;
      break;
    case AnimChannelType::ScaleY:
      transform->scale.y = value;
      break;
    case AnimChannelType::ScaleZ:
      transform->scale.z = value;
      break;
    case AnimChannelType::QuaternionX:
      transform->rotation.x = value;
      break;
    case AnimChannelType::QuaternionY:
      transform->rotation.y = value;
      break;
    case AnimChannelType::QuaternionZ:
      transform->rotation.z = value;
      break;
    case AnimChannelType::QuaternionW:
      transform->rotation.w = value;
      break;
    default:
      // All other operations are not supported.
      CHECK(false);
  }
}

TransformMotivator TransformProcessor::AllocateMotivator(int dimensions) {
  TransformMotivator motivator;
  AllocateMotivatorIndices(&motivator, dimensions);
  return motivator;
}

void TransformProcessor::AdvanceFrame(absl::Duration delta_time) {
  Defragment();
  for (TransformData& data : data_) {
    data.transform = Transform();
    for (const auto& op : data.ops) {
      ApplyOp(op.Type(), op.Value(), &data.transform);
    }
    // Values may be interpolated so normalize the rotation just in case.
    data.transform.rotation.SetNormalized();
  }
  // TODO: Once we've reached the end of a non-loooping animation and the
  // values are constant, we should consider switching out the splines to
  // constants to improve performance.
}

const Transform& TransformProcessor::Value(Motivator::Index index) const {
  return Data(index).transform;
}

void TransformProcessor::BlendTo(Motivator::Index index,
                                 const std::vector<AnimationChannel>& anim,
                                 const AnimationPlayback& playback) {
  TransformData& data = Data(index);

  // Since q and -q represent the same orientation, the current quaternion
  // values may need to be negated to ensure the blend doesn't wildly change
  // individual component values.
  AlignQuaternionOps(data, anim);

  // Ops are always stored in order of ascending IDs. Scan through the old
  // and new ops trying to match IDs.
  size_t old_idx = 0;
  size_t new_idx = 0;
  const size_t num_new_ops = anim.size();

  // Initialize the transform to the start value of the new animation.
  if (data.ops.empty()) {
    for (const auto& op : anim) {
      ApplyOp(op.type, StartValue(op), &data.transform);
    }
    data.transform.rotation.SetNormalized();
  }
  data.ops.reserve(num_new_ops);

  while (old_idx < data.ops.size() && new_idx < num_new_ops) {
    auto& old_op = data.ops[old_idx];
    const auto& new_op = anim[new_idx];

    // Ops are blendable if they have identical IDs. If not, handle whichever
    // has the lower ID since it cannot possibly have a Blendable op in the
    // other list.
    if (old_op.Type() == new_op.type) {
      if (new_op.spline) {
        old_op.BlendToSpline(new_op.spline.get(), playback, Engine());
      } else {
        old_op.BlendToValue(StartValue(new_op), playback, Engine());
      }
      ++old_idx;
      ++new_idx;
    } else if (old_op.Type() < new_op.type) {
      // There is no target op, so blend to the default value. If we're already
      // at the default value, then we can remove the op entirely.
      const float default_value = ChannelDefaultValue(new_op.type);
      if (old_op.IsSettled(default_value)) {
        data.ops.erase(data.ops.begin() + old_idx);
      } else {
        old_op.BlendToValue(default_value, playback, Engine());
        ++old_idx;
      }
    } else {
      // New ops are inserted in order. `old_idx` points to the old op with
      // the next highest ID compared to `new_op`, meaning it also provides
      // the correct insertion point.
      auto iter = data.ops.begin() + old_idx;
      iter = data.ops.emplace(iter, new_op.type);
      if (new_op.spline) {
        iter->BlendToSpline(new_op.spline.get(), playback, Engine());
      } else {
        iter->BlendToValue(StartValue(new_op), playback, Engine());
      }
      ++new_idx;
      // Advance `old_idx` so it still points to the same `old_op` now that
      // one has been inserted before it.
      ++old_idx;
    }
  }

  // Fill out remaining old ops by blending them to default.
  int num_ops = data.ops.size();
  while (old_idx < num_ops) {
    TransformOp& op = data.ops[old_idx++];
    const float default_value = ChannelDefaultValue(op.Type());
    op.BlendToValue(default_value, playback, Engine());
  }

  // Fill out remaining new ops by inserting them.
  while (new_idx < num_new_ops) {
    const auto& new_op = anim[new_idx++];
    TransformOp& op = data.ops.emplace_back(new_op.type);
    if (new_op.spline) {
      op.BlendToSpline(new_op.spline.get(), playback, Engine());
    } else {
      op.BlendToValue(StartValue(new_op), playback, Engine());
    }
  }
}

void TransformProcessor::AlignQuaternionOps(
    TransformData& data, const std::vector<AnimationChannel>& anim) {
  // Extract the first quaternion from the target animation.
  quat target = quat::Identity();
  for (const auto& op : anim) {
    if (op.type == AnimChannelType::QuaternionW) {
      target.w = StartValue(op);
    } else if (op.type == AnimChannelType::QuaternionX) {
      target.x = StartValue(op);
    } else if (op.type == AnimChannelType::QuaternionY) {
      target.y = StartValue(op);
    } else if (op.type == AnimChannelType::QuaternionZ) {
      target.z = StartValue(op);
    }
  }
  target.SetNormalized();

  // Since q and -q represent the same orientation, we can negate the current
  // quaternion operations if it will make them closer to `next`.
  if (data.transform.rotation.Dot(target) < 0.f) {
    for (auto& op : data.ops) {
      op.NegateIfQuaternionOp();
    }
  }
}

void TransformProcessor::SetPlaybackRate(Motivator::Index index,
                                         float playback_rate) {
  TransformData& data = Data(index);
  for (auto& op : data.ops) {
    op.SetPlaybackRate(playback_rate);
  }
}

void TransformProcessor::SetRepeating(Motivator::Index index, bool repeat) {
  TransformData& data = Data(index);
  for (auto& op : data.ops) {
    op.SetRepeating(repeat);
  }
}

absl::Duration TransformProcessor::TimeRemaining(Motivator::Index index) const {
  absl::Duration time = absl::ZeroDuration();
  const TransformData& data = Data(index);
  for (const auto& op : data.ops) {
    time = std::max(time, op.TimeRemaining());
  }
  return time;
}

Motivator::Index TransformProcessor::NumIndices() const {
  return static_cast<Motivator::Index>(data_.size());
}

void TransformProcessor::CloneIndices(Motivator::Index dest,
                                      Motivator::Index src, int dimensions,
                                      AnimationEngine* engine) {
  for (Motivator::Index i = 0; i < dimensions; ++i) {
    const TransformData& data_src = Data(src + i);
    TransformData& data_dst = Data(dest + i);
    data_dst.transform = data_src.transform;
    data_dst.ops.clear();
    data_dst.ops.reserve(data_src.ops.size());
    for (const auto& op : data_src.ops) {
      data_dst.ops.emplace_back(op.Type()).CloneFrom(op);
    }
  }
}

void TransformProcessor::ResetIndices(Motivator::Index index, int dimensions) {
  // Callers depend on indices staying consistent between calls to this
  // function, so just reset the TransformData states to empty instead
  // of erasing them.
  for (Motivator::Index i = index; i < index + dimensions; ++i) {
    TransformData& data = data_[i];
    data.transform = Transform();
    data.ops.clear();
  }
}

void TransformProcessor::MoveIndices(Motivator::Index old_index,
                                     Motivator::Index new_index,
                                     int dimensions) {
  for (int i = 0; i < dimensions; ++i) {
    using std::swap;
    swap(data_[new_index + i], data_[old_index + i]);
    data_[old_index + i].transform = Transform();
    data_[old_index + i].ops.clear();
  }
}

void TransformProcessor::SetNumIndices(Motivator::Index num_indices) {
  data_.resize(num_indices);
}

TransformProcessor::TransformData& TransformProcessor::Data(
    Motivator::Index index) {
  CHECK(ValidIndex(index));
  return data_[index];
}

const TransformProcessor::TransformData& TransformProcessor::Data(
    Motivator::Index index) const {
  CHECK(ValidIndex(index));
  return data_[index];
}

TransformProcessor::TransformOp::TransformOp(AnimChannelType type)
    : type_(type) {
  const_value_ = ChannelDefaultValue(type);
}

void TransformProcessor::TransformOp::SetRepeating(bool repeat) {
  if (motivator_.Valid()) {
    motivator_.SetRepeating(repeat);
  }
}

void TransformProcessor::TransformOp::SetPlaybackRate(float playback_rate) {
  if (motivator_.Valid()) {
    motivator_.SetPlaybackRate(playback_rate);
  }
}

float TransformProcessor::TransformOp::Value() const {
  return motivator_.Valid() ? motivator_.Value() : const_value_;
}

float TransformProcessor::TransformOp::Velocity() const {
  return motivator_.Valid() ? motivator_.Velocity() : 0.f;
}

absl::Duration TransformProcessor::TransformOp::TimeRemaining() const {
  return motivator_.Valid() ? motivator_.TimeRemaining() : absl::ZeroDuration();
}

void TransformProcessor::TransformOp::CloneFrom(const TransformOp& rhs) {
  type_ = rhs.type_;
  const_value_ = rhs.const_value_;
  motivator_.CloneFrom(&rhs.motivator_);
}

bool TransformProcessor::TransformOp::IsSettled(float value) const {
  if (motivator_.Valid()) {
    const float diff = std::fabs(motivator_.Value() - value);
    const float velocity = std::fabs(motivator_.Velocity());
    return diff < kDefaultEpsilon && velocity < kDefaultEpsilon;
  } else {
    return const_value_ == value;
  }
}

void TransformProcessor::TransformOp::BlendToValue(
    float value, const AnimationPlayback& playback, AnimationEngine* engine) {
  if (!motivator_.Valid()) {
    // This channel did not exist previously, so snap it to the new constant
    // Blending nothing to constant happens immediately.
    const float default_value = ChannelDefaultValue(type_);
    motivator_ = engine->AcquireMotivator<SplineMotivator>();
    motivator_.SetTarget(default_value, 0.f, absl::ZeroDuration());
    motivator_.SetTarget(value, 0.f, playback.blend_time);
  } else if (AreNearlyEqual(motivator_.Value(), value) &&
             AreNearlyEqual(motivator_.Velocity(), 0.f)) {
    // Blending a spline already at the desired constant and with 0 velocity
    // transforms this op into a constant immediately.
    motivator_.Invalidate();
    const_value_ = value;
  } else {
    motivator_.SetTarget(value, 0.f, playback.blend_time);
  }
}

void TransformProcessor::TransformOp::BlendToSpline(
    const CompactSpline* spline, const AnimationPlayback& playback,
    AnimationEngine* engine) {
  if (!motivator_.Valid()) {
    motivator_ = engine->AcquireMotivator<SplineMotivator>();
    motivator_.SetTarget(const_value_, absl::ZeroDuration());
  }
  DCHECK(spline);
  motivator_.SetSpline(*spline, playback);
}

void TransformProcessor::TransformOp::NegateIfQuaternionOp() {
  if (type_ < AnimChannelType::QuaternionX) {
    return;
  } else if (type_ > AnimChannelType::QuaternionW) {
    return;
  }
  if (motivator_.Valid()) {
    motivator_.SetTarget(-motivator_.Value(), -motivator_.Velocity(),
                         absl::ZeroDuration());
  } else {
    const_value_ = -const_value_;
  }
}
}  // namespace redux
