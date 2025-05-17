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

#ifndef REDUX_SYSTEMS_MODEL_USD_PLUGIN_H_
#define REDUX_SYSTEMS_MODEL_USD_PLUGIN_H_

#include <memory>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/synchronization/mutex.h"
#include "redux/modules/base/asset_loader.h"
#include "redux/modules/base/data_container.h"
#include "redux/modules/base/registry.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/defaultResolver.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/timestamp.h"

namespace redux {

// A pxr::ArResolver that uses the redux::AssetLoader for loading assets.
class UsdAssetResolver : public pxr::ArDefaultResolver {
 public:
  void BindRegistry(Registry* registry);

  // Explicitly registers an asset with the internal cache. A subsequent call to
  // _OpenAsset with this path will return the given data (clearing out the
  // internal caching.)
  void RegisterAsset(const std::string& path,
                     std::shared_ptr<DataContainer> data);

 protected:
  std::shared_ptr<pxr::ArAsset> _OpenAsset(
      const pxr::ArResolvedPath& resolvedPath) const override;

  pxr::ArResolvedPath _Resolve(const std::string& path) const override;

  pxr::ArResolvedPath _ResolveForNewAsset(
      const std::string& path) const override;

  pxr::ArTimestamp _GetModificationTimestamp(
      const std::string& path,
      const pxr::ArResolvedPath& resolvedPath) const override;

 private:
  Registry* registry_;
  AssetLoader* asset_loader_;
  mutable absl::Mutex mutex_;
  mutable absl::flat_hash_map<std::string, std::shared_ptr<DataContainer>>
      asset_cache_;
};

UsdAssetResolver* GetGlobalUsdAssetResolver();

}  // namespace redux

#endif  // REDUX_SYSTEMS_MODEL_USD_PLUGIN_H_
