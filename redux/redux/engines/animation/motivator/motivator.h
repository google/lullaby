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

#ifndef REDUX_ENGINES_ANIMATION_MOTIVATOR_MOTIVATOR_H_
#define REDUX_ENGINES_ANIMATION_MOTIVATOR_MOTIVATOR_H_

#include "redux/engines/animation/common.h"

namespace redux {

class AnimProcessor;

// Drives a value towards a target value or along a path.
//
// The value can be one-dimensional (e.g. a float), or multi-dimensional
// (e.g. a transform). This is defined by the sub-class of this class.
//
// The algorithm that drives a Motivator's value moves towards its target is
// determined by the AnimProcessor to which the Motivator belongs. This is
// specified in Motivator::Init().
//
// A Motivator does not store any data itself. It is an index into its owning
// AnimProcessor. Sub-classes should define an API for the Motivator that allows
// users to query the data.
//
// Only one Motivator can reference a specific index in an AnimProcessor.
class Motivator {
 public:
  using Index = int;
  static constexpr Index kInvalidIndex = static_cast<Index>(-1);

  ~Motivator();

  // Disallow copying of Motivators since only one Motivator can reference a
  // specific index in a AnimProcessor.
  Motivator(const Motivator& original) = delete;
  Motivator& operator=(const Motivator& original) = delete;

  // Transfers ownership of `rhs` motivator to `this` motivator.
  // `rhs` motivator is reset and must be initialized again before being
  // read. We want to allow moves primarily so that we can have vectors of
  // Motivators.
  Motivator(Motivator&& rhs) noexcept;
  Motivator& operator=(Motivator&& rhs) noexcept;

  // Detatches this Motivator from its AnimProcessor. Functions other than
  // Init() and Valid() should no longer be called.
  void Invalidate();

  // Returns true if this Motivator is currently being driven by an
  // AnimProcessor.
  bool Valid() const;

  // Returns the number of values that this Motivator is driving. For example, a
  // 3D position would return 3, since it drives three floats. A single 4x4
  // matrix would return 1, since it's driving one transform. This value is
  // determined by the AnimProcessor backing this motivator.
  int Dimensions() const;

  // Initializes this Motivator to the current state of another Motivator.
  //
  // This function is explicitly not a copy constructor because it has a
  // different index that references different data.
  void CloneFrom(const Motivator* other);

  // Checks the consistency of internal state. Useful for debugging.
  // If this function ever returns false, there has been some sort of memory
  // corruption or similar bug.
  bool Sane() const;

 protected:
  // The AnimProcessor uses the functions below. It does not modify data
  // directly.
  friend class AnimProcessor;

  // These should only be called by AnimProcessor!
  Motivator() = default;
  void Init(AnimProcessor* processor, Index index);
  void Reset();

  // All calls to an Motivator are proxied to an AnimProcessor. Motivator data
  // and processing is centralized to allow for scalable optimizations (e.g.
  // SIMD or parallelization).
  AnimProcessor* processor_ = nullptr;

  // This index here uniquely identifies this Motivator to the AnimProcessor.
  Index index_ = kInvalidIndex;
};

}  // namespace redux

#endif  // REDUX_ENGINES_ANIMATION_MOTIVATOR_MOTIVATOR_H_
