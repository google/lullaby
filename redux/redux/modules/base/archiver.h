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

#ifndef REDUX_MODULES_BASE_ARCHIVER_H_
#define REDUX_MODULES_BASE_ARCHIVER_H_

#include <type_traits>

#include "redux/modules/base/hash.h"

namespace redux::detail {

// Wraps a Serializer class with functionality that allows it to inspect/visit
// the member variables of objects being serialized.
//
// The Archiver is not meant to be used directly. Instead, you should call the
// free function:
//
//   template <typename Serializer, typename Value>
//   redux::Serialize(Serializer* serializer, Value* value, HashValue key);
//
// which will wrap the Serializer in the Archiver and start the serialization
// process.
//
// An example will help demonstrate its usage. Given the following classes:
//
//   struct BaseClass {
//     int base_value;
//     template <typename Archive>
//     void Serialize(Archive archive) {
//       archive(&base_value, ConstHash("base_value"));
//     }
//   };
//
//   struct ChildClass : BaseClass {
//     int child_value;
//     template <typename Archive>
//     void Serialize(Archive archive) {
//       BaseClass::Serialize(archive);
//       archive(&child_value, ConstHash("child_value"));
//     }
//   };
//
//   struct CompositeClass {
//     ChildClass child1;
//     ChildClass child2;
//     std::string value;
//     template <typename Archive>
//     void Serialize(Archive archive) {
//       archive(&child1, ConstHash("child1"));
//       archive(&child2, ConstHash("child2"));
//       archive(&value, ConstHash("value"));
//     }
//   };
//
// The following code snippet:
//   Serializer s;
//   CompositeClass cc;
//   Serialize(&s, &cc, ConstHash("cc"));
//
// Will be the equivalent of the following function calls:
//   s.Begin(Hash("cc"));
//   s.Begin(Hash("child1"));
//   s(&cc.child1.base_value, ConstHash("base_value"));
//   s(&cc.child1.child_value, ConstHash("child_value"));
//   s.End();
//   s.Begin(Hash("child2"));
//   s(&cc.child2.base_value, ConstHash("base_value"));
//   s(&cc.child2.child_value, ConstHash("child_value"));
//   s.End();
//   s(&cc.value, ConstHash("value"));
//   s.End();
//
// The Archiver is responsible for "navigating" the Serialize functions for
// the values being serialized and delegating the appropriate calls to the
// Serializer. See serialize.h for more information.

// A helper class used to determine if T has a Serialize function.
template <typename T, typename Archive>
struct IsSerializableImpl {
  template <typename U>
  static typename std::is_same<void, decltype(std::declval<U>().Serialize(
                                         std::declval<Archive>()))>::type
  Test(U*);

  template <typename U>
  static std::false_type Test(...);

  using type = decltype(Test<T>(nullptr));
};

// IsSerializable<T, Archive>: does T have a member function with the signature:
//     void T::Serialize(Archive);
template <typename T, typename Archive>
using IsSerializableT = typename IsSerializableImpl<T, Archive>::type;
template <typename T, typename Archive>
inline constexpr bool IsSerializable = IsSerializableT<T, Archive>::value;

template <typename Serializer>
class Archiver {
  using Self = Archiver<Serializer>;

  // Helper to determines if |T| has a member function with the signature:
  //  void T::Begin(HashValue).
  template <typename T>
  struct IsScopedImpl {
    template <typename U>
    static typename std::is_same<void, decltype(std::declval<U>().Begin(
                                           std::declval<HashValue>()))>::type
    Test(U*);

    template <typename U>
    static std::false_type Test(...);

    using type = decltype(Test<T>(nullptr));
  };

 public:
  explicit Archiver(Serializer* serializer) : serializer_(serializer) {}

  template <typename T>
  void operator()(T& value, HashValue key) {
    constexpr bool kScoped = IsScopedImpl<Serializer>::type::value;
    constexpr bool kSerializable = IsSerializable<T, Self>;

    if constexpr (kSerializable) {
      if constexpr (kScoped) {
        serializer_->Begin(key);
      }
      value.Serialize(*this);
      if constexpr (kScoped) {
        serializer_->End();
      }
    } else {
      (*serializer_)(value, key);
    }
  }

  // Returns whether or not the wrapped Serializer is destructive (ie. will
  // overwrite the values in the objects being serialized).
  bool IsDestructive() const { return serializer_->IsDestructive(); }

 private:
  Serializer* serializer_;
};

}  // namespace redux::detail

#endif  // REDUX_MODULES_BASE_ARCHIVER_H_
