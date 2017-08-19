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

#ifndef LULLABY_UTIL_UNORDERED_VECTOR_MAP_H_
#define LULLABY_UTIL_UNORDERED_VECTOR_MAP_H_

#include <assert.h>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace lull {

// A map-like container of Key to Object.
//
// Objects are stored in a vector of arrays to ensure good locality of reference
// when iterating over them.  Efficient iteration can be done by calling ForEach
// or by using the provided iterators. An unordered_map is used to provide O(1)
// access to individual objects.
//
// New objects are always inserted at the "end" of the vector of arrays.
// Objects are removed by first swapping the "target" object with the "end"
// object, then popping the last object of the end.  The swap is performed
// using std::swap.
//
// This container does not provide any order guarantees.  Objects stored in
// the containers will be shuffled around during removal operations.  Any
// pointers to objects in the container should be used with care.  This
// container is also not thread safe.  Finally, the ForEach function is not
// re-entrant - do not insert/remove objects from the container during
// iteration.
template <typename Key, typename Object, typename KeyFn,
          typename LookupHash = std::hash<Key>>
class UnorderedVectorMap {
  template <bool IsConst>
  class Iterator;

 public:
  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;

  // The |page_size| specifies the number of elements to store in contiguous
  // memory before allocating a new "page" for more elements.
  explicit UnorderedVectorMap(size_t page_size) : page_size_(page_size) {}

  UnorderedVectorMap(const UnorderedVectorMap& rhs) = delete;
  UnorderedVectorMap& operator=(const UnorderedVectorMap& rhs) = delete;

  // Default implementation of move constructor.  The MSVC12 toolchain does not
  // support =default for move constructors, so define it explicitly.
  // TODO(b/28276908) Remove after switch to MSVC 2015.
  UnorderedVectorMap(UnorderedVectorMap&& rhs)
      : objects_(std::move(rhs.objects_)),
        lookup_table_(std::move(rhs.lookup_table_)),
        page_size_(rhs.page_size_) {}

  // Default implementation of move assignment.  The MSVC12 toolchain does not
  // support =default for move assignment, so define it explicitly.
  // TODO(b/28276908) Remove after switch to MSVC 2015.
  UnorderedVectorMap& operator=(UnorderedVectorMap&& rhs) {
    if (this != &rhs) {
      objects_ = std::move(rhs.objects_);
      lookup_table_ = std::move(rhs.lookup_table_);
      page_size_ = rhs.page_size_;
    }
    return *this;
  }

  // Emplaces an object at the end of the container's internal memory and
  // returns a pointer to it.  Returns nullptr if there is already an Object in
  // the container that Hashes to the same key.
  // Note: The Object will be created in order to call the KeyFn() function to
  // determine its key.  If there is a collision, the newly created Object will
  // be immediately destroyed.
  template<typename... Args>
  Object* Emplace(Args&&... args) {
    // Grow the internal storage if necessary, either because this is the first
    // element being added or because the "back" array is full.
    if (objects_.empty() || objects_.back().Size() == page_size_) {
      objects_.emplace_back(page_size_);
    }

    // Add the element to the "end" of the ArrayVector.
    auto& back_page = objects_.back();
    Object* obj = back_page.Push(std::forward<Args>(args)...);

    // Check to see if an object with this key is already being stored and, if
    // so, remove it.  Otherwise, add this new one.  Unfortunately, we can
    // only check the key after we have created the object.
    KeyFn key_fn;
    const auto& key = key_fn(*obj);
    const Index index(objects_.size() - 1, back_page.Size() - 1);

    if (lookup_table_.emplace(key, index).second) {
      return obj;
    } else {
      Destroy(index);
      return nullptr;
    }
  }

  // Destroys the Object associated with |key|.  The Object being destroyed will
  // be swapped with the object at the end of the internal storage structure,
  // and then will be "popped" off the back.
  void Destroy(const Key& key) {
    auto iter = lookup_table_.find(key);
    if (iter == lookup_table_.end()) {
      return;
    }
    Destroy(iter->second);
    lookup_table_.erase(iter);
  }

  // Returns a pointer to the Object associated with |key|, or nullptr if no
  // such Object exists.
  Object* Get(const Key& key) {
    auto iter = lookup_table_.find(key);
    if (iter == lookup_table_.end()) {
      return nullptr;
    }

    const Index& index = iter->second;
    Object* obj = objects_[index.first].Get(index.second);
    return obj;
  }

  // Returns a pointer to the Object associated with |key|, or nullptr if no
  // such Object exists.
  const Object* Get(const Key& key) const {
    auto iter = lookup_table_.find(key);
    if (iter == lookup_table_.end()) {
      return nullptr;
    }

    const Index& index = iter->second;
    const Object* obj = objects_[index.first].Get(index.second);
    return obj;
  }

