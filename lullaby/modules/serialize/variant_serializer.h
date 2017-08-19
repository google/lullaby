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

#ifndef LULLABY_MODULES_SERIALIZE_VARIANT_SERIALIZER_H_
#define LULLABY_MODULES_SERIALIZE_VARIANT_SERIALIZER_H_

#include <stack>
#include <unordered_map>
#include <vector>

#include "lullaby/modules/serialize/serialize.h"
#include "lullaby/util/common_types.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/variant.h"

namespace lull {

// This file contains Serializers that can read/write to a VariantMap.
//
// VariantSerializers effectively manages a tree of Variants.  Leaf nodes
// are Variant objects to fundamental types (eg. ints, floats, bools, strings)
// while inner nodes are VariantMaps (or, sometimes, VaraintArrays).  This
// allows for the representation of complex composite data, similar to how
// JSON data is structured.


// Serializes data into a VariantMap.
class SaveToVariant {
 public:
  explicit SaveToVariant(VariantMap* variant) : root_(variant) {
    if (!variant) {
      LOG(DFATAL) << "Cannot save to empty variant!";
    }
  }

  // Adds a new "internal" node (ie. VariantMap) to be the current node to
  // which data will be serialized.  This node/map is associated with |key| on
  // the current map.
  void Begin(HashValue key) {
    if (stack_.empty()) {
      stack_.emplace(root_);
    } else {
      VariantMap* curr_map = stack_.top();
      auto iter = curr_map->emplace(key, VariantMap());
      VariantMap* next_map = iter.first->second.Get<VariantMap>();
      stack_.emplace(next_map);
    }
  }

  // Makes the top "internal" node/map the parent of the current node/map.
  void End() {
    if (!stack_.empty()) {
      stack_.pop();
    } else {
      LOG(DFATAL) << "Begin/End mismatch.";
    }
  }

  // Saves fundamental types as leaf-nodes on the current node/map.
  template <typename T>
  typename std::enable_if<detail::IsSerializeFundamental<T>::kValue, void>::type
  operator()(T* ptr, HashValue key) {
    Save(ptr, key);
  }

  // Saves strings as a leaf-node on the current node/map.
  void operator()(std::string* ptr, HashValue key) { Save(ptr, key); }

  // Converts the vector to a VariantArray and stores that array as a leaf-node
  // on the current node/map.
  template <typename T, typename... Args>
  void operator()(std::vector<T, Args...>* ptr, lull::HashValue key) {
    VariantArray arr;
    for (auto& t : *ptr) {
      arr.emplace_back(t);
    }
    Save(&arr, key);
  }

  // Converts the unordered_map to a VariantMap and stores that map as a
  // leaf-node on the current node/map.
  template <typename K, typename V, typename... Args>
  void operator()(std::unordered_map<K, V, Args...>* ptr, lull::HashValue key) {
    VariantMap map;
    for (auto& iter : *ptr) {
      map.emplace(iter.first, iter.second);
    }
    Save(&map, key);
  }

  // Errors on all other types.
  template <typename T>
  typename std::enable_if<!detail::IsSerializeFundamental<T>::kValue,
                          void>::type
  operator()(T* ptr, HashValue key) {
    LOG(DFATAL) << "Serialize not implemented for this type.";
  }

  bool IsDestructive() const { return false; }

 private:
  // Stores the object referenced by |ptr| in the "top" VariantMap with the
  // specified |key|.
  template <typename T>
  void Save(T* ptr, HashValue key) {
    if (!stack_.empty()) {
      VariantMap* map = stack_.top();
      map->emplace(key, Variant(*ptr));
    } else {
      LOG(DFATAL) << "No VariantMap in stack - cannot save key: " << key;
    }
  }

  // The root-level variant map.
  VariantMap* root_;

  // The stack of variant maps.
  std::stack<VariantMap*> stack_;
};

// Serializes data out of a VariantMap.
class LoadFromVariant {
 public:
  explicit LoadFromVariant(const VariantMap* variant) : root_(variant) {
    if (!variant) {
      LOG(DFATAL) << "Cannot load from empty variant!";
    }
  }

  // Adds a new "internal" node (ie. VariantMap) to be the current node to
  // which data will be serialized.  This node/map is associated with |key| on
  // the current map.
  void Begin(HashValue key) {
    if (stack_.empty()) {
      stack_.emplace(root_);
    } else {
      const VariantMap* curr_map = stack_.top();
      auto iter = curr_map->find(key);
      if (iter == curr_map->end()) {
        LOG(DFATAL) << "No such element with key " << key;
        return;
      }
      const VariantMap* next_map = iter->second.Get<VariantMap>();
      if (!next_map) {
        LOG(DFATAL) << "Expected a VariantMap at key " << key;
        return;
      }
      stack_.emplace(next_map);
    }
  }

  // Makes the top "internal" node/map the parent of the current node/map.
  void End() {
    if (!stack_.empty()) {
      stack_.pop();
    } else {
      LOG(DFATAL) << "Begin/End mismatch.";
    }
  }

  // Loads fundamental types from leaf-nodes on the current node/map.
  template <typename T>
  typename std::enable_if<detail::IsSerializeFundamental<T>::kValue, void>::type
  operator()(T* ptr, HashValue key) {
    Load(ptr, key);
  }

  // Loads strings from a leaf-node on the current node/map.
  void operator()(std::string* ptr, HashValue key) { Load(ptr, key); }

  // Loads a VariantArray from the current node/map and converts it to the
  // output vector.
  template <typename T, typename... Args>
  void operator()(std::vector<T, Args...>* ptr, lull::HashValue key) {
    VariantArray arr;
    Load(&arr, key);

    ptr->clear();
    for (auto& var : arr) {
      const T* value = var.Get<T>();
      if (value) {
        ptr->emplace_back(*value);
      } else {
        LOG(DFATAL) << "Type mismatch in VariantArray with key " << key;
      }
    }
  }

  // Loads a VariantMap from the current node/map and converts it to the
  // output unordered_map.
  template <typename K, typename V, typename... Args>
  void operator()(std::unordered_map<K, V, Args...>* ptr, lull::HashValue key) {
    VariantMap map;
    Load(&map, key);

    for (auto& iter : map) {
      const V* value = iter.second.Get<V>();
      if (value) {
        ptr->emplace(iter.first, std::move(*value));
      } else {
        LOG(DFATAL) << "Type mismatch in VariantMap with key " << key;
      }
    }
  }

  // Errors on all other types.
  template <typename T>
  typename std::enable_if<!detail::IsSerializeFundamental<T>::kValue,
                          void>::type
  operator()(T* ptr, HashValue key) {
    LOG(DFATAL) << "Serialize not implemented for this type.";
  }

  bool IsDestructive() const { return true; }

 private:
  // Copies the object stored in the "top" VariantMap with the specified |key|
  // into the |ptr|.
  template <typename T>
  void Load(T* ptr, HashValue key) {
    if (stack_.empty()) {
      return;
    }

    const VariantMap* map = stack_.top();
    const auto iter = map->find(key);
    if (iter == map->end()) {
      return;
    }
    const T* value = iter->second.Get<T>();
    if (value == nullptr) {
      return;
    }
    *ptr = *value;
  }

  // The root-level variant map.
  const VariantMap* root_;

  // The stack of variant maps.
  std::stack<const VariantMap*> stack_;
};

}  // namespace lull

#endif  // LULLABY_MODULES_SERIALIZE_VARIANT_SERIALIZER_H_
