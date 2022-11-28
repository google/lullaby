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

#ifndef REDUX_MODULES_BASE_REF_TUPLE_H_
#define REDUX_MODULES_BASE_REF_TUPLE_H_

#include <tuple>
#include <type_traits>
#include <utility>

#include "redux/modules/base/logging.h"

namespace redux {

// Provides the index of T within the list of elements. For example,
//
//     IndexOfElementWithin<float, int, bool, float, string>::value;
//
// has the value 2 since `float` is the second type in the list of types
// `int`, `bool`, `float`, and `string`.
template <typename T, typename... Elements>
struct IndexOfElementWithin;

namespace detail {

template <class T, class Tuple>
struct IndexOfElementWithinHelper;

template <class T, class... Elements>
struct IndexOfElementWithinHelper<T, std::tuple<T, Elements...>> {
  static const std::size_t value = 0;
};

template <class T, class U, class... Elements>
struct IndexOfElementWithinHelper<T, std::tuple<U, Elements...>> {
  static const std::size_t value =
      1 + IndexOfElementWithinHelper<T, std::tuple<Elements...>>::value;
};

}  // namespace detail

// Definition of IndexOfElementWithin.
template <typename T, typename... Elements>
struct IndexOfElementWithin
    : detail::IndexOfElementWithinHelper<T, std::tuple<Elements...>> {};

// RefTuple is similar to std::tuple<...>, but stores a pointer to each value
// rather than having the values stored directly in the object.
//
// This also allows it to act like a "sparse tuple" where not all values are
// set. The Bindings template parameter is an std::index_sequence that specifies
// which tuple elements have values.
//
// This class is intended to be used with the DataTable class. As a result, the
// `Fields` template argument are assumed to be of type DataColumn and the
// actual underlying type is defined as Field::Type.
template <typename Bindings, typename... Fields>
class RefTuple {
 public:
  // Initializes the RefTuple as an invalid value.
  explicit RefTuple(std::nullptr_t) : ok_(false) {}

  // Initializes the RefTuple with the given pointers.
  template <typename... Args>
  explicit RefTuple(Args&&... vals) : ok_(true) {
    Init(Bindings{}, vals...);
  }

  RefTuple& operator=(const RefTuple& rhs) = default;

  // Assigns the values from a scalar (in the case where we only have a single
  // value) or from a std::tuple.
  template <typename T>
  RefTuple& operator=(const T& value) {
    if constexpr (Bindings::size() == 1) {
      Nth<0>() = value;
    } else {
      using Mapping = std::make_index_sequence<Bindings::size()>;
      AssignFromTuple(Mapping{}, value);
    }
    return *this;
  }

  // Returns true if the RefTuple contains valid values.
  explicit operator bool() const { return ok_; }

  // Returns true if the Field element has a valid value.
  template <typename Field>
  bool Has() const {
    return std::get<FieldToIndex<Field>::value>(ptrs_) != nullptr;
  }

  // Returns a reference to the given Field.
  template <typename Field>
  auto& Get() {
    CHECK(ok_);
    return DoGetNth<FieldToIndex<Field>::value>();
  }

  template <typename Field>
  const auto& Get() const {
    CHECK(ok_);
    return DoGetNth<FieldToIndex<Field>::value>();
  }

  // Returns a reference to the N'th element. This is similar to std::get in
  // terms of functionality.
  template <std::size_t N>
  auto& Nth() {
    CHECK(ok_);
    return DoGetNth<MappedIndex<N>(Bindings{})>();
  }

  template <std::size_t N>
  const auto& Nth() const {
    CHECK(ok_);
    return DoGetNth<MappedIndex<N>(Bindings{})>();
  }

  // For situations where the RefTuple contains only a single element, we can
  // use pointer-like semantics for accessing the reference.
  auto& operator*() {
    static_assert(Bindings::size() == 1);
    CHECK(ok_);
    return Nth<0>();
  }

  const auto& operator*() const {
    static_assert(Bindings::size() == 1);
    CHECK(ok_);
    return Nth<0>();
  }

 private:
  template <std::size_t... N, typename... Args>
  void Init(std::index_sequence<N...>, Args&&... vals) {
    (DoSetNth<N>(&vals), ...);
  }

  template <std::size_t... N, typename... Args>
  void AssignFromTuple(std::index_sequence<N...>,
                       const std::tuple<Args...>& tuple) {
    ((Nth<N>() = std::get<N>(tuple)), ...);
  }

  template <std::size_t N, typename T>
  void DoSetNth(T* ptr) {
    CHECK(ptr);
    std::get<N>(ptrs_) = ptr;
  }

  template <std::size_t N>
  auto& DoGetNth() {
    auto ptr = std::get<N>(ptrs_);
    CHECK(ptr != nullptr) << N << "th element is null.";
    return *ptr;
  }

  template <std::size_t N>
  const auto& DoGetNth() const {
    auto ptr = std::get<N>(ptrs_);
    CHECK(ptr != nullptr) << N << "th element is null.";
    return *ptr;
  }

  template <std::size_t N, std::size_t... S>
  static constexpr std::size_t MappedIndex(std::index_sequence<S...>) {
    constexpr std::size_t kMapping[] = {S...};
    return kMapping[N];
  }

  template <typename T>
  using FieldToIndex = IndexOfElementWithin<typename std::decay_t<T>,
                                            typename std::decay_t<Fields>...>;

  // We want to store pointers to the underlying data values in a std::tuple.
  template <typename T>
  using FieldType =
      std::conditional<std::is_const<T>::value, const typename T::Type*,
                       typename T::Type*>;

  std::tuple<typename FieldType<Fields>::type...> ptrs_;
  bool ok_ = false;
};

// Provide unqualified get<N> functions to so that RefTuple can work with
// structured bindings.
template <std::size_t N, typename Bindings, typename... Fields>
auto& get(const RefTuple<Bindings, Fields...>& r) {
  return r.template Nth<N>();
}
template <std::size_t N, typename Bindings, typename... Fields>
auto& get(RefTuple<Bindings, Fields...>&& r) {
  return r.template Nth<N>();
}

}  // namespace redux

namespace std {

// Provide specializations of std::tuple_size and std::tuple_element so that
// RefTuple can work with structured bindings.

template <typename Bindings, typename... Args>
struct tuple_size<redux::RefTuple<Bindings, Args...>>
    : std::integral_constant<std::size_t, Bindings::size()> {};

template <std::size_t N, typename... Args>
struct tuple_element<N, redux::RefTuple<Args...>> {
  using type =
      decltype(std::declval<redux::RefTuple<Args...>>().template Nth<N>());
};

}  // namespace std

#endif  // REDUX_MODULES_BASE_REF_TUPLE_H_
