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

#ifndef REDUX_MODULES_ECS_BLUEPRINT_FACTORY_H_
#define REDUX_MODULES_ECS_BLUEPRINT_FACTORY_H_

#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "redux/modules/base/registry.h"
#include "redux/modules/base/resource_manager.h"
#include "redux/modules/base/serialize.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/ecs/blueprint.h"
#include "redux/modules/var/var.h"

namespace redux {

// Responsible for creating Blueprint objects from string representations.
// See blueprint.h for more information.
class BlueprintFactory {
 public:
  explicit BlueprintFactory(Registry* registry);

  // Loads the datafile at the specified `uri` and parses it into a Blueprint.
  BlueprintPtr LoadBlueprint(std::string_view uri);

  // Parses the given `text` into a Blueprint.
  BlueprintPtr ReadBlueprint(std::string_view text);

 private:
  BlueprintPtr ParseBlueprintDatafile(std::string name, std::string_view text);

  Registry* registry_ = nullptr;
  ResourceManager<Blueprint> blueprints_;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::BlueprintFactory);

#endif  // REDUX_MODULES_ECS_BLUEPRINT_FACTORY_H_
