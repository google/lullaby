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

#ifndef REDUX_SYSTEMS_MODEL_MODEL_ASSET_FACTORY_H_
#define REDUX_SYSTEMS_MODEL_MODEL_ASSET_FACTORY_H_

#include <string_view>

#include "redux/modules/base/registry.h"
#include "redux/systems/model/model_asset.h"

namespace redux {

// A statically generated linked-list that allows compile-time registration of
// ModelAsset instances.
//
// Users should use REDUX_REGISTER_MODEL_ASSET macro to register ModelAsset
// types, then can call ModelAssetFactory::CreateModelAsset to create a
// ModelAssetInstance for an asset at the given URI.
class ModelAssetFactory {
 public:
  // Constructor. Do not use directly, instead use REDUX_REGISTER_MODEL_ASSET.
  using CreateFn = ModelAssetPtr (*)(Registry* registry, std::string_view);
  ModelAssetFactory(const char* ext, CreateFn fn);

  // Creates a ModelAsset instance based on the extension of the URI.
  static ModelAssetPtr CreateModelAsset(Registry* registry,
                                        std::string_view uri);

 private:
  const char* ext_ = nullptr;
  CreateFn fn_ = nullptr;
  ModelAssetFactory* next_ = nullptr;
};

}  // namespace redux

// Associates an extension with a specific ModelAssetType. Example usage:
//   REDUX_REGISTER_MODEL_ASSET(rxmodel, redux::ReduxModelAsset);
//
// Note: extension should not be enclosed in quotes as it will also be used to
// generate the C++ object name to use for the static registry.
#define REDUX_REGISTER_MODEL_ASSET(Ext, Type)                  \
  redux::ModelAssetPtr create_##Ext(redux::Registry* registry, \
                                    std::string_view uri) {    \
    return redux::ModelAssetPtr(new Type(registry, uri));      \
  }                                                            \
  redux::ModelAssetFactory g_model_asset_##Ext("." #Ext, create_##Ext);

#endif  // REDUX_SYSTEMS_MODEL_MODEL_ASSET_FACTORY_H_
