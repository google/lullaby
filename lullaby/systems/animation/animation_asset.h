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

#ifndef LULLABY_SYSTEMS_ANIMATION_ANIMATION_ASSET_H_
#define LULLABY_SYSTEMS_ANIMATION_ANIMATION_ASSET_H_

#include "motive/anim_generated.h"
#include "motive/anim.h"
#include "motive/anim_table.h"
#include "motive/math/compact_spline.h"
#include "motive/spline_anim_generated.h"
#include "lullaby/modules/file/asset.h"

namespace lull {

// Asset containing animation data loaded using the AssetLoader.
//
// The raw data is converted into runtime motive::CompactSplines,
// motive::RigAnim, or motive::AnimTable for use by the AnimationSystem.
class AnimationAsset : public Asset {
 public:
  explicit AnimationAsset();

  // Converts and stores |data| as either a RigAnim or a vector of
  // CompactSplines.
  void OnFinalize(const std::string& filename, std::string* data) override;

  // Returns the number of CompactSplines in the data.
  int GetNumCompactSplines() const;

  // Returns the number of RigAnims in the data.
  int GetNumRigAnims() const;

  // Gets the Nth CompactSpline, or NULL if either the index is out of bounds or
  // the data is a RigAnim or AnimTable.
  const motive::CompactSpline* GetCompactSpline(int idx) const;

  // Gets the RigAnim data at |idx|, or NULL if the data is a vector of
  // CompactSplines.
  const motive::RigAnim* GetRigAnim(int idx) const;

  // Gets the values that are used to drive an animation.
  // |ops| is an optional array of length |dimensions| that specifies the
  // purpose of data in each dimension.  It is used to extract the appropriate
  // spline from a RigAnim and to set default values for the constants.
  // |splines| and |constants| are output  arrays of length |dimensions|.
  // If the n-th value of |splines| is non-null, then the n-th dimension is
  // driven by that spline. Otherwise, the n-th dimension should use the
  // value at the n-th index of |constants|.  If the data is not RigAnims,
  // |rig_anim_index| is ignored.
  void GetSplinesAndConstants(int rig_anim_index, int dimensions,
                              const motive::MatrixOperationType* ops,
                              const motive::CompactSpline** splines,
                              float* constants) const;

 private:
  // Extracts spline data from a flatbuffer into spline_buffer_.
  bool GetSplinesFromFlatBuffers(const motive::CompactSplineAnimFloatFb* src);

  // Emplaces a motive::CompactSpline into |buffer| using the data in |src|.
  motive::CompactSpline* CreateCompactSpline(
      const motive::CompactSplineFloatFb* src, uint8_t* buffer);

  std::unique_ptr<motive::RigAnim> rig_anim_;  // Rig animation data.
  std::unique_ptr<motive::AnimTable> anim_table_;  // Anim table data.
  std::vector<uint8_t> spline_buffer_;         // Buffer containing spline data.
  int num_splines_;  // Number of compact splines in the buffer.
};

typedef std::shared_ptr<AnimationAsset> AnimationAssetPtr;

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::AnimationAsset);

#endif  // LULLABY_SYSTEMS_ANIMATION_ANIMATION_ASSET_H_
