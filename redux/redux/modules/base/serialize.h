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

#ifndef REDUX_MODULES_BASE_SERIALIZE_H_
#define REDUX_MODULES_BASE_SERIALIZE_H_

#include "redux/modules/base/archiver.h"
#include "redux/modules/base/hash.h"

namespace redux {

// Serializes the |value| with the |key| using the provided |serializer|.
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
// A Serializer can be any object that provides the following API:
//
//   template <typename T>
//   void operator()(T* ptr, HashValue key);
//
// This is the function that performs the actual serialization. It is strongly
// recommended that specific overloads be implemented for this function to
// handle value types explicitly. For example, while fundamental types (eg.
// bools, floats, ints, etc.) and even math types (eg. vec3, quat, etc.) can
// be serialized by memcpy'ing there values, values-types like pointers and
// STL containers cannot. As such, a purely generic operator() that can
// handle all types will likely result in errors.
//
//   bool IsDestructive() const;
//
// This allows objects that are being serialized to provide special handling
// depending on whether the serialization is a "save" operation (ie. the data
// in the object is being serialized to a wire format) or a "load" operation
// (ie. the data in the object will be overridden by the data from the
// Serializer.)
//
//   void Begin(HashValue key);
//   void End();
//
// If implemented, the Begin()/End() functions on a Serializer when
// visiting/serializing a new Value type that has a Serialize member function.
// Note: the Begin()/End() functions for the Serializer are optional. If they
// are not defined, this functionality will be supressed.
template <typename Serializer, typename Value>
void Serialize(Serializer& serializer, Value& value, HashValue key = {}) {
  detail::Archiver<Serializer> archive(&serializer);
  archive(value, key);
}

}  // namespace redux

#endif  // REDUX_MODULES_BASE_SERIALIZE_H_
