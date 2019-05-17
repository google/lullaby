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

#include <chrono>

#include "gtest/gtest.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

struct TestAsset : public Asset {
 public:
  void SetFilename(const std::string& filename) override {
    this->filename = filename;
    callbacks.push_back(kSetFilename);
  }

  ErrorCode OnLoadWithError(const std::string& filename,
                            std::string* data) override {
    on_load_data = *data;
    callbacks.push_back(kOnLoad);
    return on_load_result;
  }

  ErrorCode OnFinalizeWithError(const std::string& filename,
                                std::string* data) override {
    on_final_data = *data;
    callbacks.push_back(kOnFinalize);
    return on_finalize_result;
  }

  void OnError(const std::string& filename, ErrorCode error) override {
    error_callback = error;
    callbacks.push_back(kOnError);
  }

  enum CallbackEvent {
    kSetFilename,
    kOnLoad,
    kOnFinalize,
    kOnError,
  };

  std::string filename;
  std::string on_load_data;
  std::string on_final_data;
  std::vector<CallbackEvent> callbacks;
  ErrorCode on_load_result = kErrorCode_Ok;
  ErrorCode on_finalize_result = kErrorCode_Ok;
  ErrorCode error_callback = kErrorCode_Ok;
};

constexpr char kDummyData[] = "hello world";
constexpr char kDummyData2[] = "goodbye folks";

bool LoadFile(const char* filename, std::string* data) {
  *data = kDummyData;
  return true;
}

bool LoadFile2(const char* filename, std::string* data) {
  *data = kDummyData2;
  return true;
}

bool LoadFileBad(const char* filename, std::string* data) {
  return false;
}

TEST(AssetLoader, LoadNow) {
  AssetLoader loader(LoadFile);
  auto asset = loader.LoadNow<TestAsset>("filename.txt");

  EXPECT_EQ("filename.txt", asset->filename);
  EXPECT_EQ(kDummyData, asset->on_load_data);
  EXPECT_EQ(kDummyData, asset->on_final_data);
  EXPECT_EQ(3, static_cast<int>(asset->callbacks.size()));
  EXPECT_EQ(TestAsset::kSetFilename, asset->callbacks[0]);
  EXPECT_EQ(TestAsset::kOnLoad, asset->callbacks[1]);
  EXPECT_EQ(TestAsset::kOnFinalize, asset->callbacks[2]);
}

TEST(AssetLoader, LoadAsync) {
  AssetLoader loader(LoadFile);
  auto asset = loader.LoadAsync<TestAsset>("filename.txt");

  while (loader.Finalize() != 0) {
  }

  EXPECT_EQ("filename.txt", asset->filename);
  EXPECT_EQ(kDummyData, asset->on_load_data);
  EXPECT_EQ(kDummyData, asset->on_final_data);
  EXPECT_EQ(3, static_cast<int>(asset->callbacks.size()));
  EXPECT_EQ(TestAsset::kSetFilename, asset->callbacks[0]);
  EXPECT_EQ(TestAsset::kOnLoad, asset->callbacks[1]);
  EXPECT_EQ(TestAsset::kOnFinalize, asset->callbacks[2]);
}

TEST(AssetLoader, LoadIntoNow) {
  AssetLoader loader(LoadFile);
  auto asset = std::make_shared<TestAsset>();
  loader.LoadIntoNow<TestAsset>("filename.txt", asset);

  EXPECT_EQ("filename.txt", asset->filename);
  EXPECT_EQ(kDummyData, asset->on_load_data);
  EXPECT_EQ(kDummyData, asset->on_final_data);
  EXPECT_EQ(3, static_cast<int>(asset->callbacks.size()));
  EXPECT_EQ(TestAsset::kSetFilename, asset->callbacks[0]);
  EXPECT_EQ(TestAsset::kOnLoad, asset->callbacks[1]);
  EXPECT_EQ(TestAsset::kOnFinalize, asset->callbacks[2]);
}

TEST(AssetLoader, LoadIntoAsync) {
  AssetLoader loader(LoadFile);
  auto asset = std::make_shared<TestAsset>();
  loader.LoadIntoAsync<TestAsset>("filename.txt", asset);

  while (loader.Finalize() != 0) {
  }

  EXPECT_EQ("filename.txt", asset->filename);
  EXPECT_EQ(kDummyData, asset->on_load_data);
  EXPECT_EQ(kDummyData, asset->on_final_data);
  EXPECT_EQ(3, static_cast<int>(asset->callbacks.size()));
  EXPECT_EQ(TestAsset::kSetFilename, asset->callbacks[0]);
  EXPECT_EQ(TestAsset::kOnLoad, asset->callbacks[1]);
  EXPECT_EQ(TestAsset::kOnFinalize, asset->callbacks[2]);
}

