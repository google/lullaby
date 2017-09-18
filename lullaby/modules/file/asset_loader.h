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

#ifndef LULLABY_MODULES_FILE_ASSET_LOADER_H_
#define LULLABY_MODULES_FILE_ASSET_LOADER_H_

#include <algorithm>
#include <functional>
#include <memory>
#include <string>

#include "lullaby/modules/file/asset.h"
#include "lullaby/util/async_processor.h"
#include "lullaby/util/typeid.h"

namespace lull {

// The AssetLoader is used for loading Asset objects.  It provides two main
// mechanisms for loading:
// - Immediate/Blocking: the entire loading process is performed immediately on
//   the calling thread.
// - Asynchronous: the load is performed using an AsyncProcessor and callbacks
//   are used to manage the flow of the asset through the system.
//
// See asset.h for more details.
class AssetLoader {
 public:
  // The AssetLoader uses an external function to do the actual disk load
  // operation.  It is assumed that this function is thread-safe.
  // Note: The function signature is based on fplbase::LoadFile.
  using LoadFileFn = std::function<bool(const char* filename, std::string*)>;

  // Constructs the AssetLoader using the specified load function.
  explicit AssetLoader(LoadFileFn load_fn);

  AssetLoader(const AssetLoader& rhs) = delete;
  AssetLoader& operator=(const AssetLoader& rhs) = delete;

  ~AssetLoader();

  // Creates an Asset object of type |T| using the specified constructor |Args|
  // and load the data specified by |filename| into the asset.  This call blocks
  // the calling thread until the load is complete and finalized.
  template <typename T, typename... Args>
  std::shared_ptr<T> LoadNow(const std::string& filename, Args&&... args);

  // Creates an Asset object of type |T| using the specified constructor |Args|
  // and load the data specified by |filename| into the asset.  This call uses a
  // worker thread to perform the actual loading of the data after which the
  // Finalize() function can be called to finish the loading process.
  template <typename T, typename... Args>
  std::shared_ptr<T> LoadAsync(const std::string& filename, Args&&... args);

  // Finalizes any assets that were loaded asynchronously and are ready for
  // finalizing.  This function should be called on the thread on which it is
  // safe to Finalize the asset being loaded.  If |max_num_assets_to_finalize|
  // is specified, then this function will only attempt to finalize a limited
  // number of assets per call.
  // Returns: the number of async load operations still pending.
  int Finalize();
  int Finalize(int max_num_assets_to_finalize);

  // Sets a load function so that assets can be loaded from
  // different places using custom load functions.
  void SetLoadFunction(LoadFileFn load_fn);

  // Returns the load function set in |SetLoadFunction|.
  const LoadFileFn GetLoadFunction();

  // Starts loading assets asynchronously. This is done automatically on
  // construction and it only needs to be called explicitly after Stop.
  void StartAsyncLoads();

  // Stops loading assets asynchronously. Blocks until the currently loading
  // asset has completed. Call StartAsyncLoads to resume loading the assets.
  void StopAsyncLoads();

 private:
  // Flag indicating the type of load operation being performed.
  enum LoadMode {
    kImmediate,
    kAsynchronous,
  };

  // Internal structure to represent the load request.
  struct LoadRequest {
    LoadRequest(const std::string& filename, const AssetPtr& asset);
    AssetPtr asset;        // Asset object to load data into.
    std::string filename;  // Filename of data being loaded.
    std::string data;      // Actual data contents being loaded.
  };
  using LoadRequestPtr = std::shared_ptr<LoadRequest>;

  // Prepares a load request for the given asset.
  void LoadImpl(const std::string& filename, const AssetPtr& asset,
                LoadMode mode);

  // Performs the actual loading for both immediate and asynchronous requests.
  void DoLoad(LoadRequest* req, LoadMode mode) const;

  // Performs the "finalizing" for both immediate and asynchronous requests.
  void DoFinalize(LoadRequest* req, LoadMode mode) const;

  LoadFileFn load_fn_;  // Client-provided function for performing actual load.
  int pending_requests_;  // Number of requests queued for async loading.
  AsyncProcessor<LoadRequestPtr> processor_;  // Async processor for loading
                                              // data on a worker thread.
};

template <typename T, typename... Args>
std::shared_ptr<T> AssetLoader::LoadNow(const std::string& filename,
                                        Args&&... args) {
  auto ptr = std::make_shared<T>(std::forward<Args>(args)...);
  LoadImpl(filename, ptr, kImmediate);
  return ptr;
}

template <typename T, typename... Args>
std::shared_ptr<T> AssetLoader::LoadAsync(const std::string& filename,
                                          Args&&... args) {
  auto ptr = std::make_shared<T>(std::forward<Args>(args)...);
  LoadImpl(filename, ptr, kAsynchronous);
  return ptr;
}

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::AssetLoader);

#endif  // LULLABY_MODULES_FILE_ASSET_LOADER_H_
