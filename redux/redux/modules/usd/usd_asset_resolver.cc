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

#include "redux/modules/usd/usd_asset_resolver.h"

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "redux/modules/base/asset_loader.h"
#include "redux/modules/base/data_container.h"
#include "redux/modules/base/registry.h"
#include "pxr/pxr.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/timestamp.h"

namespace redux {

// A pxr::ArAsset that is a wrapper around a DataContainer object.
class DataContainerAsset : public pxr::ArAsset {
 public:
  DataContainerAsset(std::shared_ptr<DataContainer> data)
      : data_(std::move(data)) {}

  std::shared_ptr<const char> GetBuffer() const override {
    const char* bytes = reinterpret_cast<const char*>(data_->GetBytes());

    // We need to tie the lifetime of the data container to the returned
    // shared_ptr, so we capture it in the deleter callback.
    auto lock = data_;
    return std::shared_ptr<const char>(bytes, [=](const char* ptr) {
      CHECK_EQ(ptr, reinterpret_cast<const char*>(lock->GetBytes()));
    });
  }

  std::size_t GetSize() const override { return data_->GetNumBytes(); }

  std::size_t Read(void* buffer, std::size_t count,
                   std::size_t offset) const override {
    const std::size_t size = GetSize();
    if (offset >= size) {
      return 0;
    }
    if (count > size - offset) {
      count = size - offset;
    }
    std::memcpy(buffer, data_->GetBytes() + offset, count);
    return count;
  }

  std::pair<FILE*, std::size_t> GetFileUnsafe() const override {
    // Note: Some calling code will check for a nullptr FILE* and, if so, will
    // fallback on calling the Read function above instead.
    return std::make_pair(nullptr, 0);
  }

 private:
  std::shared_ptr<DataContainer> data_;
};

void UsdAssetResolver::BindRegistry(Registry* registry) {
  registry_ = registry;
  asset_loader_ = registry->Get<AssetLoader>();
}

std::shared_ptr<pxr::ArAsset> UsdAssetResolver::_OpenAsset(
    const pxr::ArResolvedPath& resolvedPath) const {
  const std::string& path = resolvedPath.GetPathString();
  std::shared_ptr<DataContainer> data;

  mutex_.Lock();
  auto it = asset_cache_.find(path);
  if (it != asset_cache_.end()) {
    data = it->second;
    asset_cache_.erase(it);
  }
  mutex_.Unlock();

  if (data == nullptr) {
    AssetLoader::StatusOrData asset = asset_loader_->LoadNow(path);
    CHECK(asset.ok()) << "Could not load asset: " << path;
    data = std::make_shared<DataContainer>(std::move(*asset));
  }

  DataContainerAsset* asset = new DataContainerAsset(std::move(data));
  return std::shared_ptr<pxr::ArAsset>(asset);
}

pxr::ArResolvedPath UsdAssetResolver::_Resolve(const std::string& path) const {
  return pxr::ArResolvedPath(path);
}

pxr::ArResolvedPath UsdAssetResolver::_ResolveForNewAsset(
    const std::string& path) const {
  return pxr::ArResolvedPath(path);
}

pxr::ArTimestamp UsdAssetResolver::_GetModificationTimestamp(
        const std::string& path,
        const pxr::ArResolvedPath& resolvedPath) const {
  return {};
}

void UsdAssetResolver::RegisterAsset(const std::string& path,
                                     std::shared_ptr<DataContainer> data) {
  mutex_.Lock();
  asset_cache_[path] = std::move(data);
  mutex_.Unlock();
}

UsdAssetResolver* GetGlobalUsdAssetResolver() {
  pxr::ArResolver& resolver = pxr::ArGetUnderlyingResolver();
  // Note: this is rather unsafe. We just hope that USD has correctly managed
  // to find our UsdAssetResolver to use as the underlying resolver. It would
  // be nice if we could verify this somehow.
  return static_cast<UsdAssetResolver*>(&resolver);
}
}  // namespace redux

PXR_NAMESPACE_OPEN_SCOPE
AR_DEFINE_RESOLVER(redux::UsdAssetResolver, pxr::ArResolver);
PXR_NAMESPACE_CLOSE_SCOPE
