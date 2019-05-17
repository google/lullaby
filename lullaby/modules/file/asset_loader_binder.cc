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

#include "lullaby/modules/file/asset_loader_binder.h"

#include "lullaby/modules/script/function_binder.h"
#include "lullaby/util/logging.h"

namespace lull {

AssetLoaderBinder::AssetLoaderBinder(Registry* registry) : registry_(registry) {
  auto* binder = registry->Get<FunctionBinder>();
  if (!binder) {
    LOG(DFATAL) << "No FunctionBinder.";
    return;
  }

  int (AssetLoader::*finalize)(int) = &lull::AssetLoader::Finalize;
  binder->RegisterMethod("lull.AssetLoader.Finalize", finalize);
}

AssetLoader* AssetLoaderBinder::CreateAssetLoader(Registry* registry) {
  registry->Create<AssetLoaderBinder>(registry);
  return registry->Create<AssetLoader>(registry);
}

AssetLoaderBinder::~AssetLoaderBinder() {
  auto* binder = registry_->Get<FunctionBinder>();
  if (!binder) {
    LOG(DFATAL) << "No FunctionBinder.";
    return;
  }

  binder->UnregisterFunction("lull.AssetLoader.Finalize");
}

}  // namespace lull
