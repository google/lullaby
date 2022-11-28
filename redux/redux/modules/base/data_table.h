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

#ifndef REDUX_MODULES_BASE_DATA_TABLE_H_
#define REDUX_MODULES_BASE_DATA_TABLE_H_

#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/base/ref_tuple.h"

namespace redux {

// Provides a way to give a DataTable column a name, type, and default value.
//
// To use, define a new type that derives from DataColumn, and provide the
// type and an (optional) address of the constexpr function to use for creating
// a default value. For example:
//
//     struct Scale : DataColumn<vec3, &vec3::One> {};
//
// See DataTable class below for more information.
template <typename T, T (*Fn)() = nullptr>
struct DataColumn {
  using Type = T;

  static constexpr Type DefaultValue() { return Fn != nullptr ? Fn() : T{}; }
};

// An unordered associative container that maps keys to multiple values.
//
// The primary reason to use a DataTable over other associative containers (e.g.
// absl::flat_hash_map) is for high-performance iteration over all values. This
// is accomplished by storing each column of data in the DataTable in its own
// tightly-packed, vector-like data structure.
//
// Each column in the DataTable is defined by a unique DataColumn as a template
// argument (similar to std::tuple).  For example:
//
//     struct kEntity : DataColumn<Entity> {};
//     struct kPosition : DataColumn<vec3> {};
//     struct kRotation : DataColumn<quat, &quat::Identity> {};
//     struct kScale : DataColumn<vec3, &vec3::One> {};
//     using TransformTable = DataTable<kEntity, kPosition, kRotation, kScale>;
//
// The first DataColumn is used as the Key to perform lookups. Lookups return
// DataTable::Row objects which contain pointers to the actual data values,
// wrapped up in RefTuples. (See RefTuple for more information.)
//
// Iterating over the data is more efficient than with other maps since the
// data is guaranteed to be stored in contiguous memory. It is also possible to
// iterate over a specific subset of columns for better performance.
//
// A separate hash table (currently absl::flat_hash_map) is used to map Key
// values to specific offsets/indices into the DataTable for efficient lookup.
//
// Whereas new elements are added to the end of the table, erasing elements is
// done using the swap-and-pop idiom. This approach is more efficient, but it
// does mean that no order guarantees are provided (similar to unordered_map).
template <typename Key, typename... Fields>
class DataTable {
 public:
  using KeyType = typename Key::Type;

  // The index_sequence representing all the elements.
  using DefaultBindings = std::index_sequence_for<Key, Fields...>;

  // Proxy object to a single row of data in the table.
  using Row = RefTuple<DefaultBindings, const Key, Fields...>;
  using ConstRow = RefTuple<DefaultBindings, const Key, const Fields...>;

  // Initializes the internal storage for each column.
  DataTable() { InitImpl(DefaultBindings()); }

  // Removes all data from the table.
  void Clear() { ClearImpl(DefaultBindings()); }

  // Returns the number of rows in the table.
  std::size_t Size() const { return lookup_.size(); }

  // Returns a Row for the given key, creating the row if necessary.
  Row TryEmplace(const KeyType& key) {
    if (auto iter = lookup_.find(key); iter != lookup_.end()) {
      return GetImpl(iter->second, DefaultBindings());
    } else {
      return EmplaceImpl(DefaultBindings(), key, Fields::DefaultValue()...);
    }
  }

  // As above, but with additional arguments that can be used to populate a
  // newly constructor Row if needed.
  template <typename... Args>
  Row TryEmplace(const KeyType& key, Args&&... args) {
    if (auto iter = lookup_.find(key); iter != lookup_.end()) {
      return GetImpl(iter->second, DefaultBindings());
    } else {
      return EmplaceImpl(DefaultBindings(), key, std::forward<Args>(args)...);
    }
  }

