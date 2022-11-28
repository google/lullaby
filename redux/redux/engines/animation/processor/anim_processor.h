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

#ifndef REDUX_ENGINES_ANIMATION_PROCESSOR_ANIM_PROCESSOR_H_
#define REDUX_ENGINES_ANIMATION_PROCESSOR_ANIM_PROCESSOR_H_

#include <vector>

#include "redux/engines/animation/common.h"
#include "redux/engines/animation/motivator/motivator.h"
#include "redux/engines/animation/processor/index_allocator.h"
#include "redux/modules/base/typeid.h"

namespace redux {

class AnimationEngine;

// An AnimProcessor processes *all* instances of one type of Motivator.
//
// Each derivation of AnimProcessor is one animation algorithm. It holds
// all the data for all Motivators that are currently using that animation
// algorithm.
//
// We pool the processing for potential optimization opportunities. We may have
// hundreds of smoothly-interpolating one-dimensional Motivators, for example.
// It's nice to be able to update those 4 or 8 or 16 at a time using SIMD.
// And it's nice to have the data gathered in one spot if we want to use
// multiple threads.
//
// AnimProcessors exists in the internal API. For the external API, please
// see Motivator.
class AnimProcessor {
 public:
  virtual ~AnimProcessor();

  // Initializes `dst` to be a clone of the Motivator referenced by `src`.
  void CloneMotivator(Motivator* dst, Motivator::Index src);

  // Removes a motivator and return its index to the pile of allocatable
  // indices.
  //
  // This function should only be called by Motivator::Invalidate().
  void RemoveMotivator(Motivator::Index index);

  // Transfers ownership of the motivator at `index` to 'new_motivator'.
  // Resets the Motivator that currently owns `index` and initializes
  // 'new_motivator'.
  //
  // This function should only be called by Motivator's move operations.
  void TransferMotivator(Motivator::Index index, Motivator* new_motivator);

  // Returns true if `index` is currently driving a motivator. Does not do any
  // validity checking, however, like ValidMotivatorIndex() does.
  bool IsMotivatorIndex(Motivator::Index index) const;

  // Returns true if `index` is currently in a block of indices driven by
  // a motivator.
  bool ValidIndex(Motivator::Index index) const;

  // Returns true if a Motivator is referencing this index. That is, if this
  // index is part of a block of indices (for example a block of 3 indices
  // referenced by a Motivator3f), then this index is the *first* index in that
  // block.
  bool ValidMotivatorIndex(Motivator::Index index) const;

  // Returns true if `index` is currently driving `motivator`.
  bool ValidMotivator(Motivator::Index index,
                      const Motivator* motivator) const {
    return ValidIndex(index) && motivators_[index] == motivator;
  }

  // Advance the simulation by `delta_time`.
  //
  // This function should only be called by AnimationEngine::AdvanceFrame.
  virtual void AdvanceFrame(absl::Duration delta_time) = 0;

  // The number of slots occupied in the AnimProcessor. For example,
  // a position in 3D space would return 3. A single 4x4 matrix would return 1.
  int Dimensions(Motivator::Index index) const {
    return index_allocator_.CountForIndex(index);
  }

  // The lower the number, the sooner the AnimProcessor gets updated.
  // Should never change. We want a static ordering of processors.
  // Some AnimProcessors use the output of other AnimProcessors, so
  // we impose a strict ordering here.
  virtual int Priority() const { return 0; }

  // Ensure that the internal state is consistent. Call periodically when
  // debugging problems where the internal state is corrupt.
  void VerifyInternalState() const;

 protected:
  AnimProcessor() = delete;
  explicit AnimProcessor(AnimationEngine* engine)
      : index_allocator_(allocator_callbacks_), engine_(engine) {
    allocator_callbacks_.set_processor(this);
  }

  // Allocates an index for `motivator` and initialize it to that index. Returns
  // the newly allocated index.
  Motivator::Index AllocateMotivatorIndices(Motivator* motivator,
                                            int dimensions);

  // Indicates whether or not this Processor supports cloning.
  // If overridden to return true, the Processor should also override the
  // CloneIndices() signature below to implement the actual duplication.
  virtual bool SupportsCloning() { return false; }

