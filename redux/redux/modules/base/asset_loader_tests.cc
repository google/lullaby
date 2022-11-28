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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/functional/bind_front.h"
#include "redux/modules/base/asset_loader.h"
#include "redux/modules/base/data_builder.h"

namespace redux {
namespace {

using ::testing::Eq;

class AssetLoaderTest : public testing::Test {
 protected:
  void SetUp() override {
    asset_loader_.emplace(&registry_);
    auto on_load = absl::bind_front(&AssetLoaderTest::OnOpen, this);
    asset_loader_->SetOpenFunction(on_load);
  }

  void FailOnOpen(std::string message) { fail_message_ = std::move(message); }

  AssetLoader::StatusOrReader OnOpen(std::string_view filename) {
    if (!fail_message_.empty()) {
      return absl::NotFoundError(fail_message_);
    }
    file_contents_ = filename;
    auto data = reinterpret_cast<const std::byte*>(file_contents_.data());
    return DataReader::FromByteSpan({data, file_contents_.size()});
  }

  Registry registry_;
  std::optional<AssetLoader> asset_loader_;
  std::string fail_message_;
  std::string file_contents_;
};

static DataContainer FromString(std::string_view str) {
  DataBuilder builder(str.size());
  builder.Append(str.data(), str.size());
  return builder.Release();
}

static std::string ReadString(DataReader& reader) {
  std::string str;
  str.resize(reader.GetTotalLength());
  reader.Read(str.data(), reader.GetTotalLength());
  return str;
}

static std::string AsString(const DataContainer& data) {
  auto c_str = reinterpret_cast<const char*>(data.GetBytes());
  return std::string(c_str, data.GetNumBytes());
}

TEST_F(AssetLoaderTest, OpenNow) {
  auto asset = asset_loader_->OpenNow("filename.txt");
  EXPECT_TRUE(asset.ok());
  EXPECT_THAT(ReadString(asset.value()), Eq("filename.txt"));
}

TEST_F(AssetLoaderTest, LoadNow) {
  auto asset = asset_loader_->LoadNow("filename.txt");
  EXPECT_TRUE(asset.ok());
  EXPECT_THAT(AsString(asset.value()), Eq("filename.txt"));
}

TEST_F(AssetLoaderTest, OpenNowBadFile) {
  FailOnOpen("Fail");

  auto asset = asset_loader_->OpenNow("filename.txt");
  EXPECT_FALSE(asset.ok());
  EXPECT_THAT(asset.status().message(), Eq("Fail"));
}

TEST_F(AssetLoaderTest, LoadNowBadFile) {
  FailOnOpen("Fail");

  auto asset = asset_loader_->LoadNow("filename.txt");
  EXPECT_FALSE(asset.ok());
  EXPECT_THAT(asset.status().message(), Eq("Fail"));
}

TEST_F(AssetLoaderTest, OpenAsync) {
  auto future = asset_loader_->OpenAsync("filename.txt", nullptr, nullptr);
  while (asset_loader_->FinalizeAll() != 0) {
  }

  auto asset = future.get();
  EXPECT_TRUE(asset.ok());
  EXPECT_THAT(ReadString(asset.value()), Eq("filename.txt"));
}

TEST_F(AssetLoaderTest, LoadAsync) {
  auto future = asset_loader_->LoadAsync("filename.txt", nullptr, nullptr);
  while (asset_loader_->FinalizeAll() != 0) {
  }

  auto asset = future.get();
  EXPECT_TRUE(asset.ok());
  EXPECT_THAT(AsString(asset.value()), Eq("filename.txt"));
}

TEST_F(AssetLoaderTest, OpenAsyncWithFinalizer) {
  auto on_finalize = [](AssetLoader::StatusOrReader& asset) {
    EXPECT_TRUE(asset.ok());
    EXPECT_THAT(ReadString(asset.value()), Eq("filename.txt"));
  };

  asset_loader_->OpenAsync("filename.txt", nullptr, on_finalize);
  while (asset_loader_->FinalizeAll() != 0) {
  }
}

TEST_F(AssetLoaderTest, LoadAsyncWithFinalizer) {
  auto on_finalize = [](AssetLoader::StatusOrData& asset) {
    EXPECT_TRUE(asset.ok());
    EXPECT_THAT(AsString(asset.value()), Eq("filename.txt"));
  };

  asset_loader_->LoadAsync("filename.txt", nullptr, on_finalize);
  while (asset_loader_->FinalizeAll() != 0) {
  }
}

TEST_F(AssetLoaderTest, OpenAsyncWithOnLoadAndFinalizer) {
  std::string data = "hooray!";
  auto on_open = [&](AssetLoader::StatusOrReader& asset) {
    auto ptr = reinterpret_cast<const std::byte*>(data.data());
    asset = DataReader::FromByteSpan({ptr, data.size()});
  };
  auto on_finalize = [](AssetLoader::StatusOrReader& asset) {
    EXPECT_TRUE(asset.ok());
    EXPECT_THAT(ReadString(asset.value()), Eq("hooray!"));
  };
  asset_loader_->OpenAsync("filename.txt", on_open, on_finalize);

  while (asset_loader_->FinalizeAll() != 0) {
  }
}

TEST_F(AssetLoaderTest, LoadAsyncWithOnLoadAndFinalizer) {
  auto on_load = [](AssetLoader::StatusOrData& asset) {
    asset = FromString("hooray!");
  };
  auto on_finalize = [](AssetLoader::StatusOrData& asset) {
    EXPECT_TRUE(asset.ok());
    EXPECT_THAT(AsString(asset.value()), Eq("hooray!"));
  };
  asset_loader_->LoadAsync("filename.txt", on_load, on_finalize);

  while (asset_loader_->FinalizeAll() != 0) {
  }
}

TEST_F(AssetLoaderTest, LoadAsyncWhenPaused) {
  asset_loader_->StopAsyncOperations();

  auto on_load = [](AssetLoader::StatusOrData& asset) {
    asset = FromString("hooray!");
  };
  auto on_finalize = [](AssetLoader::StatusOrData& asset) {
    EXPECT_TRUE(asset.ok());
    EXPECT_THAT(AsString(asset.value()), Eq("hooray!"));
  };
  asset_loader_->LoadAsync("filename.txt", on_load, on_finalize);

  while (asset_loader_->FinalizeAll() != 0) {
  }
}

TEST_F(AssetLoaderTest, OpenAsyncBadFile) {
  FailOnOpen("Fail");

  bool on_open_called = false;
  auto on_open = [&](AssetLoader::StatusOrReader& asset) {
    EXPECT_FALSE(asset.ok());
    on_open_called = true;
  };
  bool on_finalize_called = false;
  auto on_finalize = [&](AssetLoader::StatusOrReader& asset) {
    EXPECT_FALSE(asset.ok());
    on_finalize_called = true;
  };
  asset_loader_->OpenAsync("filename.txt", on_open, on_finalize);

  while (asset_loader_->FinalizeAll() != 0) {
  }

  EXPECT_TRUE(on_open_called);
  EXPECT_TRUE(on_finalize_called);
}

TEST_F(AssetLoaderTest, LoadAsyncBadFile) {
  FailOnOpen("Fail");

  bool on_load_called = false;
  auto on_load = [&](AssetLoader::StatusOrData& asset) {
    EXPECT_FALSE(asset.ok());
    on_load_called = true;
  };
  bool on_finalize_called = false;
  auto on_finalize = [&](AssetLoader::StatusOrData& asset) {
    EXPECT_FALSE(asset.ok());
    on_finalize_called = true;
  };
  asset_loader_->LoadAsync("filename.txt", on_load, on_finalize);

  while (asset_loader_->FinalizeAll() != 0) {
  }

  EXPECT_TRUE(on_load_called);
  EXPECT_TRUE(on_finalize_called);
}

TEST_F(AssetLoaderTest, LoadAsyncBadOnLoad) {
  bool on_load_called = false;
  auto on_load = [&](AssetLoader::StatusOrData& asset) {
    on_load_called = true;
    asset = absl::Status(absl::StatusCode::kUnknown, "Fail");
  };
  bool on_finalize_called = false;
  auto on_finalize = [&](AssetLoader::StatusOrData& asset) {
    EXPECT_FALSE(asset.ok());
    on_finalize_called = true;
  };
  auto future = asset_loader_->LoadAsync("filename.txt", on_load, on_finalize);

  while (asset_loader_->FinalizeAll() != 0) {
  }

  auto asset = future.get();

  EXPECT_TRUE(on_load_called);
  EXPECT_TRUE(on_finalize_called);
  EXPECT_FALSE(asset.ok());
}

}  // namespace
}  // namespace redux
