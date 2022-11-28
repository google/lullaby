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

#include "redux/modules/base/asset_loader.h"

#ifdef __ANDROID__
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>
#endif

#include <limits>

#include "redux/modules/base/choreographer.h"
#include "redux/modules/base/data_builder.h"
#include "redux/modules/base/logging.h"

namespace redux {

#ifdef __ANDROID__
static DataReader DataReaderFromAndroidAsset(AAsset* asset) {
  auto handler = [asset](DataReader::Operation op, int64_t num,
                         void* buffer) -> mutable std::size_t {
    switch (op) {
      case DataReader::Operation::kRead: {
        auto result = AAsset_read(asset, buffer, num);
        CHECK_GT(result, 0);
        return result;
      }
      case DataReader::Operation::kSeek: {
        auto result = AAsset_seek(asset, num, SEEK_CUR);
        CHECK_GT(result, 0);
        return result;
      }
      case DataReader::Operation::kSeekFromHead: {
        auto result = AAsset_seek(asset, num, SEEK_SET);
        CHECK_GT(result, 0);
        return result;
      }
      case DataReader::Operation::kSeekFromEnd: {
        auto result = AAsset_seek(asset, num, SEEK_END);
        CHECK_GT(result, 0);
        return result;
      }
      case DataReader::Operation::kClose: {
        AAsset_close(asset);
        asset = nullptr;
        return 0;
      }
      default: {
        LOG(FATAL) << "Unknown operation: " << op;
      }
    }
  };
  return DataReader(handler);
}

static AAsset* TryOpenAndroidAsset(Registry* registry, const std::string uri) {
  if (registry == null) {
    return DataReader(nullptr);
  }
  auto* android_context = registry->Get<AndroidContext>();
  if (android_context == nullptr) {
    return DataReader(nullptr);
  }
  AAssetManager* asset_manager = android_context->GetAndroidAssetManager();
  if (asset_manager == nullptr) {
    return DataReader(nullptr);
  }
  return AAssetManager_open(asset_manager, uri.c_str(), AASSET_MODE_STREAMING);
}
#endif  // __ANDROID__

static absl::StatusOr<DataReader> OpenStream(Registry* registry,
                                             std::string_view uri) {
  CHECK(!uri.empty()) << "Must specify URI.";
  std::string filename(uri);

#ifdef __ANDROID__
  if (filename[0] != '\\') {
    AAsset* asset = TryOpenAndroidAsset(registry, filename);
    if (asset != nullptr) {
      return DataReaderFromAndroidAsset(asset);
    }
  }
#endif

  FILE* file = fopen(filename.c_str(), "rb");
  if (!file) {
    return absl::NotFoundError(absl::StrCat("Unable to open file", filename));
  }

  return DataReader::FromCFile(file);
}

static absl::StatusOr<DataContainer> ReadAll(DataReader& reader) {
  const size_t length = reader.GetTotalLength();
  CHECK_GT(length, 0);
  CHECK_EQ(reader.GetCurrentPosition(), 0);

  DataBuilder builder(length);
  std::byte* ptr = builder.GetAppendPtr(length);
  reader.Read(ptr, length);
  return builder.Release();
}

template <typename T>
void AssetLoader::Request<T>::DoAsyncOp() {
  StatusOrReader reader = (*open_fn_)(uri_);

  if constexpr (std::is_same_v<T, DataReader>) {
    result_ = std::move(reader);
  } else if constexpr (std::is_same_v<T, DataContainer>) {
    if (reader.ok()) {
      result_ = ReadAll(reader.value());
    } else {
      result_ = reader.status();
    }
  }

  if (async_op_) {
    async_op_(result_);
  }
}

template <typename T>
void AssetLoader::Request<T>::DoFinalize() {
  CHECK(finalize_task_.valid());
  finalize_task_();
}

template <typename T>
auto AssetLoader::Request<T>::PackageFinalizer(RequestPtr ptr,
                                               CallbackFn on_finalize)
    -> std::future<StatusOrT> {
  finalize_task_ = std::packaged_task<StatusOrT()>([=]() mutable {
    if (on_finalize) {
      on_finalize(result_);
    }

    StatusOrT result =  std::move(result_);
    // There's a cyclical reference created by having the packed_task in the
    // RequestPtr, so we need to clear it up here.
    ptr.reset();
    return result;
  });
  return finalize_task_.get_future();
}

AssetLoader::AssetLoader(Registry* registry) : registry_(registry) {
  SetOpenFunction(nullptr);
}

AssetLoader::~AssetLoader() {
  StopAsyncOperations();
  FinalizeAll();
}

void AssetLoader::OnRegistryInitialize() {
  registry_->Get<Choreographer>()->Add<&AssetLoader::FinalizeAll>(
      Choreographer::Stage::kPrologue);
}

void AssetLoader::SetOpenFunction(OpenFn open_fn) {
  if (open_fn) {
    open_fn_ = std::make_shared<OpenFn>(std::move(open_fn));
  } else {
    open_fn_ = std::make_shared<OpenFn>(GetDefaultOpenFunction(registry_));
  }
}

AssetLoader::OpenFn AssetLoader::GetOpenFunction() const { return *open_fn_; }

AssetLoader::OpenFn AssetLoader::GetDefaultOpenFunction(Registry* registry) {
  return [=](std::string_view uri) {
    return OpenStream(registry, uri);
  };
}

void AssetLoader::StartAsyncOperations() { processor_.Start(); }

void AssetLoader::StopAsyncOperations() { processor_.Stop(); }

auto AssetLoader::OpenNow(std::string_view uri) -> StatusOrReader {
  return (*open_fn_)(uri);
}

auto AssetLoader::LoadNow(std::string_view uri) -> StatusOrData {
  StatusOrReader reader = (*open_fn_)(uri);
  if (!reader.ok()) {
    return reader.status();
  }
  return ReadAll(reader.value());
}

auto AssetLoader::OpenAsync(std::string_view uri, ReaderCallback on_open,
                            ReaderCallback on_finalize)
    -> std::future<StatusOrReader> {
  using RequestT = Request<DataReader>;
  auto request = std::make_shared<RequestT>(uri, open_fn_, std::move(on_open));

  if (!processor_.IsRunning()) {
    request->DoAsyncOp();
  } else {
    ScheduleRequest(request);
  }

  auto future = request->PackageFinalizer(request, std::move(on_finalize));
  if (!processor_.IsRunning()) {
    request->DoFinalize();
  }
  return future;
}

auto AssetLoader::LoadAsync(std::string_view uri, DataCallback on_load,
                            DataCallback on_finalize)
    -> std::future<StatusOrData> {
  using RequestT = Request<DataContainer>;
  auto request = std::make_shared<RequestT>(uri, open_fn_, std::move(on_load));
  if (!processor_.IsRunning()) {
    request->DoAsyncOp();
  } else {
    ScheduleRequest(request);
  }
  auto future = request->PackageFinalizer(request, std::move(on_finalize));
  if (!processor_.IsRunning()) {
    request->DoFinalize();
  }
  return future;
}

void AssetLoader::ScheduleRequest(RequestPtr req) {
  ++pending_requests_;
  processor_.Enqueue(req, [](RequestPtr* req) { (*req)->DoAsyncOp(); });
}

int AssetLoader::FinalizeAll() {
  return Finalize(std::numeric_limits<int>::max());
}

int AssetLoader::Finalize(int max_num_assets_to_finalize) {
  while (max_num_assets_to_finalize > 0) {
    RequestPtr req = nullptr;
    if (!processor_.Dequeue(&req)) {
      break;
    }
    req->DoFinalize();
    --pending_requests_;
    --max_num_assets_to_finalize;
  }
  return pending_requests_;
}
}  // namespace redux
