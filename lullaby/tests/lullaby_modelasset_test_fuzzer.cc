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

#include "base/stringprintf.h"
#include "ion/base/logging.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/systems/model_asset/model_asset_system.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/string_view.h"
#include "lullaby/util/typeid.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::string test_data(reinterpret_cast<const char*>(data), size);
  lull::Registry registry;
  registry.Create<lull::AssetLoader>(
      [=](const char* name, std::string* out) -> bool {
        *out = test_data;
        return true;
      });
  auto* entity_factory = registry.Create<lull::EntityFactory>(&registry);

  auto* model_asset_system =
      entity_factory->CreateSystem<lull::ModelAssetSystem>();

  ion::base::ScopedDisableExitOnDFatal disable_exit_on_dfatal;
  model_asset_system->LoadModel("test-model");

  // Wait for pending loads to complete.
  auto* loader = registry.Get<lull::AssetLoader>();
  while (loader->Finalize() > 0) {
  }

  return 0;
}
