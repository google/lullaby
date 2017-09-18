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

#ifndef LULLABY_MODULES_FILE_ASSET_H_
#define LULLABY_MODULES_FILE_ASSET_H_

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>

#include "lullaby/util/typeid.h"

namespace lull {

// The Asset class is to be used as a base class for any resource that is to
// loaded with the AssetLoader/AssetManager.  The base-class simply provides
// an interface that allows to the derived resource to "hook" into the asset
// loading process.  The process is a three-phase process:
// 1) The binary data is loaded off disk (or other location).
// 2) The Asset::OnLoad() function is called with the raw binary data.  This
//    function may be called on a worker thread internal to the AssetLoader.  It
//    can optionally perform preprocessing on the loaded data (e.g.
//    decompression) and return the updated data.
// 3) The Asset::OnFinalize() function is called with the post-processed data
//    (as determined by the OnLoad() function).  The thread on which this
//    function is specified explicitly when using the AssetLoader to ensure th
//    loaded data can be used in a thread-safe manner.
class Asset {
 public:
  Asset() {}

  virtual ~Asset() {}

  // Callback function that can be used to store the filename associated with
  // the asset if it is useful to the derived class.
  virtual void SetFilename(const std::string& filename) {}

  // This function is called when the asset is done loading off disk.  |data|
  // contains the raw binary data that was loaded.  It can be updated to contain
  // post-processed data (e.g. decompressed data) that will then be available
  // during finalizing.
  //
  // For asynchronous loads, this function is called by the AssetLoaders worker
  // thread.  Otherwise, it is called by the thread that initiated the load.
  virtual void OnLoad(const std::string& filename, std::string* data) {}

  // This function is called when the asset is ready to be finalized with the
  // specified |data|.  The contents of |data| will be freed after this call
  // is returned.  If the asset requires the data to persist, it should
  // std::move() the data into a local buffer.
  //
  // For asynchronous loads, this function is called on the thread on which
  // calls AssetLoader::Finalize.  Otherwise, it is called by the thread that
  // initiated the load.
  virtual void OnFinalize(const std::string& filename, std::string* data) {}

 private:
  Asset(const Asset& rhs) = delete;
  Asset& operator=(const Asset& rhs) = delete;
};

// Asset type that simply holds the loaded data directly with no additional
// processing.
class SimpleAsset : public Asset {
 public:
  void OnFinalize(const std::string& filename, std::string* data) override {
    data_ = std::move(*data);
  }

  size_t GetSize() const { return data_.length(); }
  const void* GetData() const { return data_.data(); }
  const std::string& GetStringData() const { return data_; }
  std::string ReleaseData() { return std::move(data_); }

 private:
  std::string data_;
};

typedef std::shared_ptr<Asset> AssetPtr;

}  // namespace lull

#endif  // LULLABY_MODULES_FILE_ASSET_H_
