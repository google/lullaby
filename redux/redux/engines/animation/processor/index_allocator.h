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

#ifndef REDUX_ENGINES_ANIMATION_PROCESSOR_INDEX_ALLOCATOR_H_
#define REDUX_ENGINES_ANIMATION_PROCESSOR_INDEX_ALLOCATOR_H_

#include <algorithm>
#include <cstring>
#include <type_traits>

#include "redux/modules/base/logging.h"

namespace redux {

// Allocates and frees indices into an array. Tries to keep the array as small
// as possible by recycling indices that have been freed.
//
// Let's say we have an array of items that we would like to process with SIMD
// instructions. Items can be added and deleted from the array though. We don't
// want many unused indices in the array, since these holes still have to be
// processed with SIMD (which processes indices in groups of 4 or 8 or 16).
//
// The IndexAllocator is great for this situation since you can call
// Defragment() before running the SIMD algorithm. The Defragment() call will
// backfill unused indices and ensure the data is contiguous.
//
//   Details
//   =======
// Periodically, you can call Defragment() to backfill indices that have been
// freed with the largest indices. This minimizes the length of the array, and
// more importantly makes the array data contiguous.
//
// During Defragment() when an index is moved, a callback
// CallbackInterface::MoveIndex() is called so that the user can move the
// corresponding data.
//
// Whenever the array size is increased (durring Alloc()) or decreased (during
// Defragment()), a callback CallbackInterface::SetNumIndices() is called so
// that the user can grow or shrink the corresponding data.
template <class Index>
class IndexAllocator {
 public:
  // The number of indices to allocate. Same base type as Index, since the
  // `counts_` array can be as long as the largest index.
  typedef Index Count;

  class IndexRange {
   public:
    IndexRange() : start_(1), end_(0) {}
    IndexRange(Index start, Index end) : start_(start), end_(end) {}
    bool Valid() const { return start_ <= end_; }
    Index Length() const { return end_ - start_; }
    Index start() const { return start_; }
    Index end() const { return end_; }

   private:
    Index start_;
    Index end_;
  };

  class CallbackInterface {
   public:
    virtual ~CallbackInterface() {}
    virtual void SetNumIndices(Index num_indices) = 0;
    virtual void MoveIndexRange(const IndexRange& source, Index target) = 0;
  };

  // Create an empty IndexAllocator that uses the specified callback interface.
  explicit IndexAllocator(CallbackInterface& callbacks)
      : callbacks_(&callbacks) {
    static_assert(std::is_signed<Count>::value,
                  "Count (and therefore Index) must be a signed type");
  }

  // If a previously-freed index can be recycled, allocates that index.
  // Otherwise, increases the total number of indices by `count`, and return
  // the first new index. When the number of indices is increased,
  // the SetNumIndices() callback is called.
  // @param count The number of indices in this allocation. Each block of
  //              allocated indices is kept contiguous during Defragment()
  //              calls. The index returned is the first index in the block.
  Index Alloc(Count count) {
    // Recycle an unused index, if one exists and has the correct count.
    typename std::vector<Index>::iterator least_excess_it =
        unused_indices_.end();
    Count least_excess = std::numeric_limits<Count>::max();
    for (auto it = unused_indices_.begin(); it != unused_indices_.end(); ++it) {
      const Index unused_index = *it;
      const Count excess = CountForIndex(unused_index) - count;

      // Not big enough.
      if (excess < 0) continue;

      // Perfect size. Remove from `unused_indices_` pool.
      if (excess == 0) {
        unused_indices_.erase(it);
        return unused_index;
      }

      // Too big. We'll return the one with the least excess size.
      if (excess < least_excess) {
        least_excess = excess;
        least_excess_it = it;
      }
    }

    // The unused index has a count that's too high.
    if (least_excess_it != unused_indices_.end()) {
      // Return the first `count` indices.
      const Index excess_index = *least_excess_it;
      InitializeIndex(excess_index, count);

      // Put the remainder in the `unused_indices_` pool.
      const Index remainder_index = excess_index + count;
      InitializeIndex(remainder_index, least_excess);
      *least_excess_it = remainder_index;

      return excess_index;
    }

    // Allocate a new index.
    const Index new_index = num_indices();
    SetNumIndices(new_index + count);
    InitializeIndex(new_index, count);
    return new_index;
  }

