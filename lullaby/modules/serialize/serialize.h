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

#ifndef LULLABY_MODULES_SERIALIZE_SERIALIZE_H_
#define LULLABY_MODULES_SERIALIZE_SERIALIZE_H_

#include "lullaby/modules/serialize/serialize_traits.h"
#include "lullaby/util/hash.h"

namespace lull {

// Wraps a Serializer class with functionality that allows it to inspect/visit
// the member variables of objects being serialized.
//
// The Archiver is not meant to be used directly.  Instead, you should call the
// global function:
//
//   template <typename Serializer, typename Value>
//   lull::Serialize(Serializer* serializer, Value* value, HashValue key);
//
// which will wrap the Serializer in the Archiver and start the serialization
// process.
//
// An example will help demonstrate its usage.  Given the following classes:
//
//   struct BaseClass {
//     int base_value;
//     template <typename Archive>
//     void Serialize(Archive archive) {
//       archive(&base_value, Hash("base_value"));
//     }
//   };
//
//   struct ChildClass : BaseClass {
//     int child_value;
//     template <typename Archive>
//     void Serialize(Archive archive) {
//       BaseClass::Serialize(archive);
//       archive(&child_value, Hash("child_value"));
//     }
//   };
//
//   struct CompositeClass {
//     ChildClass child1;
//     ChildClass child2;
//     std::string value;
//     template <typename Archive>
//     void Serialize(Archive archive) {
//       archive(&child1, Hash("child1"));
//       archive(&child2, Hash("child2"));
//       archive(&value, Hash("value"));
//     }
//   };
//
// The following code snippet:
//   Serializer s;
//   CompositeClass cc;
//   Serialize(&s, &cc, Hash("cc"));
//
// Will be the equivalent of the following function calls:
//   s.Begin(Hash("cc"));
//   s.Begin(Hash("child1"));
//   s(&cc.child1.base_value, Hash("base_value"));
//   s(&cc.child1.child_value, Hash("child_value"));
//   s.End();
//   s.Begin(Hash("child2"));
//   s(&cc.child2.base_value, Hash("base_value"));
//   s(&cc.child2.child_value, Hash("child_value"));
//   s.End();
//   s(&cc.value, Hash("value"));
//   s.End();
//
// What's happening is that, if the Value instance being serialized has a member
// function with the signature:
//    template <typename Archive>
//    void Serialize(Archive archive);
//
// the Archiver will call that function, passing itself to it.  Otherwise, if
// the Value instance has no such function the Archiver will pass the Value
// instance directly to the Serializer.
//
// Additionally, the Archiver will also call Begin()/End() functions on a
// Serializer when visiting/serializing a new Value type that has a Serialize
// member function.  Note: the Begin()/End() functions for the Serializer are
// optional.  If they are not defined, the Archiver will simply forego calling
// these functions.
//
// It is also expected for the Serializer to provide a function:
//   bool IsDestructive() const;
//
// This allows objects that are being serialized to provide special handling
// depending on whether the serialization is a "save" operation (ie. the data
// in the object is being serialized to a wire format) or a "load" operation
// (ie. the data in the object will be overridden by the data from the
// Serializer.)
//
// Most importantly, the Serializer is expected to implement one or more
// functions with the signature:
//   template <typename T>
//   void operator()(T* ptr, HashValue key);
//
// This is the function that performs the actual serialization.  It is strongly
// recommended that specific overloads be implemented for this function to
// handle value types explicitly.  For example, while fundamental types (eg.
// bools, floats, ints, etc.) and even mathfu types (eg. vec3, quat, etc.) can
// be serialized by memcpy'ing there values, values-types like pointers and
// STL containers cannot.  As such, a purely generic operator() that can
// handle all types will likely result in errors.
template <typename Serializer>
class Archiver {
  // This type is used in determining if |U| has a member function with the
  // signature: void U::Serialize(Archive<Serializer> archive);
  template <typename U>
  using IsSerializable = detail::IsSerializable<U, Archiver<Serializer>>;

  // This type is used in determining if |U| has a member function with the
  // signature: void U::Begin(HashValue);
  template <typename U>
  using IsScopedSerializer = detail::IsScopedSerializer<U>;

 public:
  explicit Archiver(Serializer* serializer) : serializer_(serializer) {}

  // If |Value| has a Serialize member function, defer the serialization to it.
  template <typename Value>
  typename std::enable_if<IsSerializable<Value>::kValue, void>::type operator()(
      Value* value, HashValue key) {
    Begin(key);
    value->Serialize(*this);
    End();
  }

  // If |Value| does not have a Serialize function, allow the Serializer to
  // process the object directly.
  template <typename Value>
  typename std::enable_if<!IsSerializable<Value>::kValue, void>::type
  operator()(Value* value, HashValue key) {
    (*serializer_)(value, key);
  }

  // Returns whether or not the wrapped Serializer is destructive (ie. will
  // overwrite the values in the objects being serialized).
  bool IsDestructive() const { return serializer_->IsDestructive(); }

 private:
  // If |Serializer| has a Begin(HashValue) function, call it before a new
  // serializable Value type is being serialized.
  template <typename U = Serializer>
  typename std::enable_if<IsScopedSerializer<U>::kValue, void>::type Begin(
      HashValue key) {
    serializer_->Begin(key);
  }

  // If |Serializer| has a Begin(HashValue) function, call End() after a
  // serializable Value type has been serialized.
  template <typename U = Serializer>
  typename std::enable_if<IsScopedSerializer<U>::kValue, void>::type End() {
    serializer_->End();
  }

  // If |Serializer| does not have a Begin(HashValue) function, do nothing
  // before a new serializable Value type is being serialized.
  template <typename U = Serializer>
  typename std::enable_if<!IsScopedSerializer<U>::kValue, void>::type Begin(
      HashValue key) {}

  // If |Serializer| does not have a Begin(HashValue) function, call End() after
  // a serializable Value type has been serialized.
  template <typename U = Serializer>
  typename std::enable_if<!IsScopedSerializer<U>::kValue, void>::type End() {}

  Serializer* serializer_;  // The wrapped Serializer instance.
};

// Serializes the |value| with the |key| using the provided |serializer|.
template <typename Serializer, typename Value>
void Serialize(Serializer* serializer, Value* value, HashValue key) {
  Archiver<Serializer> archive(serializer);
  archive(value, key);
}

}  // namespace lull

#endif  // LULLABY_MODULES_SERIALIZE_SERIALIZE_H_
