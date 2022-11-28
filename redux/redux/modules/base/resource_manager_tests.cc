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

#include "gtest/gtest.h"
#include "redux/modules/base/resource_manager.h"

namespace redux {
namespace {

struct TestResource {
 public:
  TestResource() : value(0) {}
  explicit TestResource(int value) : value(value) {}

  int value;
};

TEST(ResourceManagerTest, Create) {
  const HashValue key(123);

  ResourceManager<TestResource> manager;
  auto res = manager.Create(key, []() {
    return std::shared_ptr<TestResource>(new TestResource(456));
  });
  EXPECT_EQ(456, res->value);
}

TEST(ResourceManagerTest, Find) {
  const HashValue key(123);

  ResourceManager<TestResource> manager(ResourceCacheMode::kCacheFullyOnCreate);
  manager.Create(key, []() {
    return std::shared_ptr<TestResource>(new TestResource(456));
  });

  auto res = manager.Find(key);
  EXPECT_EQ(456, res->value);
}

TEST(ResourceManagerTest, NoFind) {
  const HashValue key(123);

  ResourceManager<TestResource> manager;
  manager.Create(key, []() {
    return std::shared_ptr<TestResource>(new TestResource(456));
  });

  auto res = manager.Find(HashValue(456));
  EXPECT_EQ(nullptr, res);
}

TEST(ResourceManagerTest, Release) {
  const HashValue key(123);

  ResourceManager<TestResource> manager(ResourceCacheMode::kCacheFullyOnCreate);
  manager.Create(key, []() {
    return std::shared_ptr<TestResource>(new TestResource(456));
  });

  auto res = manager.Find(key);
  EXPECT_EQ(456, res->value);

  manager.Release(key);
  res.reset();

  res = manager.Find(key);
  EXPECT_EQ(nullptr, res);
}

TEST(ResourceManagerTest, GroupAttachRelease) {
  const HashValue key(123);

  ResourceManager<TestResource> manager(ResourceCacheMode::kCacheFullyOnCreate);
  manager.PushNewResourceGroup();
  manager.Create(key, []() {
    return std::shared_ptr<TestResource>(new TestResource(456));
  });

  {
    auto res = manager.Find(key);
    EXPECT_EQ(456, res->value);
  }

  auto* group = manager.PopResourceGroup();
  manager.ReleaseResourceGroup(group);

  {
    auto res = manager.Find(key);
    EXPECT_EQ(nullptr, res);
  }
}

TEST(ResourceManagerTest, GroupAttachDetachRelease) {
  const HashValue key1(123);
  const HashValue key2(456);

  ResourceManager<TestResource> manager(ResourceCacheMode::kCacheFullyOnCreate);
  manager.PushNewResourceGroup();
  manager.Create(key1, []() {
    return std::shared_ptr<TestResource>(new TestResource(456));
  });
  auto* group = manager.PopResourceGroup();
  manager.Create(key2, []() {
    return std::shared_ptr<TestResource>(new TestResource(789));
  });

  manager.ReleaseResourceGroup(group);

  {
    auto res = manager.Find(key1);
    EXPECT_EQ(nullptr, res);
  }

  {
    auto res = manager.Find(key2);
    EXPECT_EQ(789, res->value);
  }
}

TEST(ResourceManagerTest, ReleaseAlive) {
  const HashValue key(123);

  ResourceManager<TestResource> manager(ResourceCacheMode::kCacheFullyOnCreate);
  manager.Create(key, []() {
    return std::shared_ptr<TestResource>(new TestResource(456));
  });

  auto res = manager.Find(key);
  EXPECT_EQ(456, res->value);

  manager.Release(key);

  auto res2 = manager.Find(key);
  EXPECT_EQ(res, res2);
}

TEST(ResourceManagerTest, Recreate) {
  const HashValue key(123);

  ResourceManager<TestResource> manager(ResourceCacheMode::kCacheFullyOnCreate);
  manager.Create(key, []() {
    return std::shared_ptr<TestResource>(new TestResource(456));
  });
  manager.Create(key, []() {
    return std::shared_ptr<TestResource>(new TestResource(789));
  });

  auto res = manager.Find(key);
  EXPECT_EQ(456, res->value);
}

TEST(ResourceManagerTest, RecreateAlive) {
  const HashValue key(123);

  ResourceManager<TestResource> manager(ResourceCacheMode::kCacheFullyOnCreate);
  manager.Create(key, []() {
    return std::shared_ptr<TestResource>(new TestResource(456));
  });
  auto res = manager.Find(key);
  manager.Release(key);

  manager.Create(key, []() {
    return std::shared_ptr<TestResource>(new TestResource(789));
  });

  auto res2 = manager.Find(key);
  EXPECT_EQ(res, res2);
}

TEST(ResourceManagerTest, ExplicitCache) {
  const HashValue key(123);

  ResourceManager<TestResource> manager(ResourceCacheMode::kCacheExplicitly);

  auto obj1 = manager.Create(key, []() {
    return std::shared_ptr<TestResource>(new TestResource(456));
  });
  EXPECT_EQ(456, obj1->value);

  auto obj2 = manager.Find(key);
  EXPECT_EQ(nullptr, obj2);

  manager.Register(key, obj1);

  obj2 = manager.Find(key);
  EXPECT_EQ(obj1.get(), obj2.get());
}

TEST(ResourceManagerTest, WeakCache) {
  const HashValue key(123);

  ResourceManager<TestResource> manager(ResourceCacheMode::kWeakCachingOnly);

  auto obj1 = manager.Create(key, []() {
    return std::shared_ptr<TestResource>(new TestResource(456));
  });
  EXPECT_EQ(456, obj1->value);

  auto obj2 = manager.Find(key);
  EXPECT_EQ(obj1.get(), obj2.get());

  obj1.reset();

  obj2 = manager.Find(key);
  EXPECT_EQ(456, obj2->value);

  obj2.reset();

  obj2 = manager.Find(key);
  EXPECT_EQ(nullptr, obj2);
}

TEST(ResourceManagerTest, StrongCache) {
  const HashValue key(123);

  ResourceManager<TestResource> manager(ResourceCacheMode::kCacheFullyOnCreate);

  auto obj1 = manager.Create(key, []() {
    return std::shared_ptr<TestResource>(new TestResource(456));
  });
  EXPECT_EQ(456, obj1->value);

  auto obj2 = manager.Find(key);
  EXPECT_EQ(obj1.get(), obj2.get());

  obj1.reset();

  obj2 = manager.Find(key);
  EXPECT_EQ(456, obj2->value);

  obj2.reset();

  obj2 = manager.Find(key);
  EXPECT_EQ(456, obj2->value);
}

TEST(ResourceManagerTest, TrackNewInstances) {
  constexpr HashValue key(123);
  ResourceManager<TestResource> manager;

  for (int i = 0; i < 10; ++i) {
    auto obj = manager.Create(
        key, []() { return std::shared_ptr<TestResource>(new TestResource); });
    EXPECT_EQ(manager.Find(key), obj);
    manager.Release(key);
    EXPECT_EQ(manager.Find(key), obj);
    obj.reset();
    EXPECT_EQ(manager.Find(key), nullptr);
  }
}

}  // namespace
}  // namespace redux