  // Recycle 'index'. It will be used in the next allocation, or backfilled in
  // the next call to Defragment().
  // @param index Index to be freed. Must be in the range
  //              [0, num_indices_ - 1].
  void Free(Index index) {
    DCHECK(ValidIndex(index));
    unused_indices_.push_back(index);
  }

  // Only one block of unused indices left, and they're at the end of the
  // array.
  bool UnusedAtEnd() const {
    return unused_indices_.size() == 1 &&
           NextIndex(unused_indices_[0]) == num_indices();
  }

  // Backfill all unused index blocks. That is, move index blocks around
  // until all the unused index blocks have the *highest* indices. Then,
  // shrink the number of indices to remove all unused index blocks.
  //
  // Every time we move an index block, we call callbacks_->MoveIndexRange().
  // In MoveIndexRange(), the callee can correspondingly move its
  // internal data around to match the index shuffle. At the end of
  // Degragment(), the callee's internal data will be contiguous.
  // Contiguous data is essential in data-oriented design, since it
  // minimizes cache misses.
  //
  // Note that we could eliminate Defragment() function by calling MoveIndex()
  // from Free(). The code would be simpler. We move the indices lazily,
  // however, for performance: Defragment() is something that can happen on a
  // background thread.
  //
  // This function has worst case runtime of O(n) index moves, where n is
  // the total number of indices. To see why, notice that indices are only
  // moved forward, and are always moved into the forward-most hole.
  //
  // Note that there is some inefficiency with setting the `count_` array
  // excessively. The worst-case number of operations on the `count_` array
  // is greater than O(n). However, the assumption is that since `count_` is
  // just an array of integers, operations on it are insignificant compared
  // to the actual data movement that happens in callbacks_->MoveIndexRange().
  // However, there is an optimization opportunity here, most likely.
  //
  // In practice, this function will normally perform much better than O(n)
  // moves. We endeavour to fill holes with index blocks near the end of the
  // array. That is, we try to leapfrog the hole to the end of the array
  // when possible.
  //
  // Because of this, when all allocations are the same size, the worst case
  // runtime improves significantly to O(k) index moves, where k is the
  // total number of *unused* indices.
  //
  // If moving an index is cheaper than processing data for an index,
  // then you should call Defragment() right before you process data,
  // for optimal performance.
  //
  // Note that the number of indices shrinks or stays the same in this
  // function, so the final call to SetNumIndices() will never result in a
  // reallocation of the underlying array (which would be slow).
  //
  void Defragment() {
    // Quick check. An optimization.
    if (unused_indices_.size() == 0) return;

    for (;;) {
      // We check if unused index is the last index, so must be in sorted order.
      ConsolidateUnusedIndices();

      // If all the holes have been pushed to the end, we are done and can
      // trim the number of indices.
      const bool unused_at_end = unused_indices_.size() == 1 &&
                                 NextIndex(unused_indices_[0]) == num_indices();
      if (unused_at_end) break;

      // Find range of indices that will fit into the first block of
      // unused indices and move them into it.
      BackfillFirstUnused();
    }

    // Remove hole at end.
    SetNumIndices(unused_indices_[0]);
    unused_indices_.clear();
  }

  // Returns true if there are no indices allocated.
  bool Empty() const { return num_indices() == NumUnusedIndices(); }

  // Returns true if the index is current allocated.
  bool ValidIndex(Index index) const {
    if (index < 0 || index >= num_indices()) return false;

    if (counts_[index] == 0) return false;

    for (ConstIndexIterator it = unused_indices_.begin();
         it != unused_indices_.end(); ++it) {
      if (index == *it) return false;
    }

    return true;
  }

  // Returns the number of wasted indices. These holes will be plugged when
  // Degragment() is called.
  Index NumUnusedIndices() const {
    Count count = 0;
    for (std::size_t i = 0; i < unused_indices_.size(); ++i) {
      count += CountForIndex(unused_indices_[i]);
    }
    return count;
  }

  // Returns the `count` value specified in Alloc. That is, the number of
  // consecutive indices associated with `index`.
  Count CountForIndex(Index index) const {
    DCHECK_GT(counts_[index], 0);
    return counts_[index];
  }

