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

#ifndef REDUX_MODULES_BASE_REGISTRY_H_
#define REDUX_MODULES_BASE_REGISTRY_H_

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/synchronization/mutex.h"
#include "redux/modules/base/dependency_graph.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/base/typeid.h"

namespace redux {

// A map of TypeId to objects of any type registered with the TypeId system.
//
// This class can be used to simplify dependency injection. Rather than passing
// multiple pointers to various objects to the constructor of a class, a pointer
// to a registry can be used and the individual object pointers can be extracted
// from the registry.
//
// The Registry is the sole owner of the objects created/registered with it.
// It provides a raw-pointer to the object when requested. The Registry will
// destroy all objects (in reverse order of creation/registration) when it
// itself is destroyed.
//
// All operations on the Registry are thread-safe.
class Registry {
 public:
  Registry() = default;
  ~Registry();

  Registry(const Registry&) = delete;
  Registry& operator=(const Registry&) = delete;

  // Verifies all dependencies have been registered and invokes any
  // OnRegistryInitialize member functions on any registered object.
  void Initialize();

  // Creates an object of type |T| and registers it, using |Args| as its
  // constructor arguments. Returns a raw pointer to the newly created object.
  template <typename T, typename... Args>
  T* Create(Args&&... args) {
    T* ptr = new T(std::forward<Args>(args)...);
    Register(std::unique_ptr<T>(ptr));
    return ptr;
  }

  // Registers an object of type |T| so that it can be looked up in the
  // Registry. This can handle automatically casting between unique_ptrs of
  // certain different T types, which the next method "Register<T, D>()" cannot.
  // E.g.:
  //   class ChildClass : public BaseClass {};
  //   std::unique_ptr<ChildClass> ptr = ...;
  //   registry->Register<BaseClass>(std::move(ptr));
  template <typename T>
  void Register(std::unique_ptr<T> obj) {
    CHECK(obj != nullptr);
    RegisterImpl(std::shared_ptr<T>(std::move(obj)));
  }

  // Also accept unique_ptr with custom deleters.
  template <typename T, typename D>
  void Register(std::unique_ptr<T, D> obj) {
    CHECK(obj != nullptr);
    RegisterImpl(std::shared_ptr<T>(std::move(obj)));
  }

  // Gets a raw pointer to the object instance of type |T| or return null if
  // it has not been registered.
  template <typename T>
  T* Get() {
    absl::MutexLock lock(&mutex_);
    auto iter = table_.find(GetTypeId<T>());
    if (iter == table_.end()) {
      return nullptr;
    }
    return static_cast<T*>(iter->second);
  }

  // Gets a raw pointer to the object instance of type |T| or return null if
  // it has not been registered.
  template <typename T>
  const T* Get() const {
    absl::MutexLock lock(&mutex_);
    auto iter = table_.find(GetTypeId<T>());
    if (iter == table_.end()) {
      return nullptr;
    }
    return static_cast<const T*>(iter->second);
  }

  // Register a dependency of the type of |self| on another type.
  // Example usage: RegisterDependency<OtherType>(this);
  // or: RegisterDependency<OtherType, Type>();
  template <typename T, typename S>
  void RegisterDependency(S* dep = nullptr, bool init_dependency = false) {
    const TypeInfo dependent(GetTypeId<S>(), GetTypeName<S>());
    const TypeInfo dependency(GetTypeId<T>(), GetTypeName<T>());
    registered_dependencies_.emplace_back(dependent, dependency);
    if (init_dependency) {
      initialization_dependencies_.AddDependency(dependent.type,
                                                 dependency.type);
    }
  }

 private:
  using Pointer = std::shared_ptr<void>;
  using TypedPointer = std::pair<TypeId, Pointer>;
  using ObjectList = std::vector<TypedPointer>;
  // Store a raw pointer in the ObjectTable so that the lifetime of the object
  // can be more explicitly controlled (and for slightly better performance).
  using ObjectTable = absl::flat_hash_map<TypeId, void*>;
  using Initializer = std::function<void()>;

  struct TypeInfo {
    TypeInfo(TypeId t, const char* n) : type(t), name(n) {}
    TypeId type;
    const char* name;
  };

  struct DependencyInfo {
    DependencyInfo(TypeInfo dependent, TypeInfo dependency)
        : dependent_type(dependent), dependency_target_type(dependency) {}
    TypeInfo dependent_type;
    TypeInfo dependency_target_type;
  };

  // Helper to determines if |T| has a member function with the signature:
  //  void T::OnRegistryInitialize().
  template <typename T>
  struct IsInitializableImpl {
    template <typename U>
    static typename std::is_same<
        void, decltype(std::declval<U>().OnRegistryInitialize())>::type
    Test(U*);

    template <typename U>
    static std::false_type Test(...);

    using type = decltype(Test<T>(nullptr));
  };

  template <typename T>
  using IsInitializable = typename IsInitializableImpl<T>::type;

  template <typename T>
  void RegisterImpl(std::shared_ptr<T> shared) {
    const TypeId type = GetTypeId<T>();

    // Casting a shared_ptr down to a void shared_ptr preserves the shared_ptrs
    // ability to correctly delete the object.
    Pointer ptr(std::static_pointer_cast<void>(shared));

    absl::MutexLock lock(&mutex_);
    CHECK(!table_.contains(type))
        << "Object of type " << GetTypeName<T>() << " already registered.";
    table_.emplace(type, ptr.get());
    objects_.emplace_back(type, ptr);
    satisfied_dependencies_.insert(type);

    if constexpr (IsInitializable<T>::type::value) {
      initializers_[type] = [obj = shared.get()]() {
        obj->OnRegistryInitialize();
      };
    }
  }

  // Mutex for protecting all operations.
  mutable absl::Mutex mutex_;
  // List of Objects in creation order to enable destruction in reverse order.
  ObjectList objects_;
  // Map of Objects and their TypeIds for lookup.
  ObjectTable table_;
  // List of dependencies between types.
  std::vector<DependencyInfo> registered_dependencies_;
  // Set of satisfied dependencies types.
  absl::flat_hash_set<TypeId> satisfied_dependencies_;
  // List of initialize functions.
  absl::flat_hash_map<TypeId, Initializer> initializers_;
  // List of dependencies for initialization;
  DependencyGraph<TypeId> initialization_dependencies_;
};

}  // namespace redux

#endif  // REDUX_MODULES_BASE_REGISTRY_H_