  // Iterates over all Objects, passing them to the given function |Fn|.
  template<typename Fn>
  void ForEach(Fn&& fn) {
    for (auto& object : *this) {
      fn(object);
    }
  }

  // Iterates over all Objects, passing them to the given function |Fn|.
  template<typename Fn>
  void ForEach(Fn&& fn) const {
    for (auto& object : *this) {
      fn(object);
    }
  }

  // Returns the number of Objects stored in the container.
  size_t Size() const {
    const size_t objects_size = objects_.size();
    if (objects_size == 0) {
      return 0;
    }

    const size_t back_size = objects_.back().Size();
    return ((objects_size - 1) * page_size_) + back_size;
  }

  iterator begin() { return iterator(objects_.begin(), objects_.end()); }

  const_iterator begin() const {
    return const_iterator(objects_.begin(), objects_.end());
  }

  iterator end() { return iterator(objects_.end()); }

  const_iterator end() const { return const_iterator(objects_.end()); }

 private:
  // Storage for the actual Object instances.
  //
  // Ideally, we would just use an std::vector<Object>.  However, for the
  // Android toolchain (as of May 2016), this requires Object to be copyable
  // even though we go through great lengths in the UnorderedVectorMap to ensure
  // that Objects will never be copied, only moved.  As a workaround, we
  // implement our own vector-like container that allows for storing of
  // non-copyable objects.  This container implements the minimal API that is
  // needed to work with the UnorderedVectorMap.
  class ObjectArray {
   public:
    using iterator = Object*;
    using const_iterator = Object const*;

    // Allocates an array that can hold the specified number of Objects.
    explicit ObjectArray(size_t max) : memory_(nullptr), count_(0), max_(max) {
#ifdef _MSC_VER
      const size_t kAlignment = __alignof(Object);
#else
      const size_t kAlignment = alignof(Object);
#endif
      uint8_t* ptr = new uint8_t[sizeof(Object) * max];
      assert(reinterpret_cast<intptr_t>(ptr) % kAlignment == 0);
      // "Use" kAlignment variable since it seems unused in non-dbg.
      (void)kAlignment;
      memory_.reset(ptr);
    }

    ObjectArray(const ObjectArray&) = delete;
    ObjectArray& operator=(const ObjectArray&) = delete;

    // Moves allocated memory from |rhs| to |this|.
    ObjectArray(ObjectArray&& rhs)
        : memory_(std::move(rhs.memory_)), count_(rhs.count_), max_(rhs.max_) {
      rhs.count_ = 0;
      rhs.max_ = 0;
    }

    // Destroys any constructed Objects contained in this class.
    ~ObjectArray() {
      while (count_ > 0) {
        Pop();
      }
    }

    // Constructs a new Object at the end of the storage.
    template <typename... Args>
    Object* Push(Args&&... args) {
      assert(count_ < max_);
      Object* obj = Get(count_);
      new (obj) Object(std::forward<Args>(args)...);
      ++count_;
      return obj;
    }

    // Destroys the created Object at the end of the storage.
    void Pop() {
      assert(count_ > 0);
      --count_;
      Object* obj = Get(count_);
      obj->~Object();
    }

    // Gets the Object at the given |index|.
    Object* Get(size_t index) {
      Object* arr = reinterpret_cast<Object*>(memory_.get());
      return arr + index;
    }

    // Gets the Object at the given |index|.
    const Object* Get(size_t index) const {
      const Object* arr = reinterpret_cast<const Object*>(memory_.get());
      return arr + index;
    }

    // Returns the number of constructed Objects in the container.
    size_t Size() const { return count_; }

    iterator begin() { return Get(0); }

    const_iterator begin() const { return Get(0); }

    iterator end() { return Get(count_); }

    const_iterator end() const { return Get(count_); }

   private:
    // Memory for storing Object instances.
    std::unique_ptr<uint8_t[]> memory_;

    // Number of Objects that have been constructed/emplaced.
    size_t count_;

    // Maximum number of Objects that can be stored.
    size_t max_;
  };

  // An array of an array of Objects for cache-efficient iteration.
  using ArrayVector = std::vector<ObjectArray>;

  // A pair of indices to the two arrays in the ArrayVector.
  using Index = std::pair<size_t, size_t>;

  // Table that maps a Key to a specific element in the ArrayVector.
  using LookupTable = std::unordered_map<Key, Index, LookupHash>;