  // Assert if the internal state is invalid in any way.
  // Sanity check on this data structure.
  void VerifyInternalState() const {
    for (Index i = 0; i < num_indices();) {
      // Each block of indices must start with the positive size of the block.
      const Count count = counts_[i];
      CHECK_GT(count, 0);

      // Succeeding elements in a block give the offset back to the start.
      for (Index j = 1; j < count; ++j) {
        CHECK(counts_[i + j] == -j);
      }

      // Jump to the next block.
      i += count;
    }
  }

  // Returns the size of the array that  number of contiguous indices.
  // This includes all the indices that have been free.
  Index num_indices() const { return static_cast<Index>(counts_.size()); }

 private:
  typedef typename std::vector<Index>::const_iterator ConstIndexIterator;
  static const Index kInvalidIndex = static_cast<Index>(-1);

  // Returns the next allocated index. Skips over all indices associated
  // with `index`.
  Index NextIndex(Index index) const {
    DCHECK(0 <= index && index < num_indices() && counts_[index] > 0);
    return index + counts_[index];
  }

  // Returns the previous allocated index. Skips over all indices associated
  // with `index` - 1.
  Index PrevIndex(Index index) const {
    DCHECK(0 < index && index <= num_indices() &&
           (index == num_indices() || counts_[index - 1] == 1 ||
            counts_[index - 1] < 0));
    const Count prev_count = counts_[index - 1];
    return prev_count > 0 ? index - 1 : index - 1 + prev_count;
  }

  // Set up the `counts_` array to hold the size of `index`. Only the value
  // at `counts_[index]` really matters. The others are initialized for
  // debugging, and to make traversal of the `counts_` array easier.
  void InitializeIndex(Index index, Count count) {
    // Initialize the count for this index.
    counts_[index] = count;
    for (Count i = 1; i < count; ++i) {
      counts_[index + i] = -i;
    }
  }

  // Adjust internal state to match the new index size, and notify the
  // callback that size has changed.
  void SetNumIndices(Index new_num_indices) {
    // Increase (or decrease) the count logger.
    counts_.resize(new_num_indices, 0);

    // Report size change.
    callbacks_->SetNumIndices(new_num_indices);
  }

  // Combine adjacent blocks of unused incides in `unused_indices_`.
  void ConsolidateUnusedIndices() {
    // First put the indices in order so that we can process them efficiently.
    std::sort(unused_indices_.begin(), unused_indices_.end());

    // Consolidate adjacent blocks of unused indices.
    std::size_t new_num_unused = 0;
    for (std::size_t i = 0; i < unused_indices_.size();) {
      const Index unused = unused_indices_[i];

      // Find first non-consecutive index in unused_indices_.
      std::size_t j = i + 1;
      while (j < unused_indices_.size() &&
             unused_indices_[j] == NextIndex(unused_indices_[j - 1])) {
        ++j;
      }

      // Consolidate consecutive unused indices.
      const std::size_t num_consecutive = j - i;
      if (num_consecutive > 1) {
        const Count consolidated_count =
            NextIndex(unused_indices_[j - 1]) - unused;
        InitializeIndex(unused, consolidated_count);
      }

      // Write to the output array.
      unused_indices_[new_num_unused++] = unused;

      // Increment the read-counter, skipping over any we've consolidated.
      i += num_consecutive;
    }

    // Shrink number of unused indices. Size can only get smaller.
    DCHECK(new_num_unused <= unused_indices_.size());
    unused_indices_.resize(new_num_unused);
  }

