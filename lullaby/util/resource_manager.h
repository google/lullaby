/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_BASE_RESOURCE_MANAGER_H_
#define LULLABY_BASE_RESOURCE_MANAGER_H_

#include <functional>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>

#include "lullaby/util/hash.h"

namespace lull {

// The ResourceManager can be used to create and manage objects.  Objects
// managed by this class are all associated with a HashValue key.  Creating
// objects with the ResourceManager will either:
// - Create a new object instance using the CreateFn functor and map it to the
//   provided key.
// - Return a previously created object associated with the key.
//
// The ResourceManager can be configured to cache objects a few ways:
//
// 1. kCacheFullyOnCreate: Created objects will be owned by the ResourceManager
//      as well anyone with a reference to the object. In other words, even if
//      all external references to the object are invalidated, the object
//      remains "alive" until it is removed from the ResourceManager.
//
// 2. kWeakCachingOnly: Created objects will not be owned by the
//      ResourceManager. Instead, the ResourceManager will only store a weak-
//      reference to the created object. This will allow the ResourceManager
//      to return an existing object (if it is still alive) without taking
//      ownership of the object.
//
// 3. kCacheExplicitly: The ResourceManager will not cache anything by default,
//      and it is up to clients to call Register on objects explicitly.
//
// By default, the ResourceManager uses kCacheFullyOnCreate for legacy reasons,
// however kWeakCachingOnly is recommended.
//
// Internally, the ResourceManager stores both a strong-reference and a weak-
// reference to managed objects. The Release() function will remove the strong-
// reference to the object, but will maintain the weak-reference. As such, it
// is possible to still "leak" some memory even after releasing all references.
// The Erase() function will remove all references to the object. And Reset()
// will erase all object references entirely.
template <typename T>
class ResourceManager {
 public:
  class ResourceGroupStub;
  using ResourceGroup = ResourceGroupStub*;

  // Managed objects are reference counted using a shared_ptr.
  using ObjectPtr = std::shared_ptr<T>;

  // The different ways caching can be configured.
  enum CacheMode {
    kCacheFullyOnCreate,
    kWeakCachingOnly,
    kCacheExplicitly,
  };

  ResourceManager() {}
  explicit ResourceManager(CacheMode mode) : mode_(mode) {}

  ResourceManager(const ResourceManager& rhs) = delete;
  ResourceManager& operator=(const ResourceManager& rhs) = delete;

  // Returns an object associated with the |key|.  If an object is already
  // associated with the |key|, returns the cached object.  Otherwise, a new
  // object will be created using the |create| functor and it will be associated
  // with the |key| (unless the caching mode is set to kCacheExplicitly).
  using CreateFn = std::function<ObjectPtr()>;
  ObjectPtr Create(HashValue key, const CreateFn& create);

  // Associates the |obj| with |key|, overwriting any previous associations.
  void Register(HashValue key, const ObjectPtr& obj);

  // Returns the object associated with the |key|, or nullptr if no object is
  // found.
  ObjectPtr Find(HashValue key) const;

  // Releases the specified object from the internal cache, but possibly
  // maintains a weak-reference to the object.
  void Release(HashValue key);

  // Erases all references to the object from the internal cache.
  void Erase(HashValue key);

  // Releases all the objects from the internal cache.
  void Reset();

  // Creates and attaches a new ResourceGroup.  All resource allocations from
  // now on will be associated with this group.
  void PushNewResourceGroup();

  // Removes all the resources associated with a ResourceGroup from the cache.
  void ReleaseResourceGroup(ResourceGroup group);

  // Detaches a ResourceGroup so new allocations are no longer associated with
  // that ResourceGroup and returns a reference.
  ResourceGroup PopResourceGroup();

 private:
  using WeakPtr = std::weak_ptr<T>;
  using KeyList = std::list<HashValue>;

  // Internal structure for the object cache that stores both the shared_ptr and
  // weak_ptr to the object. The weak_ptr allows the ResourceManager to safely
  // reuse a previously created object even if it has been "released" as long
  // as that object is alive.
  struct ObjectCacheEntry {
    ObjectPtr strong_ref;
    WeakPtr weak_ref;
  };

  CacheMode mode_ = kCacheFullyOnCreate;
  std::unordered_map<HashValue, ObjectCacheEntry> objects_;

  // std::list allows us to use the address of the contained object safely.
  std::list<KeyList> attached_groups_;
  std::list<KeyList> detached_groups_;
};

template <typename T>
typename ResourceManager<T>::ObjectPtr ResourceManager<T>::Create(
    HashValue key, const CreateFn& create) {
  ObjectPtr obj = nullptr;
  auto iter = objects_.find(key);
  if (iter != objects_.end()) {
    // Acquire the object from the handle in case it has been released but is
    // still valid.
    obj = iter->second.weak_ref.lock();
  }

  if (!obj) {
    if (create) {
      obj = create();
    }
  }

  if (obj && mode_ != kCacheExplicitly) {
    ObjectCacheEntry entry;
    entry.weak_ref = obj;
    if (mode_ == kCacheFullyOnCreate) {
      entry.strong_ref = obj;
    }
    if (iter != objects_.end()) {
      // If the cached shared_ptr was released, reacquire it.
      if (iter->second.strong_ref == nullptr) {
        iter->second = std::move(entry);
      }
    } else {
      objects_.emplace(key, std::move(entry));
    }
    if (!attached_groups_.empty()) {
      attached_groups_.front().push_front(key);
    }
  }
  return obj;
}

template <typename T>
void ResourceManager<T>::Register(HashValue key, const ObjectPtr& obj) {
  ObjectCacheEntry entry;
  entry.strong_ref = obj;
  entry.weak_ref = obj;

  auto iter = objects_.find(key);
  if (iter != objects_.end()) {
    iter->second = std::move(entry);
  } else {
    objects_.emplace(key, std::move(entry));
  }
}

template <typename T>
auto ResourceManager<T>::Find(HashValue key) const -> ObjectPtr {
  auto iter = objects_.find(key);
  if (iter != objects_.end()) {
    // Acquire the object from the weak_ref in case it has been released from
    // the cache but is still actually alive.
    return iter->second.weak_ref.lock();
  } else {
    return nullptr;
  }
}

template <typename T>
void ResourceManager<T>::Release(HashValue key) {
  auto iter = objects_.find(key);
  if (iter != objects_.end()) {
    iter->second.strong_ref.reset();
  }
}

template <typename T>
void ResourceManager<T>::Erase(HashValue key) {
  objects_.erase(key);
}

template <typename T>
void ResourceManager<T>::Reset() {
  objects_.clear();
}

template <typename T>
void ResourceManager<T>::PushNewResourceGroup() {
  attached_groups_.emplace_front();
}

template <typename T>
void ResourceManager<T>::ReleaseResourceGroup(ResourceGroup group) {
  auto impl = reinterpret_cast<KeyList*>(group);
  auto it =
      std::find_if(detached_groups_.begin(), detached_groups_.end(),
                   [impl](const KeyList& query) { return &query == impl; });
  if (it != detached_groups_.end()) {
    for (HashValue key : *it) {
      Erase(key);
    }
    detached_groups_.erase(it);
  }
}

template <typename T>
typename ResourceManager<T>::ResourceGroup
ResourceManager<T>::PopResourceGroup() {
  detached_groups_.emplace_front(std::move(attached_groups_.front()));
  attached_groups_.pop_front();
  return reinterpret_cast<ResourceGroup>(&detached_groups_.front());
}

}  // namespace lull

#endif  // LULLABY_BASE_RESOURCE_MANAGER_H_
