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

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "absl/container/flat_hash_map.h"
#include "redux/modules/base/serialize_traits.h"

namespace redux {
namespace {

TEST(SerializeTraitsTest, Pointer) {
  using Pointer = void*;
  using ConstPointer = const void*;
  using NonPointer = int;

  EXPECT_TRUE(IsPointerV<Pointer>);
  EXPECT_TRUE(IsPointerV<ConstPointer>);
  EXPECT_FALSE(IsPointerV<NonPointer>);
}

TEST(SerializeTraitsTest, IsUniquePtr) {
  using UniquePtr = std::unique_ptr<std::string>;
  using UniquePtrWithDeleter =
      std::unique_ptr<std::string, std::function<void(std::string*)>>;
  using NonUniquePtr = std::shared_ptr<std::string>;

  EXPECT_TRUE(IsUniquePtrV<UniquePtr>);
  EXPECT_TRUE(IsUniquePtrV<UniquePtrWithDeleter>);
  EXPECT_FALSE(IsUniquePtrV<NonUniquePtr>);
}

TEST(SerializeTraitsTest, IsOptional) {
  using Opt = std::optional<std::string>;
  using NonOpt = std::string;

  EXPECT_TRUE(IsOptionalV<Opt>);
  EXPECT_FALSE(IsOptionalV<NonOpt>);
}

TEST(SerializeTraitsTest, IsVector) {
  struct MyAlloc : public std::allocator<std::string> {};
  using Vec = std::vector<std::string>;
  using VecWithAlloc = std::vector<std::string, MyAlloc>;
  using NonVec = std::string;

  EXPECT_TRUE(IsVectorV<Vec>);
  EXPECT_TRUE(IsVectorV<VecWithAlloc>);
  EXPECT_FALSE(IsVectorV<NonVec>);
}

TEST(SerializeTraitsTest, IsMap) {
  using Map = absl::flat_hash_map<std::string, std::string>;
  using NonMap = std::string;

  EXPECT_TRUE(IsMapV<Map>);
  EXPECT_FALSE(IsMapV<NonMap>);
}

}  // namespace
}  // namespace redux