  // Removes the row associated with the given key. Internally, this will cause
  // a swap-and-pop of the element with the "last" element in the container.
  // As such, this function effectively invalidates any iterators/rows.
  void Erase(const KeyType& key) {
    auto iter = lookup_.find(key);
    if (iter == lookup_.end()) {
      return;
    }

    const Index back = GetIndex(lookup_.size() - 1);
    const KeyType other_key = GetKey(back);

    const Index index = iter->second;
    SwapAndPopImpl(index, DefaultBindings());

    if (index.page != back.page || index.element != back.element) {
      lookup_[other_key] = index;
    }
    lookup_.erase(iter);
  }

  // Returns true if there is data associated the key.
  bool Contains(const KeyType& key) const {
    return lookup_.contains(key);
  }

  // Returns the full Row associated with the key.
  Row FindRow(const KeyType& key) { return FindImpl(key, DefaultBindings()); }

  // Returns the full ConstRow associated with the key.
  ConstRow FindRow(const KeyType& key) const {
    return FindImpl(key, DefaultBindings());
  }

  // Returns the Row associated with the key. The row will only contain the
  // columns specified by T...
  template <typename... T>
  auto Find(const KeyType& key) {
    return FindImpl(key, BindingsFor<T...>());
  }

  // Returns the ConstRow associated with the key. The row will only contain the
  // columns specified by T...
  template <typename... T>
  auto Find(const KeyType& key) const {
    return FindImpl(key, BindingsFor<T...>());
  }

  // Returns the n-th Row in the data table.
  Row At(std::size_t n) {
    CHECK_LT(n, lookup_.size()) << "Index out of bounds.";
    return GetImpl(GetIndex(n), DefaultBindings());
  }

  // Returns the n-th ConstRow in the data table.
  ConstRow At(std::size_t n) const {
    CHECK_LT(n, lookup_.size()) << "Index out of bounds.";
    return GetImpl(GetIndex(n), DefaultBindings());
  }

  // Swaps data between two rows.
  void Swap(std::size_t n0, std::size_t n1) {
    CHECK_LT(n0, lookup_.size()) << "Index out of bounds.";
    CHECK_LT(n1, lookup_.size()) << "Index out of bounds.";
    if (n0 != n1) {
      const Index index0 = GetIndex(n0);
      const Index index1 = GetIndex(n1);
      const KeyType key0 = GetKey(index0);
      const KeyType key1 = GetKey(index1);
      SwapImpl(index0, index1, DefaultBindings());
      lookup_[key0] = index1;
      lookup_[key1] = index0;
    }
  }

  // Iterates over all the data in the table, invokes the function `fn`. The
  // arguments passed into the function are specified by the columns T...
  // The data is passed individually as parameters, not as as Row/RefTuple.
  template <typename... T, typename Fn>
  void ForEach(const Fn& fn) {
    const std::size_t last_page = (lookup_.size() / page_capacity_);
    const std::size_t last_page_count = lookup_.size() % page_capacity_;
    for (std::size_t page = 0; page <= last_page; ++page) {
      const std::size_t num =
          (page == last_page) ? last_page_count : page_capacity_;
      ForEachInner(fn, num, GetColumnPageData<T>(page)...);
    }
  }

 private:

  // A C++ iterator type that is used to support iteration over the DataTable.
  // The Map and Row types are templated so we can use the same implementation
  // for both (non-const) iterator and const_iterator.
  template <typename RowT, typename MapT>
  class RowIterator {
    using SelfT = RowIterator<RowT, MapT>;

   public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = RowT;
    using reference = RowT&;
    using pointer = RowT*;
    using difference_type = std::ptrdiff_t;

    reference operator*() { return row_; }
    const reference operator*() const { return row_; }

    pointer operator->() { return &row_; }
    const pointer operator->() const { return &row_; }

    SelfT& operator++() {
      ++index_;
      if (map_ && index_ < map_->Size()) {
        row_ = map_->At(index_);
      } else {
        row_ = RowT(nullptr);
      }
      return *this;
    }

    SelfT operator++(int) {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    friend bool operator==(const SelfT& a, const SelfT& b) {
      return a.map_ == b.map_ && a.index_ == b.index_;
    }

    friend bool operator!=(const SelfT& a, const SelfT& b) {
      return !(a == b);
    }

   private:
    friend class DataTable<Key, Fields...>;

    RowIterator(MapT* map, std::size_t index)
        : map_(map), index_(index), row_(nullptr) {
      if (map_ && index_ < map_->Size()) {
        row_ = map_->At(index_);
      }
    }

    MapT* map_;
    std::size_t index_;
    RowT row_;
  };

