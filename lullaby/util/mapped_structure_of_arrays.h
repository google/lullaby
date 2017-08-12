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

#ifndef LULLABY_UTIL_MAPPED_STRUCTURE_OF_ARRAYS_H_
#define LULLABY_UTIL_MAPPED_STRUCTURE_OF_ARRAYS_H_

#include <unordered_map>

#include "lullaby/util/structure_of_arrays.h"
#include "lullaby/util/logging.h"

namespace lull {

// The mapped structure of arrays a structure of arrays of type |Elements| and
// tracks the position of each element via a given |KeyType|.
//
// This is useful for things like tracking components belonging to an entity
// while keeping different types of the data tightly packed in structure of
// arrays. For example:
// using PositionType = vec3;
// using VelocityType = vec3;
// using MassType = float;
// MappedStructureOfArrays<Entity,PositionType,VelocityType,MassType> physics;
// physics.Emplace(entity, position, velocity, mass);
// // Add more physics entities...
//
// // Remove a specific physics entity
// physics.Remove(entity);
//
// Note that the map is not guaranteed to maintain the same order between
// operations and positions of elements may move around if Pop or Remove are
// used. Add, Emplace and other operations will keep the order but may
// invalidate the memory address returned by Data.
template <typename KeyType, typename... Elements>
class MappedStructureOfArrays {
 public:
  MappedStructureOfArrays() {}

  // Inserts an empty element to the arrays, referenced by |key|.
  void Insert(const KeyType& key) {
    if (Contains(key)) {
      LOG(DFATAL) << "Attempting to add " << key
                  << " but an element already exist.";
      return;
    }

    const size_t index = index_to_key_.size();
    index_to_key_.push_back(key);
    soa_.Resize(soa_.Size() + 1);
    key_to_index_[key] = index;
  }

  // Inserts an element to the arrays, referenced by |key|.
  void Insert(const KeyType& key, const Elements&... elements) {
    if (Contains(key)) {
      LOG(DFATAL) << "Attempting to add " << key
                  << " but an element already exist.";
      return;
    }

    const size_t index = index_to_key_.size();
    index_to_key_.push_back(key);
    soa_.Push(elements...);
    key_to_index_[key] = index;
  }

  // Emplaces elements into the arrays, referenced by |key|.
  void Emplace(const KeyType& key, Elements&&... elements) {
    if (Contains(key)) {
      LOG(DFATAL) << "Attempting to emplace " << key
                  << " but an element already exist.";
      return;
    }

    const size_t index = index_to_key_.size();
    index_to_key_.push_back(key);
    soa_.Emplace(std::move(elements...));
    key_to_index_[key] = index;
  }

  // Removes an element references by |key| from the arrays.
  void Remove(const KeyType& key) {
    const auto it = key_to_index_.find(key);
    if (it == key_to_index_.end()) {
      LOG(DFATAL) << "Attempting to remove " << key << " but it doesn't exist.";
      return;
    }

    const size_t index = key_to_index_[key];
    const size_t end_index = index_to_key_.size() - 1;
    if (index < end_index) {
      Swap(index, end_index);
    }

    index_to_key_.pop_back();
    soa_.Pop();
    key_to_index_.erase(key);
  }

  // Swaps the positions of two elements in the arrays.
  void Swap(size_t index0, size_t index1) {
    const size_t size = Size();
    if (size <= index0 || size <= index1) {
      LOG(DFATAL) << "Attempting to swap elements at indices" << index0
                  << "and " << index1 << " but only have " << size
                  << "elements.";
      return;
    }

    const KeyType key0 = index_to_key_[index0];
    const KeyType key1 = index_to_key_[index1];

    using std::swap;
    swap(key_to_index_[key0], key_to_index_[key1]);
    swap(index_to_key_[index0], index_to_key_[index1]);

    soa_.Swap(index0, index1);
  }

  // Helper to deduce the type of the Nth element.
  template <size_t N>
  using NthTypeOf =
      typename StructureOfArrays<Elements...>::template NthTypeOf<N>;

  // Returns a pointer to the |ArrayIndex|th array.
  template <std::size_t ArrayIndex>
  NthTypeOf<ArrayIndex>* Data() {
    return soa_.template Data<ArrayIndex>();
  }

  // Returns a const pointer to the |ArrayIndex|th array.
  template <std::size_t ArrayIndex>
  const NthTypeOf<ArrayIndex>* Data() const {
    return soa_.template Data<ArrayIndex>();
  }

  // Returns ref of the element referenced by |key| at the |ArrayIndex|th array.
  template <std::size_t ArrayIndex>
  NthTypeOf<ArrayIndex>& At(const KeyType& key) {
    return soa_.template At<ArrayIndex>(GetIndex(key));
  }

  // Returns const ref of the element referenced by |key| at the |ArrayIndex|th
  // array.
  template <std::size_t ArrayIndex>
  const NthTypeOf<ArrayIndex>& At(const KeyType& key) const {
    return soa_.template At<ArrayIndex>(GetIndex(key));
  }

  // Returns the index of elements with |key| in the array. If the key doesn't
  // exist it will return -1 which translate to max size of size_t.
  size_t GetIndex(const KeyType& key) const {
    const auto it = key_to_index_.find(key);
    if (it == key_to_index_.end()) {
      LOG(DFATAL) << "Element with key " << key << " doesn't exist.";
      return -1;
    }

    return it->second;
  }

  bool Contains(const KeyType& key) const {
    return key_to_index_.count(key) != 0;
  }

  bool Empty() const { return soa_.Empty(); }

  // Returns the number of elements inside all of the arrays.
  size_t Size() const { return soa_.Size(); }

 private:
  using KeyToIndexMap = std::unordered_map<KeyType, size_t>;
  using IndexToKey = std::vector<KeyType>;

  StructureOfArrays<Elements...> soa_;
  KeyToIndexMap key_to_index_;
  IndexToKey index_to_key_;
};

}  // namespace lull

#endif  // LULLABY_UTIL_MAPPED_STRUCTURE_OF_ARRAYS_H_
