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

#include "lullaby/modules/file/asset_loader.h"

#ifdef __ANDROID__
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>
#endif

#include <fstream>
#include <limits>

#ifdef __ANDROID__
#include "lullaby/util/android_context.h"
#endif
#include "lullaby/util/error.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/time.h"

#ifndef LULLABY_ASSET_LOADER_LOG_TIMES
#define LULLABY_ASSET_LOADER_LOG_TIMES 0
#endif

namespace lull {

static bool LoadFileDirect(const std::string& filename, std::string* dest) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    LOG(ERROR) << "Failed to open file " << filename;
    return false;
  }

  file.seekg(0, std::ios::end);
  const std::streamoff length = file.tellg();
  if (length < 0) {
    LOG(ERROR) << "Failed to get file size for " << filename;
    return false;
  }

  dest->resize(static_cast<size_t>(length));
  file.seekg(0, std::ios::beg);
  file.read(&(*dest)[0], dest->size());
  return file.good();
}

#ifdef __ANDROID__

static bool LoadFileUsingAAssetManager(AAssetManager* android_asset_manager,
                                       const std::string& filename,
                                       std::string* dest) {
  if (!android_asset_manager) {
    LOG(ERROR) << "Missing AAssetManager.";
    return false;
  }
  AAsset* asset = AAssetManager_open(android_asset_manager, filename.c_str(),
                                     AASSET_MODE_STREAMING);
  if (!asset) {
    LOG(ERROR) << "Failed to open asset " << filename;
    return false;
  }
  const off_t len = AAsset_getLength(asset);
  dest->resize(static_cast<size_t>(len));
  const int rlen = AAsset_read(asset, &(*dest)[0], len);
  AAsset_close(asset);
  return len == rlen && len > 0;
}

static bool LoadFileAndroid(Registry* registry, const std::string& filename,
                            std::string* dest) {
  AAssetManager* android_asset_manager = nullptr;
  auto* android_context = registry->Get<AndroidContext>();
  if (android_context) {
    android_asset_manager = android_context->GetAndroidAssetManager();
  }
  if (android_asset_manager && !filename.empty() && filename[0] != '\\') {
    const bool loaded =
        LoadFileUsingAAssetManager(android_asset_manager, filename, dest);
    if (loaded) {
      return loaded;
    }
  }

  return LoadFileDirect(filename, dest);
}

#endif  // __ANDROID__

AssetLoader::LoadRequest::LoadRequest(const std::string& filename,
                                      const AssetPtr& asset)
    : asset(asset), filename(filename), error(kErrorCode_Ok) {
  asset->SetFilename(filename);
}

AssetLoader::AssetLoader(Registry* registry) : registry_(registry) {
  SetLoadFunction(nullptr);
}

AssetLoader::AssetLoader(LoadFileFn load_fn) {
  SetLoadFunction(std::move(load_fn));
}

void AssetLoader::LoadImpl(const std::string& filename, const AssetPtr& asset,
                           LoadMode mode) {
  switch (mode) {
    case kImmediate: {
      LoadRequest req(filename, asset);
      DoLoad(&req, mode);
      DoFinalize(&req, mode);
      break;
    }
    case kAsynchronous: {
      LoadRequestPtr req(new LoadRequest(filename, asset));
      ++pending_requests_;
      processor_.Enqueue(
          req, [=](LoadRequestPtr* req) { DoLoad(req->get(), mode); });
      break;
    }
  }
}

int AssetLoader::Finalize() {
  return Finalize(std::numeric_limits<int>::max());
}

int AssetLoader::Finalize(int max_num_assets_to_finalize) {
  while (max_num_assets_to_finalize > 0) {
    LoadRequestPtr req = nullptr;
    if (!processor_.Dequeue(&req)) {
      break;
    }

    DoFinalize(req.get(), kAsynchronous);
    --pending_requests_;
    --max_num_assets_to_finalize;
  }
  return pending_requests_;
}

void AssetLoader::DoLoad(LoadRequest* req, LoadMode mode) const {
#if LULLABY_ASSET_LOADER_LOG_TIMES
  Timer load_timer;
#endif
  // Actually load the data using the provided load function.
  const bool success = load_fn_(req->filename.c_str(), &req->data);
#if LULLABY_ASSET_LOADER_LOG_TIMES
  {
    const auto dt = MillisecondsFromDuration(load_timer.GetElapsedTime());
    LOG(INFO) << "[" << dt << "] " << req->filename << " LoadFn: " << mode;
  }
#endif

  if (success == false) {
    // TODO: Change the LoadFileFn signature to allow for arbitrary
    // error codes.
    req->error = kErrorCode_NotFound;
    return;
  }

#if LULLABY_ASSET_LOADER_LOG_TIMES
  Timer on_load_timer;
#endif
  // Notify the Asset of the loaded data.
  req->error = req->asset->OnLoadWithError(req->filename, &req->data);
#if LULLABY_ASSET_LOADER_LOG_TIMES
  {
    const auto dt = MillisecondsFromDuration(on_load_timer.GetElapsedTime());
    LOG(INFO) << "[" << dt << "] " << req->filename << " OnLoad: " << mode;
  }
#endif
}

void AssetLoader::DoFinalize(LoadRequest* req, LoadMode mode) const {
#if LULLABY_ASSET_LOADER_LOG_TIMES
  Timer timer;
#endif

  // Notify the Asset to finalize the data on the finalizer thread.
  if (req->error == kErrorCode_Ok) {
    req->error = req->asset->OnFinalizeWithError(req->filename, &req->data);
  }
#if LULLABY_ASSET_LOADER_LOG_TIMES
  const auto dt = MillisecondsFromDuration(timer.GetElapsedTime());
  LOG(INFO) << "[" << dt << "] " << req->filename << " OnFinalize: " << mode;
#endif

  // Notify the Asset if an error occurred at any point during the load.
  if (req->error != kErrorCode_Ok) {
    req->asset->OnError(req->filename, req->error);
    if (error_fn_) {
      error_fn_(req->filename, req->error);
    }
  }
}

void AssetLoader::SetLoadFunction(LoadFileFn load_fn) {
  if (load_fn) {
    load_fn_ = std::move(load_fn);
  } else {
    load_fn_ = GetDefaultLoadFunction();
  }
}

AssetLoader::LoadFileFn AssetLoader::GetLoadFunction() const {
  return load_fn_;
}

AssetLoader::LoadFileFn AssetLoader::GetDefaultLoadFunction() const {
#ifdef __ANDROID__
  Registry* registry = registry_;
  if (registry) {
    return [registry](const std::string& filename, std::string* dest) {
      return LoadFileAndroid(registry, filename, dest);
    };
  }
#endif

  return LoadFileDirect;
}

void AssetLoader::SetOnErrorFunction(OnErrorFn error_fn) {
  error_fn_ = std::move(error_fn);
}

void AssetLoader::StartAsyncLoads() { processor_.Start(); }

void AssetLoader::StopAsyncLoads() { processor_.Stop(); }

}  // namespace lull
