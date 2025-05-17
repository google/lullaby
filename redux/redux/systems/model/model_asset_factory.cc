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

#include "redux/systems/model/model_asset_factory.h"

#include <string_view>
#include "absl/base/attributes.h"
#include "redux/modules/base/filepath.h"
#include "redux/modules/base/registry.h"
#include "redux/systems/model/model_asset.h"

namespace redux {

ABSL_CONST_INIT static ModelAssetFactory* global_model_asset_registry_ =
    nullptr;

ModelAssetFactory::ModelAssetFactory(const char* ext, CreateFn fn)
    : ext_(ext), fn_(fn) {
  next_ = global_model_asset_registry_;
  global_model_asset_registry_ = this;
}

ModelAssetPtr ModelAssetFactory::CreateModelAsset(Registry* registry,
                                                   std::string_view uri) {
  for (ModelAssetFactory* iter = global_model_asset_registry_; iter != nullptr;
       iter = iter->next_) {
    if (EndsWith(uri, iter->ext_)) {
      return iter->fn_(registry, uri);
    }
  }
  return nullptr;
}

}  // namespace redux
