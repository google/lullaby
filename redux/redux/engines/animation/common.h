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

#ifndef REDUX_ENGINES_ANIMATION_COMMON_H_
#define REDUX_ENGINES_ANIMATION_COMMON_H_

#include <cstddef>
#include <limits>

#include "redux/data/asset_defs/anim_asset_def_generated.h"

namespace redux {

// Identify bone for skeletal animation. Each non-root bone has a parent whose
// BoneIndex is less than its own. Each bone has a transformation matrix. By
// traversing up the tree to a root bone, multiplying the transformation
// matrices as you go, you can get the global transform for the bone.
using BoneIndex = std::uint16_t;
static const BoneIndex kMaxNumBones = std::numeric_limits<BoneIndex>::max() - 1;
static const BoneIndex kInvalidBoneIdx = std::numeric_limits<BoneIndex>::max();

inline bool IsTranslationChannel(AnimChannelType type) {
  return type == AnimChannelType::TranslateX ||
         type == AnimChannelType::TranslateY ||
         type == AnimChannelType::TranslateZ;
}

inline bool IsQuaternionChannel(AnimChannelType type) {
  return type == AnimChannelType::QuaternionX ||
         type == AnimChannelType::QuaternionY ||
         type == AnimChannelType::QuaternionZ ||
         type == AnimChannelType::QuaternionW;
}

inline bool IsScaleChannel(AnimChannelType type) {
  return type == AnimChannelType::ScaleX || type == AnimChannelType::ScaleY ||
         type == AnimChannelType::ScaleZ;
}

inline float ChannelDefaultValue(AnimChannelType type) {
  return IsScaleChannel(type) || (type == AnimChannelType::QuaternionW) ? 1.f
                                                                        : 0.f;
}
}  // namespace redux

#endif  // REDUX_ENGINES_ANIMATION_COMMON_H_