  // An STL style iterator for accessing the elements of an UnorderedVectorMap.
  // This class provides both the const and non-const implementation. If the
  // container is modified at any point during the iteration then the iterator
  // will become invalid.
  template <bool IsConst>
  class Iterator {
    using OuterIterator =
        typename std::conditional<IsConst, typename ArrayVector::const_iterator,
                                  typename ArrayVector::iterator>::type;
    using InnerIterator =
        typename std::conditional<IsConst, typename ObjectArray::const_iterator,
                                  typename ObjectArray::iterator>::type;

   public:
    // STL iterator traits.
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename std::iterator_traits<InnerIterator>::value_type;
    using difference_type =
        typename std::iterator_traits<InnerIterator>::difference_type;
    using reference = typename std::iterator_traits<InnerIterator>::reference;
    using pointer = typename std::iterator_traits<InnerIterator>::pointer;

    // A default constructed iterator should first be assigned to a valid
    // iterator. Any operation before assignment other than destruction is
    // undefined.
    Iterator() = default;

    // Allow conversion from the non-const iterator to the const
    // iterator. Relies on the fact that the internal iterators only support
    // conversions from non-const to const.
    Iterator(const Iterator<false>& other)
        : outer_(other.outer_),
          outer_end_(other.outer_end_),
          inner_(other.inner_) {}

    reference operator*() const {
      assert(outer_ != outer_end_);
      return *inner_;
    }

    pointer operator->() const {
      assert(outer_ != outer_end_);
      return inner_;
    }

    Iterator& operator++() {
      assert(outer_ != outer_end_);
      ++inner_;
      FindNextElement();
      return *this;
    }

    Iterator operator++(int) {
      Iterator temp(*this);
      operator++();
      return temp;
    }

    // Allow assignment from a non-const iterator to a const iterator. Relies on
    // the fact that the internal iterators only support assignment from
    // non-const to const.
    Iterator& operator=(const Iterator<false>& other) {
      outer_ = other.outer_;
      outer_end_ = other.outer_end_;
      inner_ = other.inner_;
      return *this;
    }

    // Allow equality comparison between const and non-const iterators.
    friend bool operator==(const Iterator& lhs, const Iterator& rhs) {
      return lhs.outer_ == rhs.outer_ &&
             (lhs.outer_ == lhs.outer_end_ || lhs.inner_ == rhs.inner_);
    }

    // Allow inequality comparison between const and non-const iterators.
    friend bool operator!=(const Iterator& lhs, const Iterator& rhs) {
      return !(lhs == rhs);
    }

   private:
    // Allow the container class access to the private constructors.
    friend class UnorderedVectorMap;
    // Allow the const iterator access to the private members of the non-const
    // iterator for the conversion constructor and assignment.
    friend class Iterator<true>;

    // Construct an end iterator.
    explicit Iterator(OuterIterator outer_end)
        : Iterator(outer_end, outer_end) {}

    // Construct an iterator with a potentially different beginning and end.
    Iterator(OuterIterator outer_begin, OuterIterator outer_end)
        : outer_(outer_begin), outer_end_(outer_end) {
      if (outer_ != outer_end_) {
        inner_ = outer_->begin();
        FindNextElement();
      }
    }

    // If the inner iterator has reached the end of the current container then
    // find the next element, skipping over any empty inner containers.
    void FindNextElement() {
      while (inner_ == outer_->end() && ++outer_ != outer_end_) {
        inner_ = outer_->begin();
      }
    }

    OuterIterator outer_;
    OuterIterator outer_end_;
    InnerIterator inner_;
  };

  // Destroys the Object at the specified |index|.  Performs a swap-and-pop for
  // Objects not at the end of the ArrayVector.
  void Destroy(const Index& index) {
    // The object to remove is in the "middle" of the ArrayVector, so swap it
    // with the one at the very end.
    auto& back_page = objects_.back();
    const bool is_in_last_page = (index.first == objects_.size() - 1);
    const bool is_last_element = (index.second == back_page.Size() - 1);
    if (!is_in_last_page || !is_last_element) {
      Object* obj = objects_[index.first].Get(index.second);
      Object* other = back_page.Get(back_page.Size() - 1);
      KeyFn key_fn;
      lookup_table_[key_fn(*other)] = index;
      using std::swap;
      swap(*obj, *other);
    }

    // The object we want to destroy is at the very back, so just pop it.
    objects_.back().Pop();

    // If the "back" array is empty, we can remove it.
    if (objects_.back().Size() == 0) {
      objects_.pop_back();
    }
  }

  // The vector of arrays used to store Object instances.
  ArrayVector objects_;

  // The map of key to Index in the ArrayVector for an Object.
  LookupTable lookup_table_;

  // The maximum size of the internal array in the ArrayVector.
  size_t page_size_;
};

}  // namespace lull

#endif  // LULLABY_UTIL_UNORDERED_VECTOR_MAP_H_