TEST(AssetLoader, SimpleAsset) {
  AssetLoader loader(LoadFile);
  auto asset = loader.LoadNow<SimpleAsset>("filename.txt");

  EXPECT_EQ(sizeof(kDummyData), asset->GetSize() + 1);
  EXPECT_EQ(std::string(kDummyData),
            std::string(static_cast<const char*>(asset->GetData())));
  EXPECT_EQ(kDummyData, asset->GetStringData());

  std::string str = asset->ReleaseData();
  EXPECT_EQ(size_t(0), asset->GetSize());
  EXPECT_EQ(kDummyData, str);
}

TEST(AssetLoader, SetFileLoader) {
  AssetLoader loader(LoadFile);
  auto asset1 = loader.LoadNow<TestAsset>("filename.txt");
  EXPECT_EQ(kDummyData, asset1->on_load_data);
  EXPECT_EQ(kDummyData, asset1->on_final_data);

  loader.SetLoadFunction(LoadFile2);
  auto asset2 = loader.LoadNow<TestAsset>("filename.txt");
  EXPECT_EQ(kDummyData2, asset2->on_load_data);
  EXPECT_EQ(kDummyData2, asset2->on_final_data);
}

TEST(AssetLoader, LoadFileError) {
  AssetLoader loader(LoadFileBad);
  auto asset = std::make_shared<TestAsset>();
  loader.LoadIntoNow<TestAsset>("filename.txt", asset);

  EXPECT_EQ(kErrorCode_NotFound, asset->error_callback);
  EXPECT_EQ(2, static_cast<int>(asset->callbacks.size()));
  EXPECT_EQ(TestAsset::kSetFilename, asset->callbacks[0]);
  EXPECT_EQ(TestAsset::kOnError, asset->callbacks[1]);
}

TEST(AssetLoader, OnLoadError) {
  AssetLoader loader(LoadFile);
  auto asset = std::make_shared<TestAsset>();
  asset->on_load_result = kErrorCode_Unknown;
  loader.LoadIntoNow<TestAsset>("filename.txt", asset);

  EXPECT_EQ(kErrorCode_Unknown, asset->error_callback);
  EXPECT_EQ(3, static_cast<int>(asset->callbacks.size()));
  EXPECT_EQ(TestAsset::kSetFilename, asset->callbacks[0]);
  EXPECT_EQ(TestAsset::kOnLoad, asset->callbacks[1]);
  EXPECT_EQ(TestAsset::kOnError, asset->callbacks[2]);
}

TEST(AssetLoader, OnFinalizeError) {
  AssetLoader loader(LoadFile);
  auto asset = std::make_shared<TestAsset>();
  asset->on_finalize_result = kErrorCode_Unknown;
  loader.LoadIntoNow<TestAsset>("filename.txt", asset);

  EXPECT_EQ(kErrorCode_Unknown, asset->error_callback);
  EXPECT_EQ(4, static_cast<int>(asset->callbacks.size()));
  EXPECT_EQ(TestAsset::kSetFilename, asset->callbacks[0]);
  EXPECT_EQ(TestAsset::kOnLoad, asset->callbacks[1]);
  EXPECT_EQ(TestAsset::kOnFinalize, asset->callbacks[2]);
  EXPECT_EQ(TestAsset::kOnError, asset->callbacks[3]);
}

TEST(AssetLoader, OnErrorCallback) {
  AssetLoader loader(LoadFileBad);

  std::string error_filename;
  ErrorCode error_code;
  loader.SetOnErrorFunction([&](const std::string& filename, ErrorCode error) {
    error_filename = filename;
    error_code = error;
  });

  auto asset = std::make_shared<TestAsset>();
  loader.LoadIntoNow<TestAsset>("filename.txt", asset);

  EXPECT_EQ("filename.txt", error_filename);
  EXPECT_EQ(kErrorCode_NotFound, error_code);
}

}  // namespace
}  // namespace lull

LULLABY_SETUP_TYPEID(TestAsset);
