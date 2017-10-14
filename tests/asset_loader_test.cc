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

#include <chrono>

#include "gtest/gtest.h"
#include "lullaby/modules/file/asset_loader.h"
#include "tests/portable_test_macros.h"

namespace lull {
namespace {

struct TestAsset : public Asset {
 public:
  void SetFilename(const std::string& filename) override {
    this->filename = filename;
    callbacks.push_back(kSetFilename);
  }

  void OnLoad(const std::string& filename, std::string* data) override {
    on_load_data = *data;
    callbacks.push_back(kOnLoad);
  }

  void OnFinalize(const std::string& filename, std::string* data) override {
    on_final_data = *data;
    callbacks.push_back(kOnFinalize);
  }

  enum CallbackEvent {
    kSetFilename,
    kOnLoad,
    kOnFinalize,
  };

  std::string filename;
  std::string on_load_data;
  std::string on_final_data;
  std::vector<CallbackEvent> callbacks;
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

TEST(AssetLoaderDeathTest, NullFileLoader) {
  PORT_EXPECT_DEBUG_DEATH(AssetLoader loader(nullptr), "");
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


}  // namespace
}  // namespace lull

LULLABY_SETUP_TYPEID(TestAsset);
