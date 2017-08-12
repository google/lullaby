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

#ifndef LULLABY_UTIL_STRUCTURE_OF_ARRAYS_H_
#define LULLABY_UTIL_STRUCTURE_OF_ARRAYS_H_

#include <memory>
#include <type_traits>
#include <vector>

#include "lullaby/util/logging.h"

namespace lull {

// A structure of arrays holds a number of arrays of different types, making it
// easy to keep, track and maintain multiple types together while keeping each
// in separate tight arrays for fast iteration.
//
// Example usage:
// // Arrays of positions and velocities.
// StructureOfArrays<mathfu::vec3, mathfu::vec3, float> soa;
// soa.push_back(pos0, mathfu::kZeros3f);
// soa.push_back(pos1, mathfu::kZeros3f);
// soa.push_back(pos2, mathfu::kZeros3f);
//
// // Add some velocities...
// soa.Get<1>(0) = some_velocity;
// soa.Get<1>(1) = some_other_velocity;
// soa.Get<1>(2) = some_other_other_velocity;
//
// Iterate the arrays adding velocities to positions.
// mathfu::vec3* positions = soa.Data<0>();
// const mathfu::vec3* velocities = soa.Data<1>();
// for (int i = 0; i < soa.Size(); ++i) {
//   positions[i] += velocities[i] * delta_seconds;
// }

// This macro does pack expansion in a braced-init-list. The inlined expression
// in the braces is called for every type in the variadic type (Elements...).
// At each iteration we retrieve an array of each type using GetArray and call
// the expression from it. The variable 'i' is used to track which iteration we
// are at and is used to get the correct array for each type.
// The dummy variable is then cast to avoid compiler errors for unused variable.
//
// Used on the structure above, the macro is expanded as such:
// size_t i = 0;
// int dummy[] = {
//   (GetArray<mathfu::vec3>(i)->expression, ++i, 0),
//   (GetArray<mathfu::vec3>(i)->expression, ++i, 0),
//   (GetArray<float>(i)->expression, ++i, 0),
// };
// (void)dummy;
//
// This is essentially creating an array from a braced-init-list of
// types (void, int, 0) and each of the functions is called in order for each
// type in the template. The comma operator then collapses it into the final
// element, so we are left with:
// int dummy[] = { 0, 0, 0 };
// ... which we toss away.
//
// Refer to this page for more info:
// http://en.cppreference.com/w/cpp/language/parameter_pack
#define LULLABY_SOA_FOR_EACH(expression)                          \
  size_t i = 0;                                                   \
  int dummy[] = {(GetArray<Elements>(i)->expression, ++i, 0)...}; \
  (void)dummy

template <typename... Elements>
class StructureOfArrays {
 public:
  // Helper to deduce the type of the Nth element.
  template <size_t N>
  using NthTypeOf =
      typename std::tuple_element<N, std::tuple<Elements...>>::type;

  StructureOfArrays() {
    // This macro does pack expansion in a braced-init-list. The inlined
    // expression in the braces is called for every type in the variadic type
    // (Elements...). At each iteration we retrieve an array of each type using
    // GetArray and call the expression from it. The variable 'i' is used to
    // track which iteration we are at and is used to get the correct array for
    // each type. The dummy variable is then cast to avoid compiler errors for
    // unused variable.
    //
    // Refer to this page for more info:
    // http://en.cppreference.com/w/cpp/language/parameter_pack
    size_t array_index = 0;
    int dummy_init[] = {(
        new (GetArray<Elements>(array_index++)) std::vector<Elements>(), 0)...};
    (void)dummy_init;  // avoids unused variable compiler warnings.
  }

  StructureOfArrays(const StructureOfArrays& other) : StructureOfArrays() {
    *this = other;
  }

  StructureOfArrays(StructureOfArrays&& other) {
    size_t idx = 0;
    int dummy_init[] = {(new (GetArray<Elements>(idx)) std::vector<Elements>(
                             std::move(*other.GetArray<Elements>(idx))),
                         ++idx, 0)...};
    (void)dummy_init;  // avoids unused variable compiler warnings.

    size_ = other.size_;
    other.size_ = 0;
  }

