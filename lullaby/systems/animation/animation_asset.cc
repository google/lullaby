/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#include "lullaby/systems/animation/animation_asset.h"

#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/make_unique.h"
#include "motive/anim_list_generated.h"
#include "motive/io/flatbuffers.h"

namespace lull {
namespace {
// .motivelist files contain a single list of anims, but we read them into an
// AnimTable, which is a list of lists of anims.  So we always just read the
// 0th list.
const int kAnimListIndex = 0;
}  // namespace

AnimationAsset::AnimationAsset()
    : Asset(), rig_anim_(nullptr), anim_table_(nullptr), num_splines_(0) {}

void AnimationAsset::OnFinalize(const std::string& filename,
                                std::string* data) {
  if (filename.find(".motiveanim") != std::string::npos) {
    rig_anim_ = MakeUnique<motive::RigAnim>();
    const motive::RigAnimFb* src = motive::GetRigAnimFb(data->data());
    if (src) {
      motive::RigAnimFromFlatBuffers(*src, rig_anim_.get());
    }
  } else if (filename.find(".motivelist") != std::string::npos) {
    anim_table_ = MakeUnique<motive::AnimTable>();
    const motive::AnimListFb* src = motive::GetAnimListFb(data->data());
    if (src) {
      if (!anim_table_->InitFromFlatBuffers(*src, nullptr /* load_fn */)) {
        LOG(ERROR) << "Failed to load anim table";
      }
    }
  } else {
    const motive::CompactSplineAnimFloatFb* src =
        motive::GetCompactSplineAnimFloatFb(data->data());
    if (src) {
      if (!GetSplinesFromFlatBuffers(src)) {
        LOG(DFATAL) << "Error processing file" << filename;
      }
    }
  }
}

bool AnimationAsset::GetSplinesFromFlatBuffers(
    const motive::CompactSplineAnimFloatFb* src) {
  if (src->splines() == nullptr) {
    LOG(ERROR) << "No splines in file.";
    return false;
  }
  num_splines_ = src->splines()->size();
  if (num_splines_ == 0) {
    return false;
  }

  // Get the number of splines in the animation and the size of the buffer
  // needed to store the CompactSpline array.
  int buffer_size = 0;
  for (int i = 0; i < num_splines_; ++i) {
    const motive::CompactSplineFloatFb* spline = src->splines()->Get(i);
    const uint16_t num_nodes = static_cast<uint16_t>(spline->nodes()->size());
    buffer_size += static_cast<int>(motive::CompactSpline::Size(num_nodes));
  }
  if (buffer_size == 0) {
    return false;
  }

  buffer_size *= 2;  // Double the buffer size to allow for spline smoothing.
  spline_buffer_.resize(buffer_size);

  // Convert the CompactSplineAnimFloatFb* array into a CompactSpline array.
  uint8_t* iter = spline_buffer_.data();
  for (int i = 0; i < num_splines_; ++i) {
    const motive::CompactSplineFloatFb* spline = src->splines()->Get(i);
    const motive::CompactSpline* cs = CreateCompactSpline(spline, iter);
    if (cs) {
      iter += cs->Size();
      buffer_size -= static_cast<int>(cs->Size());
    }
  }
  if (buffer_size < 0) {
    LOG(ERROR) << "Failed spline smoothing";
    return false;
  }
  return true;
}

motive::CompactSpline* AnimationAsset::CreateCompactSpline(
    const motive::CompactSplineFloatFb* src, uint8_t* buffer) {
  const unsigned int num_nodes = src->nodes()->size();
  if (num_nodes == 0) {
    return nullptr;
  }

  const float total_time = src->nodes()->Get(num_nodes - 1)->time();
  const motive::Range range(src->min_value(), src->max_value());
  const float motive_total_time =
      static_cast<float>(AnimationSystem::GetMotiveTimeFromSeconds(total_time));

  // The maximum number of nodes is twice the data count in case the spline
  // is smoothed.
  const unsigned int max_nodes = num_nodes * 2;
  motive::CompactSpline* spline = motive::CompactSpline::CreateInPlace(
      static_cast<uint16_t>(max_nodes), buffer);
  const float granularity =
      motive::CompactSpline::RecommendXGranularity(motive_total_time);
  spline->Init(range, granularity);

  for (unsigned int i = 0; i < num_nodes; ++i) {
    const motive::CompactSplineFloatNodeFb* node = src->nodes()->Get(i);
    const float x = static_cast<float>(
        AnimationSystem::GetMotiveTimeFromSeconds(node->time()));
    const float y = node->value();
    const float derivative =
        AnimationSystem::GetMotiveDerivativeFromSeconds(node->derivative());
    spline->AddNode(x, y, derivative);
  }
  spline->Finalize();
  return spline;
}

const motive::CompactSpline* AnimationAsset::GetCompactSpline(int idx) const {
  if (spline_buffer_.empty()) {
    return nullptr;
  }
  const motive::CompactSpline* splines =
      reinterpret_cast<const motive::CompactSpline*>(spline_buffer_.data());
  return splines->NextAtIdx(idx);
}

void AnimationAsset::GetSplinesAndConstants(
    int rig_anim_index, int dimensions, const motive::MatrixOperationType* ops,
    const motive::CompactSpline** splines, float* constants) const {
  const motive::RigAnim* rig_anim = GetRigAnim(rig_anim_index);
  if (rig_anim != nullptr && ops != nullptr) {
    const motive::BoneIndex root_bone_index = 0;
    // Pull splines and constant values from the RigAnim for the root bone.
    rig_anim->GetSplinesAndConstants(root_bone_index, ops, dimensions, splines,
                                      constants);
  } else {
    for (int k = 0; k < dimensions; ++k) {
      // Use the splines in the order that they are listed.
      splines[k] = k < num_splines_ ? GetCompactSpline(k) : nullptr;

      // Constant values are just the defaults for the operations.
      constants[k] = ops ? motive::OperationDefaultValue(ops[k]) : 0.f;
    }
  }
}

int AnimationAsset::GetNumCompactSplines() const { return num_splines_; }

int AnimationAsset::GetNumRigAnims() const {
  if (rig_anim_) {
    return 1;
  }
  if (anim_table_) {
    return anim_table_->NumAnims(kAnimListIndex);
  }
  return 0;
}

const motive::RigAnim* AnimationAsset::GetRigAnim(int index) const {
  if (rig_anim_) {
    return rig_anim_.get();
  }
  if (anim_table_) {
    return anim_table_->Query(kAnimListIndex, index);
  }
  return nullptr;
}

}  // namespace lull
