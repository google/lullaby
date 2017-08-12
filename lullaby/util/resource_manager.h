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

#ifndef LULLABY_UTIL_RESOURCE_MANAGER_H_
#define LULLABY_UTIL_RESOURCE_MANAGER_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "lullaby/util/hash.h"

namespace lull {

// The ResourceManager can be used to create and manage objects.  Objects
// managed by this class are all associated with a HashValue key.  Creating
// objects with the Resourcemanager will either:
// - Create a new object instance using the CreateFn functor and map it to the
//   provided key.
// - Return a previously created object associated with the key.
//
// The shared reference to an object can be removed from the internal cache
// by calling Release() with the object's key.  If all external references to
// the object are reset as well, then calling Create with the key will create a
// new object instance.  However, if the object is still "alive" externally
// (ie. there are still active shared_ptr references to the object) then the
// internal cache will reacquire the object instance when calling Create instead
// of creating a new instance.
template <typename T>
class ResourceManager {
 public:
  // Resource are reference counted using a shared_ptr.
  using ObjectPtr = std::shared_ptr<T>;

  // Client-provided function for creating an object instance.
  using CreateFn = std::function<ObjectPtr()>;

  ResourceManager() {}

  // Returns an object associated with the |key|.  If an object is already
  // associated with the |key|, return the cached object.  Otherwise, a new
  // object will be created using the |create| functor and associated with the
  // |key|.
  ObjectPtr Create(HashValue key, CreateFn create);

  // Find the object associated with the |key|.  Returns nullptr if no object is
  // found.
  ObjectPtr Find(HashValue key) const;

  // Release the specified object from the internal cache.
  void Release(HashValue key);

  // Release all the objects from the internal cache.
  void Reset();

 private:
  using WeakPtr = std::weak_ptr<T>;

  // Internal structure for the object cache.  It stores both the shared_ptr and
  // weak_ptr to the object.  When the object is first created, both the
  // shared_ptr and weak_ptr are set to the object.  The Release function resets
  // the shared_ptr but leaves the weak_ptr alone.  The weak_ptr allows the
  // ResourceManager to safely recache a previously created object even if it
  // has been released by the ResourceManager as long as that object is alive.
  struct ObjectCacheEntry {
    explicit ObjectCacheEntry(ObjectPtr ptr) : object(ptr), handle(ptr) {}
    ObjectPtr object;  // Strong reference to object.
    WeakPtr handle;    // Weak reference to object.
  };

  using ObjectMap = std::unordered_map<HashValue, ObjectCacheEntry>;
  ObjectMap objects_;  // Object cache.

  ResourceManager(const ResourceManager& rhs) = delete;
  ResourceManager& operator=(const ResourceManager& rhs) = delete;
};

template <typename T>
typename ResourceManager<T>::ObjectPtr ResourceManager<T>::Create(
    HashValue key, CreateFn create) {
  ObjectPtr obj = nullptr;
  auto iter = objects_.find(key);
  if (iter != objects_.end()) {
    // Acquire the object from the handle in case it has been released but is
    // still valid.
    obj = iter->second.handle.lock();
  }

  if (!obj) {
    if (create) {
      obj = create();
    }
  }

  if (obj) {
    if (iter != objects_.end()) {
      // If the cached shared_ptr was released, reacquire it.
      if (iter->second.object == nullptr) {
        iter->second = ObjectCacheEntry(obj);
      }
    } else {
      objects_.emplace(key, ObjectCacheEntry(obj));
    }
  }
  return obj;
}

template <typename T>
typename ResourceManager<T>::ObjectPtr ResourceManager<T>::Find(
    HashValue key) const {
  ObjectPtr obj = nullptr;
  auto iter = objects_.find(key);
  if (iter != objects_.end()) {
    // Acquire the object from the handle in case it has been released but is
    // still valid.
    obj = iter->second.handle.lock();
  }
  return obj;
}

template <typename T>
void ResourceManager<T>::Release(HashValue key) {
  auto iter = objects_.find(key);
  if (iter != objects_.end()) {
    iter->second.object.reset();
  }
}

template <typename T>
void ResourceManager<T>::Reset() {
  objects_.clear();
}

}  // namespace lull

#endif  // LULLABY_UTIL_RESOURCE_MANAGER_H_