 public:
  // Standard C++ iterator types for range-based for loops.
  using iterator = RowIterator<Row, DataTable<Key, Fields...>>;
  using const_iterator = RowIterator<ConstRow, const DataTable<Key, Fields...>>;

  // Standard C++ begin/end implementations to support range-based for loops.
  iterator begin() { return iterator(this, 0); }
  const const_iterator begin() const { return const_iterator(this, 0); }
  const const_iterator cbegin() const { return const_iterator(this, 0); }
  iterator end() { return iterator(this, lookup_.size()); }
  const const_iterator end() const {
    return const_iterator(this, lookup_.size());
  }
  const const_iterator cend() const {
    return const_iterator(this, lookup_.size());
  }

 private:
  template <typename... T>
  using BindingsFor =
      std::index_sequence<IndexOfElementWithin<T, Key, Fields...>::value...>;

  // Each column of data is effectively a vector-of-vectors. The "index" of
  // a given element is therefore comprised of an index for each vector.
  struct Index {
    std::size_t page = 0;
    std::size_t element = 0;
  };

  // A specialized vector-of-vector container used for each column of data.
  template <typename T>
  class Column {
   public:
    using Type = typename T::Type;

    Column() = default;

    void Clear() { pages_.clear(); }

    template <typename Arg>
    void Add(std::size_t page_capacity, Arg&& arg) {
      if (pages_.empty() || pages_.back().size() == page_capacity) {
        pages_.emplace_back().reserve(page_capacity);
      }
      if constexpr (kIsBool) {
        const bool value = arg;
        pages_.back().emplace_back(static_cast<std::byte>(value));
      } else {
        pages_.back().emplace_back(std::forward<Arg>(arg));
      }
    }

    // Swaps the element at `index` with the back element, and pop the back.
    void SwapAndPop(const Index& index) {
      CHECK(!pages_.empty() && !pages_.back().empty());

      // We don't have to swap if `index` refers to the last element.
      const bool skip_swap = (index.page == pages_.size() - 1 &&
                              index.element == pages_.back().size() - 1);
      if (!skip_swap) {
        auto& a = pages_[index.page][index.element];
        auto& b = pages_.back().back();
        using std::swap;
        swap(a, b);
      }

      pages_.back().pop_back();
      if (pages_.back().empty()) {
        pages_.pop_back();
      }
    }

    void Swap(const Index& index0, const Index& index1) {
      auto& a = pages_[index0.page][index0.element];
      auto& b = pages_[index1.page][index1.element];
      using std::swap;
      swap(a, b);
    }

    Type* GetPageData(std::size_t n) {
      if constexpr (kIsBool) {
        return reinterpret_cast<Type*>(pages_[n].data());
      } else {
        return pages_[n].data();
      }
    }

    const Type* GetPageData(std::size_t n) const {
      if constexpr (kIsBool) {
        return reinterpret_cast<const Type*>(pages_[n].data());
      } else {
        return pages_[n].data();
      }
    }

    Type& operator[](Index index) {
      return GetPageData(index.page)[index.element];
    }

    const Type& operator[](const Index& index) const {
      return GetPageData(index.page)[index.element];
    }

   private:
    // Avoid the vector<bool> specialization by using vector<byte> instead.
    // We need to do this because we can't return references to the booleans
    // inside a vector<bool>.
    static_assert(sizeof(bool) == sizeof(std::byte));
    static constexpr bool kIsBool = std::is_same_v<Type, bool>;
    using StorageType = std::conditional_t<kIsBool, std::byte, Type>;

    std::vector<std::vector<StorageType>> pages_;
  };

  Index GetIndex(std::size_t n) const {
    return {n / page_capacity_, n % page_capacity_};
  }

  template <std::size_t N>
  auto& GetColumn() {
    return std::get<N>(cols_[N]);
  }

