/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#include <limits>

#include "lullaby/util/logging.h"
#include "lullaby/util/time.h"

#ifndef LULLABY_ASSET_LOADER_LOG_TIMES
#define LULLABY_ASSET_LOADER_LOG_TIMES 0
#endif

namespace lull {

AssetLoader::LoadRequest::LoadRequest(const std::string& filename,
                                      const AssetPtr& asset)
    : asset(asset), filename(filename) {
  asset->SetFilename(filename);
}

AssetLoader::AssetLoader(LoadFileFn load_fn)
    : load_fn_(load_fn), pending_requests_(0) {
  if (!load_fn) {
    LOG(DFATAL) << "Must provide a load callback.";
  }
}

AssetLoader::~AssetLoader() {
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
  load_fn_(req->filename.c_str(), &req->data);
#if LULLABY_ASSET_LOADER_LOG_TIMES
  {
    const auto dt = MillisecondsFromDuration(load_timer.GetElapsedTime());
    LOG(INFO) << "[" << dt << "] " << req->filename << " LoadFn: " << mode;
  }
#endif

#if LULLABY_ASSET_LOADER_LOG_TIMES
  Timer on_load_timer;
#endif
  // Notify the Asset of the loaded data.
  req->asset->OnLoad(req->filename, &req->data);
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
  req->asset->OnFinalize(req->filename, &req->data);
#if LULLABY_ASSET_LOADER_LOG_TIMES
  const auto dt = MillisecondsFromDuration(timer.GetElapsedTime());
  LOG(INFO) << "[" << dt << "] " << req->filename << " OnFinalize: " << mode;
#endif
}

void AssetLoader::SetLoadFunction(LoadFileFn load_fn) {
  load_fn_ = load_fn;
}

const AssetLoader::LoadFileFn AssetLoader::GetLoadFunction() {
  return load_fn_;
}

void AssetLoader::StartAsyncLoads() { processor_.Start(); }

void AssetLoader::StopAsyncLoads() { processor_.Stop(); }

}  // namespace lull
