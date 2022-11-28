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

#ifndef REDUX_MODULES_ECS_BLUEPRINT_H_
#define REDUX_MODULES_ECS_BLUEPRINT_H_

#include <memory>
#include <string>
#include <utility>

#include "redux/modules/base/logging.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/var/var_array.h"

namespace redux {

// Blueprints are a special type of datafile that are used for supporting
// data-driven Entity descriptions.  (See modules/datafile for more information
// about datafiles).
//
// A blueprint file can be written thusly:
//
// {
//    #import: ...,
//    redux::TransformDef: {
//      ..
//    },
//    redux::RenderDef: {
//      ..
//    },
//    redux::PhysicsDef: {
//      ..
//    },
// }
//
// The BlueprintFactory can be used to parse the above into a Blueprint object.
// Internally, the Blueprint stores the data in a VarArray. This array contains
// a VarTable for each component in the file in the order it appears.
//
// Blueprints are "unevaluated"; they store the data in Vars and ScriptValue
// expressions. The EntityFactory will "resolve" them into a concrete and valid
// set of C++ objects that will be passed into the various Systems.

class Blueprint {
 public:
  Blueprint() = default;
  Blueprint(std::string name, VarArray components)
      : name_(std::move(name)), components_(std::move(components)) {}

  // Returns the name of this blueprint. This is often the uri from where this
  // blueprint was loaded.
  const std::string& GetName() const { return name_; }

  // Returns the number of components specified in this blueprint.
  size_t GetNumComponents() const { return components_.Count(); }

  // Returns the component at the given `index`.
  const Var& GetComponent(size_t index) const { return components_.At(index); }

  // Returns the TypeId of the component at the given `index`.
  TypeId GetComponentType(size_t index) const {
    const Var& component = components_.At(index);
    const HashValue type = component[ConstHash("$type")].ValueOr(HashValue());
    return TypeId(type.get());
  }

 private:
  std::string name_;
  VarArray components_;
};

using BlueprintPtr = std::shared_ptr<Blueprint>;

}  // namespace redux

#endif  // REDUX_MODULES_ECS_BLUEPRINT_H_
