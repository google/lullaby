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

#ifndef REDUX_MODULES_BASE_STATIC_REGISTRY_H_
#define REDUX_MODULES_BASE_STATIC_REGISTRY_H_

#include "redux/modules/base/registry.h"

namespace redux {

// A mechanism that allows for objects to be created with the Registry at
// static initialization time.
//
// This allows users to automatically create registry objects by simply linking
// the libraries without having to modify any code.
//
// To enable this functionality, libraries must first define a factory function
// with the signature void(Registry*) which will create objects using the
// Registry. Then, they must declare a single static variable in their library
// like:
//   static StaticRegistry g_my_static_variable(MyCreateFn);
//
// On the binary side, users can simply call StaticRegistry::Create() which will
// invoke all the factory functions that have been registered with this system.
class StaticRegistry {
 public:
  // Adds this function to the list of global factory functions to be called.
  // This must be declared as a static variable!
  using CreateFn = void (Registry*);
  explicit StaticRegistry(CreateFn fn);

  // Calls all the factory functions that were previously created.
  static void Create(Registry* registry);

 private:
  CreateFn* fn_ = nullptr;
  StaticRegistry* next_ = nullptr;
};

}  // namespace redux

#endif  // REDUX_MODULES_BASE_STATIC_REGISTRY_H_