  ~StructureOfArrays() { LULLABY_SOA_FOR_EACH(~vector()); }

  StructureOfArrays& operator=(const StructureOfArrays& other) {
    size_t idx = 0;
    int dummy_init[] = {
        (*GetArray<Elements>(idx) = *other.GetArray<Elements>(idx), ++idx,
         0)...};
    (void)dummy_init;  // avoids unused variable compiler warnings.

    size_ = other.size_;
    return *this;
  }

  StructureOfArrays& operator=(StructureOfArrays&& other) {
    size_t idx = 0;
    int dummy_init[] = {
        (*GetArray<Elements>(idx) = std::move(*other.GetArray<Elements>(idx)),
         ++idx, 0)...};
    (void)dummy_init;  // avoids unused variable compiler warnings.

    size_ = other.size_;
    other.size_ = 0;
    return *this;
  }

  // Returns the number of elements in the arrays.
  size_t Size() const { return size_; }

  // Returns |true| if there are no elements in the arrays, |false| otherwise.
  bool Empty() const { return (size_ == 0); }

  // Adds an element to the end of the arrays via rvalue.
  void Emplace(Elements&&... elements) {
    LULLABY_SOA_FOR_EACH(emplace_back(std::forward<Elements>(elements)));

    ++size_;
  }

  // Adds an element to the end of the arrays, as a copy of the const ref value.
  void Push(const Elements&... elements) {
    LULLABY_SOA_FOR_EACH(push_back(elements));

    ++size_;
  }

  // Erases the last element in the arrays.
  void Pop() {
    if (Empty()) {
      return;
    }

    LULLABY_SOA_FOR_EACH(pop_back());

    --size_;
  }

  // Erases elements of a given index from the arrays.
  void Erase(size_t element_index) {
    if (element_index >= size_) {
      LOG(DFATAL) << "Attempting to erase " << element_index << " while size is"
                  << size_;

      return;
    }

    size_t array_index = 0;
    int dummy[] = {(EraseImpl<Elements>(element_index, array_index++), 0)...};
    (void)dummy;

    --size_;
  }

  // Erases |num_elements| elements from |start_index| from the arrays.
  void Erase(size_t start_index, size_t num_elements) {
    if (start_index >= size_ || (start_index + num_elements) > size_) {
      LOG(DFATAL) << "Attempting to erase " << start_index << " to "
                  << (start_index + num_elements) << " while size is" << size_;

      return;
    }

    size_t array_index = 0;
    int dummy[] = {
        (EraseImpl<Elements>(start_index, num_elements, array_index++), 0)...};
    (void)dummy;

    // Decrement number of elements in the arrays.
    size_ -= num_elements;
  }

  // Swaps two elements inside the array.
  void Swap(size_t index0, size_t index1) {
    if (index0 >= size_ || index1 >= size_) {
      LOG(DFATAL) << "Attempting to swap " << index0 << " and " << index1
                  << " elements, but size is" << size_;

      return;
    }

    if (index0 == index1) {
      return;
    }

    size_t array_index = 0;
    int dummy[] = {(SwapImpl<Elements>(index0, index1, array_index++), 0)...};
    (void)dummy;
  }

  // Reserves enough memories in all arrays to contain |reserve_size| elements.
  void Reserve(size_t reserve_size) {
    LULLABY_SOA_FOR_EACH(reserve(reserve_size));
  }

  // Resizes the arrays to contain |size| elements;
  void Resize(size_t size) {
    LULLABY_SOA_FOR_EACH(resize(size));

    size_ = size;
  }

  // Returns a pointer to the |ArrayIndex|th array.
  template <std::size_t ArrayIndex>
  NthTypeOf<ArrayIndex>* Data() {
    DCHECK(ArrayIndex < kNumElements);

    using ElementType = NthTypeOf<ArrayIndex>;
    std::vector<ElementType>* array = GetArray<ElementType>(ArrayIndex);

    return array->data();
  }

