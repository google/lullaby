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

#ifndef REDUX_MODULES_BASE_ASSET_LOADER_H_
#define REDUX_MODULES_BASE_ASSET_LOADER_H_

#include <algorithm>
#include <functional>
#include <future>
#include <memory>
#include <string>

#include "absl/status/statusor.h"
#include "redux/modules/base/async_processor.h"
#include "redux/modules/base/data_container.h"
#include "redux/modules/base/data_reader.h"
#include "redux/modules/base/registry.h"

namespace redux {

// The AssetLoader is used for retrieving binary data associated with a URI.
//
// You can use the AssetLoader to either Open or Load binary data. Open
// operations return a DataReader that you can use to access the binary data
// in a streaming-like manner. Load operations will copy the entire contents
// of the binary data into a DataContainer.
//
// Loading can be performed either Immediately or Asynchronously. Immediate
// operations will perform the entire loading process on the calling thread.
// Asynchronous operations will perform the loading using an AsyncProcessor and
// callbacks are used to manage the asset during this process.
class AssetLoader {
 public:
  explicit AssetLoader(Registry* registry);
  ~AssetLoader();

  AssetLoader(const AssetLoader& rhs) = delete;
  AssetLoader& operator=(const AssetLoader& rhs) = delete;

  void OnRegistryInitialize();

  // DataContainer returned by Load operations.
  using StatusOrData = absl::StatusOr<DataContainer>;

  // DatReader returned by Open operations.
  using StatusOrReader = absl::StatusOr<DataReader>;

  // Data and Reader callbacks used for load or open operations, respectively.
  using DataCallback = std::function<void(StatusOrData&)>;
  using ReaderCallback = std::function<void(StatusOrReader&)>;

  // Loads the asset at the given `uri` into a DataContainer. Blocks the calling
  // thread until the operation is done.
  StatusOrData LoadNow(std::string_view uri);

  // Opens the asset at the given `uri` into a DataReader. Blocks the calling
  // thread until the operation is done.
  StatusOrReader OpenNow(std::string_view uri);

  // Loads the asset at the given `uri` into a DataContainer. The `on_load`
  // callback will be executed on a worker thread after the data is loaded.
  // The `on_finalize` callback will be called during Finalize(), allowing
  // the caller to know when the asset is ready to use.
  std::future<StatusOrData> LoadAsync(std::string_view uri,
                                      DataCallback on_load,
                                      DataCallback on_finalize);

  // Opens the asset at the given `uri` into a DataContainer. The `on_open`
  // callback will be executed on a worker thread after the data is loaded.
  // The `on_finalize` callback will be called during Finalize(), allowing
  // the caller to know when the asset is ready to use.
  std::future<StatusOrReader> OpenAsync(std::string_view uri,
                                        ReaderCallback on_open,
                                        ReaderCallback on_finalize);

  // Runs the `on_finalize` callbacks for all assets that have finished their
  // async open/load operations. Will limit the number of finalizers called if
  // `max_num_assets_to_finalize` is specified. Returns the number of async
  // operations still pending.
  int FinalizeAll();
  int Finalize(int max_num_assets_to_finalize);

  using OpenFn = std::function<StatusOrReader(std::string_view uri)>;

  // Sets the function that will be used to open assets.
  void SetOpenFunction(OpenFn open_fn);

  // Returns the function set in `SetOpenFunction`.
  OpenFn GetOpenFunction() const;

  // Returns the default open function.
  static OpenFn GetDefaultOpenFunction(Registry* registry = nullptr);

  // Starts opening and loading assets asynchronously. This is done
  // automatically on construction and only needs to be called explicitly after
  // StopAsyncOperations.
  void StartAsyncOperations();

  // Stops opening and loading assets asynchronously. Blocks until the currently
  // loading asset have completed. Call StartAsyncOperations to resume async
  // operations.
  void StopAsyncOperations();

 private:
  using OpenFnPtr = std::shared_ptr<OpenFn>;

  class RequestBase {
   public:
    virtual ~RequestBase() = default;
    virtual void DoAsyncOp() = 0;
    virtual void DoFinalize() = 0;
  };
  using RequestPtr = std::shared_ptr<RequestBase>;

  template <typename T>
  class Request : public RequestBase {
   public:
    using StatusOrT = absl::StatusOr<T>;
    using CallbackFn = std::function<void(StatusOrT&)>;

    Request(std::string_view uri, OpenFnPtr open_fn, CallbackFn async_op)
        : uri_(uri),
          open_fn_(std::move(open_fn)),
          async_op_(std::move(async_op)) {}

    std::future<StatusOrT> PackageFinalizer(RequestPtr ptr,
                                            CallbackFn on_finalize);

    void DoAsyncOp() override;

    void DoFinalize() override;

   private:
    std::string uri_;
    StatusOrT result_;
    OpenFnPtr open_fn_;
    CallbackFn async_op_;
    std::packaged_task<StatusOrT()> finalize_task_;
  };

  void ScheduleRequest(RequestPtr request);

  Registry* registry_ = nullptr;
  AsyncProcessor<RequestPtr> processor_;
  OpenFnPtr open_fn_ = nullptr;
  int pending_requests_ = 0;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::AssetLoader);

#endif  // REDUX_MODULES_BASE_ASSET_LOADER_H_
