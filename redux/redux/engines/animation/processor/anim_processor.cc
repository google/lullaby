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

#include "redux/engines/animation/processor/anim_processor.h"

#include "redux/engines/animation/motivator/motivator.h"

namespace redux {

AnimProcessor::~AnimProcessor() {
  // Reset all of the Motivators that we're currently driving.
  // We don't want any of them to reference us after we've been destroyed.
  for (Motivator::Index index = 0; index < index_allocator_.num_indices();
       index += Dimensions(index)) {
    if (motivators_[index] != nullptr) {
      RemoveMotivatorWithoutNotifying(index);
    }
  }

  // Sanity-check: Ensure that we have no more active Motivators.
  assert(index_allocator_.Empty());
}

void AnimProcessor::VerifyInternalState() const {
#if REDUX_VERIFY_INTERNAL_PROCESSOR_STATE
  // Check the validity of the IndexAllocator.
  index_allocator_.VerifyInternalState();

  // Check the validity of each Motivator.
  Motivator::Index len = static_cast<Motivator::Index>(motivators_.size());
  for (Motivator::Index i = 0; i < len; i += Dimensions(i)) {
    // If a Motivator is nullptr, its index should not be allocated.
    assert((motivators_[i] == nullptr && !index_allocator_.ValidIndex(i)) ||
           motivators_[i]->Valid());

    if (motivators_[i] == nullptr) continue;

    // All back pointers for a motivator should be the same.
    const int dimensions = Dimensions(i);
    for (Motivator::Index j = i + 1; j < i + dimensions; ++j) {
      assert(motivators_[i] == motivators_[j]);
    }

    // A Motivator should be referenced once.
    for (Motivator::Index j = i + dimensions; j < len; j += Dimensions(j)) {
      assert(motivators_[i] != motivators_[j]);
    }
  }
#endif  // REDUX_VERIFY_INTERNAL_PROCESSOR_STATE
}

void AnimProcessor::CloneMotivator(Motivator* dst, Motivator::Index src) {
  // Early out if the Processor doesn't support duplication to avoid allocating
  // and destroying new indices.
  if (!SupportsCloning()) {
    return;
  }

  // Assign an 'index' to reference the new Motivator. All interactions between
  // the Motivator and AnimProcessor use this 'index' to identify the data.
  const int dimensions = Dimensions(src);
  const Motivator::Index dst_index = AllocateMotivatorIndices(dst, dimensions);

  // Call the AnimProcessor-specific cloning routine.
  CloneIndices(dst_index, src, dimensions, Engine());
}

// Don't notify derived classes. Useful in the destructor, since derived classes
// have already been destroyed.
void AnimProcessor::RemoveMotivatorWithoutNotifying(Motivator::Index index) {
  // Ensure the Motivator no longer references us.
  motivators_[index]->Reset();

  // Ensure we no longer reference the Motivator.
  const int dimensions = Dimensions(index);
  for (int i = 0; i < dimensions; ++i) {
    motivators_[index + i] = nullptr;
  }

  // Recycle 'index'. It will be used in the next allocation, or back-filled in
  // the next call to Defragment().
  index_allocator_.Free(index);
}

void AnimProcessor::RemoveMotivator(Motivator::Index index) {
  assert(ValidMotivatorIndex(index));

  // Call the AnimProcessor-specific remove routine.
  ResetIndices(index, Dimensions(index));

  // Need this version since the destructor can't call the pure virtual
  // RemoveIndex() above.
  RemoveMotivatorWithoutNotifying(index);

  VerifyInternalState();
}

void AnimProcessor::TransferMotivator(Motivator::Index index,
                                      Motivator* new_motivator) {
  assert(ValidMotivatorIndex(index));

  // Ensure old Motivator does not reference us anymore. Only one Motivator is
  // allowed to reference 'index'.
  Motivator* old_motivator = motivators_[index];
  old_motivator->Reset();

  // Set up new_motivator to reference 'index'.
  new_motivator->Init(this, index);

  // Update our reference to the unique Motivator that references 'index'.
  const int dimensions = Dimensions(index);
  for (int i = 0; i < dimensions; ++i) {
    motivators_[index + i] = new_motivator;
  }

  VerifyInternalState();
}

bool AnimProcessor::IsMotivatorIndex(Motivator::Index index) const {
  return motivators_[index] != nullptr &&
         (index == 0 || motivators_[index - 1] != motivators_[index]);
}

bool AnimProcessor::ValidIndex(Motivator::Index index) const {
  return index < index_allocator_.num_indices() &&
         motivators_[index] != nullptr &&
         motivators_[index]->processor_ == this;
}

bool AnimProcessor::ValidMotivatorIndex(Motivator::Index index) const {
  return ValidIndex(index) && IsMotivatorIndex(index);
}

Motivator::Index AnimProcessor::AllocateMotivatorIndices(Motivator* motivator,
                                                         int dimensions) {
  // Assign an 'index' to reference the new Motivator. All interactions between
  // the Motivator and AnimProcessor use this 'index' to identify the data.
  const Motivator::Index index = index_allocator_.Alloc(dimensions);

  // Keep a pointer to the Motivator around. We may Defragment() the indices and
  // move the data around. We also need to remove the Motivator when we're
  // destroyed.
  for (int i = 0; i < dimensions; ++i) {
    motivators_[index + i] = motivator;
  }

  // Initialize the motivator to point at our AnimProcessor.
  motivator->Init(this, index);
  VerifyInternalState();
  return index;
}

void AnimProcessor::SetNumIndicesBase(Motivator::Index num_indices) {
  // When the size decreases, we don't bother reallocating the size of the
  // 'motivators_' vector. We want to avoid reallocating as much as possible,
  // so we let it grow to its high-water mark.
  //
  // TODO: Ideally, we should reserve approximately the right amount of storage
  // for motivators_. That would require adding a user-defined initialization
  // parameter.
  motivators_.resize(num_indices);

  // Call derived class.
  SetNumIndices(num_indices);
}

void AnimProcessor::MoveIndexRangeBase(const IndexRange& source,
                                       Motivator::Index target) {
  // Reinitialize the motivators to point to the new index.
  const Motivator::Index index_diff = target - source.start();
  for (Motivator::Index i = source.start(); i < source.end();
       i += Dimensions(i)) {
    motivators_[i]->Init(this, i + index_diff);
  }

  // Tell derivated class about the move.
  MoveIndices(source.start(), target, source.Length());

  // Reinitialize the motivator pointers.
  for (Motivator::Index i = source.start(); i < source.end(); ++i) {
    // Assert we're moving something valid onto something invalid.
    assert(motivators_[i] != nullptr);
    assert(motivators_[i + index_diff] == nullptr);

    // Move our internal data too.
    motivators_[i + index_diff] = motivators_[i];
    motivators_[i] = nullptr;
  }
}
}  // namespace redux