  // Returns a const pointer to the |ArrayIndex|th array.
  template <std::size_t ArrayIndex>
  const NthTypeOf<ArrayIndex>* Data() const {
    DCHECK(ArrayIndex < kNumElements);

    using ElementType = NthTypeOf<ArrayIndex>;
    const std::vector<ElementType>* array = GetArray<ElementType>(ArrayIndex);

    return array->data();
  }

  // Returns a reference to the |index|th element from the |ArrayIndex|th array
  // as type |ElementType|.
  template <std::size_t ArrayIndex>
  NthTypeOf<ArrayIndex>& At(size_t index) {
    DCHECK(ArrayIndex < kNumElements);
    DCHECK(index < size_);

    using ElementType = NthTypeOf<ArrayIndex>;
    std::vector<ElementType>* array = GetArray<ElementType>(ArrayIndex);

    return (*array)[index];
  }

  // Returns a const reference to the |index|th element from the |ArrayIndex|th
  // array as type |ElementType|.
  template <std::size_t ArrayIndex>
  const NthTypeOf<ArrayIndex>& At(size_t index) const {
    DCHECK(ArrayIndex < kNumElements);
    DCHECK(index < size_);

    using ElementType = NthTypeOf<ArrayIndex>;
    const std::vector<ElementType>* array = GetArray<ElementType>(ArrayIndex);

    return (*array)[index];
  }

  // Returns the number of arrays.
  size_t GetNumElements() const { return kNumElements; }

 private:
  template <class Type>
  std::vector<Type>* GetArray(size_t array_index) {
    static_assert(sizeof(std::vector<Type>) == sizeof(std::vector<void*>),
                  "Structure of Array assumes of vector<Type> is same size as "
                  "vector<void*> however the assumption failed.");
    static_assert(alignof(std::vector<Type>) == alignof(std::vector<void*>),
                  "Structure of Array assumes of vector<Type> is same "
                  "alignment as vector<void*> however the assumption failed.");

    return reinterpret_cast<std::vector<Type>*>(&arrays_[array_index]);
  }

  template <class Type>
  const std::vector<Type>* GetArray(size_t array_index) const {
    static_assert(sizeof(std::vector<Type>) == sizeof(std::vector<void*>),
                  "Structure of Array assumes of vector<Type> is same size as "
                  "vector<void*> however the assumption failed.");
    static_assert(alignof(std::vector<Type>) == alignof(std::vector<void*>),
                  "Structure of Array assumes of vector<Type> is same "
                  "alignment as vector<void*> however the assumption failed.");

    return reinterpret_cast<const std::vector<Type>*>(&arrays_[array_index]);
  }

  template <class Type>
  void EraseImpl(size_t element_index, size_t array_index) {
    std::vector<Type>* array = GetArray<Type>(array_index);

    array->erase(array->begin() + element_index);
  }

  template <class Type>
  void EraseImpl(size_t element_index, size_t num_elements,
                 size_t array_index) {
    std::vector<Type>* array = GetArray<Type>(array_index);

    array->erase(array->begin() + element_index,
                 array->begin() + element_index + num_elements);
  }

  template <class Type>
  void SwapImpl(size_t index0, size_t index1, size_t array_index) {
    std::vector<Type>* array = GetArray<Type>(array_index);

    using std::swap;
    swap((*array)[index0], (*array)[index1]);
  }

  using DataStorageType = std::vector<void*>;
  using ArrayType = std::aligned_storage<sizeof(DataStorageType),
                                         alignof(DataStorageType)>::type;
  static const size_t kNumElements = sizeof...(Elements);
  size_t size_ = 0;
  ArrayType arrays_[kNumElements];
};

}  // namespace lull

#endif  // LULLABY_UTIL_STRUCTURE_OF_ARRAYS_H_