  // Move later blocks of indices into the first hole in `unused_indices_`.
  // That is, move the first hole farther back in the index array.
  void BackfillFirstUnused() {
    DCHECK(unused_indices_.size() > 0);
    const IndexRange unused_range(
        unused_indices_[0],
        unused_indices_[0] + CountForIndex(unused_indices_[0]));

    // Find a fill_range after unused_range that we can move into unused_range.
    //
    // Case 1. Fill
    //   indices in fill_range are moved into the first part of unused_range.
    //   fill_range.Length() <= unused_range.Length()
    //
    //   before:       unused_range              fill_range
    //             |..................|         |abcdefghij|
    //
    //   after:    |abcdefghij|.......|         |..........|
    //                       unused_hole         fill_hole
    //
    //
    // Case 2. Shift
    //   indices in fill_range are moved to the left, into unused_range.
    //   this opens up a hole where fill_range used to end
    //
    //   before:    unused_range    fill_range
    //             |............|abcdefghijklmnop|
    //
    //   after:    |abcdefghijklmnop|............|
    //                                shift_hole
    //
    IndexRange fill_range = LastIndexRangeSmallerThanHole(unused_range.start());
    const bool is_fill = fill_range.Valid();
    if (!is_fill) {
      // If there's no index range that will fit into the hole, shift over
      // all the indices between this hole and the next.
      const Index next_hole_index =
          unused_indices_.size() > 1 ? unused_indices_[1] : num_indices();
      fill_range = IndexRange(NextIndex(unused_range.start()), next_hole_index);
    }

    // Allow the callback to move data associated with the indices.
    callbacks_->MoveIndexRange(fill_range, unused_range.start());

    // Move `counts_` to fill_range's new location
    memmove(&counts_[unused_range.start()], &counts_[fill_range.start()],
            fill_range.Length() * sizeof(counts_[0]));

    // Re-initialize `counts_` for new holes.
    if (is_fill) {
      // See Case 1 above: Add a hole for the range we just moved.
      InitializeIndex(fill_range.start(), fill_range.Length());
      unused_indices_[0] = fill_range.start();

      // If we didn't completely fill unused_range, add a hole for the rest.
      const IndexRange unused_hole(unused_range.start() + fill_range.Length(),
                                   unused_range.end());
      if (unused_hole.Length() > 0) {
        InitializeIndex(unused_hole.start(), unused_hole.Length());
        unused_indices_.push_back(unused_hole.start());
      }
    } else {
      // See Case 2 above: Add a hole at the end of the range we shifted over.
      const IndexRange shift_hole(unused_range.start() + fill_range.Length(),
                                  fill_range.end());
      InitializeIndex(shift_hole.start(), shift_hole.Length());
      unused_indices_[0] = shift_hole.start();
    }
  }

  IndexRange LastIndexRangeSmallerThanHole(Index index) const {
    // We want the last consecutive range of indices of length <= count.
    const Count count = CountForIndex(index);

    // Loop from the back. `end` is the end of the range.
    DCHECK(unused_indices_.size() > 0);
    std::size_t unused_i = unused_indices_.size() - 1;
    for (Index end = num_indices(); end > index; end = PrevIndex(end)) {
      // Skip over unused indices.
      const Index unused_start = unused_indices_[unused_i];
      const Index unused_end = NextIndex(unused_start);
      DCHECK(unused_end <= end);
      if (end == unused_end) {
        unused_i--;
        continue;
      }

      // Loop towards the front while the size still fits into `count`.
      Index start = end;
      for (Index j = PrevIndex(end); j > index; j = PrevIndex(j)) {
        if (end - j > count) break;
        if (j == unused_start) break;
        start = j;
      }

      // If at least some indices are in range, use those.
      if (start < end) return IndexRange(start, end);
    }

    // No index range fits, so return an invalid range.
    return IndexRange();
  }

  // When indices are moved or the number of inidices changes, we notify the
  // caller via these callbacks.
  CallbackInterface* callbacks_;

  // For every valid index, the number of indices
  // associated with that index. For intermediate indices, negative number
  // representing the offset to the actual index.
  //
  //              valid indices
  //               |   |      |            |   |
  //               v   v      v            v   v
  // For example:  1 | 2 -1 | 4 -1 -2 -3 | 1 | 1
  //                      ^      ^  ^  ^
  //                      |      |  |  |
  //                     offset to the actual index
  std::vector<Count> counts_;

  // When an index is freed, we keep track of it here. When an index is
  // allocated, we use one off this array, if one exists.
  // When Defragment() is called, we empty this array by filling all the
  // unused indices with the highest allocated indices. This reduces the total
  // size of the data arrays.
  std::vector<Index> unused_indices_;
};

}  // namespace redux

#endif  // REDUX_ENGINES_ANIMATION_PROCESSOR_INDEX_ALLOCATOR_H_