  // Initializes data at [dst, dst + dimensions) to a clone of the data at
  // [src, src + dimensions). Processors that support initializing new
  // Motivators from existing Motivators should override this function.
  virtual void CloneIndices(Motivator::Index dst, Motivator::Index src,
                            int dimensions, AnimationEngine* engine) {
    // Hitting this assertion means SupportsCloning() returned true but the
    // Processor didn't override this function.
    assert(false);
  }

  // Resets data at [index, index + dimensions).
  // See comment above InitializeIndex for meaning of `index`.
  // If your AnimProcessor stores data in a plain array, you probably have
  // nothing to do. But if you use dynamic memory per index,
  // (which you really shouldn't - too slow!), you should deallocate it here.
  // For debugging, it might be nice to invalidate the data.
  virtual void ResetIndices(Motivator::Index index, int dimensions) = 0;

  // Moves the data chunk of length `dimensions` from `old_index` into
  // `new_index`. Used by Defragment().
  // Note that the index range starting at `new_index` is guaranteed to be
  // inactive.
  virtual void MoveIndices(Motivator::Index old_index,
                           Motivator::Index new_index, int dimensions) = 0;

  // Increases or decreases the total number of indices.
  // If decreased, existing indices >= num_indices should be uninitialized.
  // If increased, internal arrays should be extended to new_indices, and
  // new items in the arrays should be initialized as reset.
  virtual void SetNumIndices(Motivator::Index num_indices) = 0;

  // When an index is moved, the Motivator that references that index is
  // updated. Can be called at the discretion of your AnimProcessor,
  // but normally called at the beginning of your
  // AnimProcessor::AdvanceFrame.
  void Defragment() { index_allocator_.Defragment(); }

  // Returns a handle to the AnimationEngine instance that owns this processor.
  AnimationEngine* Engine() { return engine_; }
  const AnimationEngine* Engine() const { return engine_; }

 private:
  using MotivatorIndexAllocator = IndexAllocator<Motivator::Index>;
  using IndexRange = MotivatorIndexAllocator::IndexRange;

  // Don't notify derived class.
  void RemoveMotivatorWithoutNotifying(Motivator::Index index);

  // Handle callbacks from IndexAllocator.
  void MoveIndexRangeBase(const IndexRange& source, Motivator::Index target);
  void SetNumIndicesBase(Motivator::Index num_indices);

  // Proxy callbacks from IndexAllocator into AnimProcessor.
  class AllocatorCallbacks : public MotivatorIndexAllocator::CallbackInterface {
   public:
    AllocatorCallbacks() : processor_(nullptr) {}
    void set_processor(AnimProcessor* processor) { processor_ = processor; }
    void SetNumIndices(Motivator::Index num_indices) override {
      processor_->SetNumIndicesBase(num_indices);
    }
    void MoveIndexRange(const IndexRange& source,
                        Motivator::Index target) override {
      processor_->MoveIndexRangeBase(source, target);
    }

   private:
    AnimProcessor* processor_;
  };

  // Back-pointer to the Motivators for each index. The Motivators reference
  // this AnimProcessor and a specific index into the AnimProcessor,
  // so when the index is moved, or when the AnimProcessor itself is
  // destroyed, we need to update the Motivator.
  // Note that we only keep a reference to a single Motivator per index.
  // When a Motivator is moved, the old Motivator is Reset and the reference
  // here is updated.
  std::vector<Motivator*> motivators_;

  // Proxy calbacks into AnimProcessor. The other option is to derive
  // AnimProcessor from IndexAllocator::CallbackInterface, but that would
  // create a messier API, and not be great OOP.
  // This member should be initialized before index_allocator_ is initialized.
  AllocatorCallbacks allocator_callbacks_;

  // When an index is freed, we keep track of it here. When an index is
  // allocated, we use one off this array, if one exists.
  // When Defragment() is called, we empty this array by filling all the
  // unused indices with the highest allocated indices. This reduces the total
  // size of the data arrays.
  MotivatorIndexAllocator index_allocator_;

  // A handle to the owning AnimationEngine. This is required when new
  // Motivators are created outside of typical initialization times.
  AnimationEngine* engine_;
};
}  // namespace redux

#endif  // REDUX_ENGINES_ANIMATION_PROCESSOR_ANIM_PROCESSOR_H_
