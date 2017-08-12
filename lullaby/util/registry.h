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

#ifndef LULLABY_UTIL_REGISTRY_H_
#define LULLABY_UTIL_REGISTRY_H_

#include <assert.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "lullaby/util/dependency_checker.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/time.h"
#include "lullaby/util/typeid.h"

#ifndef LULLABY_REGISTRY_LOG_DESTRUCTION
#define LULLABY_REGISTRY_LOG_DESTRUCTION 0
#endif

namespace lull {

// A map of TypeId to objects of any type registered with the TypeId system.
//
// This class can be used to simplify dependency injection.  Rather than passing
// multiple pointers to various objects to the constructor of a class, a pointer
// to a registry can be used and the individual object pointers can be extracted
// from the registry.
//
// The Registry is the sole owner of the objects created/registered with it.
// It provides a raw-pointer to the object when requested.  The Registry will
// destroy all objects (in reverse order of creation/registration) when it
// itself is destroyed.
//
// All operations on the Registry are thread-safe.
class Registry {
 public:
  Registry() {}

  ~Registry() {
    // Destroy objects in reverse order of registration.
    for (auto iter = objects_.rbegin(); iter != objects_.rend(); ++iter) {
#if LULLABY_REGISTRY_LOG_DESTRUCTION
      Timer timer;
#endif
      // Destroy the object before removing it from the ObjectTable in case
      // the object is referenced in the registry in its destructor.
      iter->second.reset();
#if LULLABY_REGISTRY_LOG_DESTRUCTION
      {
        const auto dt = MillisecondsFromDuration(timer.GetElapsedTime());
        LOG(INFO) << "[" << dt << "] Registry Destroy: " << iter->first;
      }
#endif
      table_.erase(iter->first);
    }
    table_.clear();
    objects_.clear();
  }

  // Creates an object of type |T| and register it, using |Args| as its
  // constructor arguments.
  // Returns a raw pointer to the newly created object or null if an object
  // of type |T| is already registered.
  template <typename T, typename... Args>
  T* Create(Args&&... args) {
    if (Get<T>() != nullptr) {
      return nullptr;
    }

#if LULLABY_REGISTRY_LOG_DESTRUCTION
    Timer timer;
#endif
    T* ptr = new T(std::forward<Args>(args)...);
#if LULLABY_REGISTRY_LOG_DESTRUCTION
    {
      const auto dt = MillisecondsFromDuration(timer.GetElapsedTime());
      LOG(INFO) << "[" << dt << "] Registry Create " << GetTypeName<T>()
                << " (" << GetTypeId<T>() << ")";
    }
#endif
    Register(std::unique_ptr<T>(ptr));
    return ptr;
  }

  // Registers an object of type |T| so that it can be looked up in the
  // Registry.
  template <typename T>
  void Register(std::unique_ptr<T> obj) {
    assert(obj != nullptr);
    const TypeId type = GetTypeId<T>();

    std::shared_ptr<T> shared(std::move(obj));
    // Casting a shared_ptr down to a void shared_ptr preserves the shared_ptrs
    // ability to correctly delete the object.
    Pointer ptr(std::static_pointer_cast<void>(shared));

    std::unique_lock<std::mutex> lock(mutex_);
    table_.emplace(type, ptr.get());
    objects_.emplace_back(type, ptr);
    dependency_checker_.SatisfyDependency(type);
  }

  // Gets a raw pointer to the object instance of type |T| or return NULL if
  // it has not been registered.
  template<typename T>
  T* Get() {
    std::unique_lock<std::mutex> lock(mutex_);
    auto iter = table_.find(GetTypeId<T>());
    if (iter == table_.end()) {
      return nullptr;
    }
    return static_cast<T*>(iter->second);
  }

  // Gets a raw pointer to the object instance of type |T| or return NULL if
  // it has not been registered.
  template<typename T>
  const T* Get() const {
    std::unique_lock<std::mutex> lock(mutex_);
    auto iter = table_.find(GetTypeId<T>());
    if (iter == table_.end()) {
      return nullptr;
    }
    return static_cast<const T*>(iter->second);
  }

  // Registers that there is a dependency for |dependent_type| on
  // |dependency_type|. This allows types to declare that they need another type
  // to be in the registry.
  void RegisterDependency(TypeId dependent_type, const char* dependent_name,
                          TypeId dependency_type, const char* dependency_name) {
    dependency_checker_.RegisterDependency(dependent_type, dependent_name,
                                           dependency_type, dependency_name);
  }

  // Helper function to register a dependency of the type of |self| on another
  // type.
  // Example usage: RegisterDependency<OtherType>(this);
  template <typename T, typename S>
  void RegisterDependency(S* self) {
    RegisterDependency(GetTypeId<S>(), GetTypeName<S>(), GetTypeId<T>(),
                       GetTypeName<T>());
  }

  // Checks that all registered dependencies have been satisfied, logging DFATAL
  // if they are not.
  void CheckAllDependencies() { dependency_checker_.CheckAllDependencies(); }

 private:
  using Pointer = std::shared_ptr<void>;
  using TypedPointer = std::pair<TypeId, Pointer>;
  using ObjectList = std::vector<TypedPointer>;

  // Store a raw pointer in the ObjectTable so that the lifetime of the object
  // can be more explicitly controlled (and for slightly better performance).
  using ObjectTable = std::unordered_map<TypeId, void*>;

  mutable std::mutex mutex_;  // Mutex for protecting all operations.
  ObjectList objects_;  // List of Objects in order of creation that is used to
                        // destroy them in reverse order.
  ObjectTable table_;   // Map of Objects and their TypeIds for lookup.

  DependencyChecker dependency_checker_;  // Used to validate dependencies.

  Registry(const Registry&) = delete;
  Registry& operator=(const Registry&) = delete;
};

}  // namespace lull

#endif  // LULLABY_UTIL_REGISTRY_H_