  template <std::size_t N>
  const auto& GetColumn() const {
    return std::get<N>(cols_[N]);
  }

  const auto& GetKey(Index index) const { return GetColumn<0>()[index]; }

  template <std::size_t N>
  auto& GetData(Index index) {
    if constexpr (N == 0) {
      // Always ensure that access to the key column is const.
      return std::as_const(GetColumn<N>()[index]);
    } else {
      return GetColumn<N>()[index];
    }
  }

  template <std::size_t N>
  const auto& GetData(Index index) const {
    return GetColumn<N>()[index];
  }

  template <typename Field>
  auto GetColumnPageData(std::size_t n) const {
    constexpr std::size_t N =
        IndexOfElementWithin<Field, Key, Fields...>::value;
    return GetColumn<N>().GetPageData(n);
  }

  template <std::size_t... N>
  void InitImpl(std::index_sequence<N...>) {
    // Initializes each column variant to be of the correct type. For example,
    // cols_[0] will be initialized to Column<KeyType>.
    (cols_[N].template emplace<N>(), ...);
  }

  template <std::size_t... N>
  void ClearImpl(std::index_sequence<N...>) {
    (GetColumn<N>().Clear(), ...);
    lookup_.clear();
  }

  template <typename... Args, std::size_t... N>
  Row EmplaceImpl(std::index_sequence<N...>, Args&&... args) {
    (GetColumn<N>().Add(page_capacity_, std::forward<Args>(args)), ...);

    const Index index = GetIndex(lookup_.size());
    const KeyType& key = GetKey(index);
    lookup_.emplace(key, index);

    return Row(GetData<N>(index)...);
  }

  template <std::size_t... N>
  void SwapAndPopImpl(Index index, std::index_sequence<N...>) {
    (GetColumn<N>().SwapAndPop(index), ...);
  }

  template <std::size_t... N>
  auto FindImpl(const KeyType& key, std::index_sequence<N...> seq) {
    using TupleType = RefTuple<decltype(seq), const Key, Fields...>;
    auto iter = lookup_.find(key);
    return iter == lookup_.end() ? TupleType(nullptr)
                                 : TupleType(GetData<N>(iter->second)...);
  }

  template <std::size_t... N>
  auto FindImpl(const KeyType& key, std::index_sequence<N...> seq) const {
    using TupleType = RefTuple<decltype(seq), const Key, const Fields...>;
    auto iter = lookup_.find(key);
    return iter == lookup_.end() ? TupleType(nullptr)
                                 : TupleType(GetData<N>(iter->second)...);
  }

  template <std::size_t... N>
  auto GetImpl(const Index& index, std::index_sequence<N...> seq) {
    using TupleType = RefTuple<decltype(seq), const Key, Fields...>;
    return TupleType(GetData<N>(index)...);
  }

  template <std::size_t... N>
  auto GetImpl(const Index& index, std::index_sequence<N...> seq) const {
    using TupleType = RefTuple<decltype(seq), const Key, const Fields...>;
    return TupleType(GetData<N>(index)...);
  }

  template <std::size_t... N>
  void SwapImpl(const Index& index0, const Index& index1,
                std::index_sequence<N...> seq) {
    (GetColumn<N>().Swap(index0, index1), ...);
  }

  template <typename Fn, typename... T>
  void ForEachInner(const Fn& fn, std::size_t n, T&&... ptrs) {
    for (std::size_t i = 0; i < n; ++i) {
      fn(ptrs[i]...);
    }
  }

  // Defines a std::variant<> type that can represent any of the data columns.
  // The n-th data column maps to the n-th variant type.
  using ColumnVariant = std::variant<Column<Key>, Column<Fields>...>;

  static constexpr std::size_t kNumColumns = std::variant_size_v<ColumnVariant>;

  ColumnVariant cols_[kNumColumns];
  absl::flat_hash_map<KeyType, Index> lookup_;
  std::size_t page_capacity_ = 32;
};

}  // namespace redux

#endif  // REDUX_MODULES_BASE_DATA_TABLE_H_
