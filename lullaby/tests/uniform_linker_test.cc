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

#include "lullaby/systems/render/detail/uniform_linker.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

using ::testing::Eq;
using detail::UniformLinker;

UniformLinker::GetUniformDataFn kDummyGetUniformDataFn =
    [](Entity target, int dimension, int count) { return nullptr; };

TEST(UniformLinkerTest, LinkUniform) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const HashValue name_hash = 3;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  float test_target_data[20];
  int update_count = 0;
  int get_data_count = 0;

  // Ensure that all the arguments passed into the lambdas are populated
  // correctly.
  manager.LinkUniform(
      test_target, test_source, name_hash,
      [test_dimension, test_count, &test_source_data, &test_target_data,
       &update_count](const float* data, int dimension, int count,
                      float* target_data) {
    EXPECT_THAT(dimension, Eq(test_dimension));
    EXPECT_THAT(count, Eq(test_count));
    EXPECT_THAT(data, Eq(test_source_data));
    EXPECT_THAT(target_data, Eq(test_target_data));
    ++update_count;
  });
  EXPECT_THAT(update_count, Eq(0));
  EXPECT_THAT(get_data_count, Eq(0));

  manager.UpdateLinkedUniforms(
      test_source, name_hash, test_source_data, test_dimension, test_count,
      [test_dimension, test_count, &test_target_data, &get_data_count](
          Entity target, int dimension, int count) {
    EXPECT_THAT(target, Eq(test_target));
    EXPECT_THAT(dimension, Eq(test_dimension));
    EXPECT_THAT(count, Eq(test_count));
    ++get_data_count;
    return test_target_data;
  });
  EXPECT_THAT(update_count, Eq(1));
  EXPECT_THAT(get_data_count, Eq(1));
}

TEST(UniformLinkerTest, LinkUniform_DefaultNullUpdateFn) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const HashValue name_hash = 3;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  float test_target_data[20];
  for (int i = 0; i < 20; ++i) {
    test_source_data[i] = static_cast<float>(i);
  }
  memset(test_target_data, 0., sizeof(test_target_data));
  int get_data_count = 0;

  // Default null update_fn will perform a simple memcpy.
  manager.LinkUniform(test_target, test_source, name_hash);
  EXPECT_THAT(get_data_count, Eq(0));
  for (int i = 0; i < 20; ++i) {
    EXPECT_THAT(test_target_data[i], Eq(0));
  }

  manager.UpdateLinkedUniforms(
      test_source, name_hash, test_source_data, test_dimension, test_count,
      [&test_target_data, &get_data_count](Entity target, int dimension,
                                           int count) {
    ++get_data_count;
    return test_target_data;
  });
  EXPECT_THAT(get_data_count, Eq(1));
  for (int i = 0; i < 20; ++i) {
    EXPECT_THAT(test_target_data[i], Eq(i));
  }
}

TEST(UniformLinkerDeathTest, LinkUniform_NullGetDataFn) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const HashValue name_hash = 3;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];

  // Default null update_fn will error if GetUniformDataFn returns nullptr.
  manager.LinkUniform(test_target, test_source, name_hash);

  EXPECT_DEBUG_DEATH(manager.UpdateLinkedUniforms(
                         test_source, name_hash, test_source_data,
                         test_dimension, test_count, kDummyGetUniformDataFn),
                     "Invalid target_data.");
  // We can't check get_data_count because EXPECT_DEBUG_DEATH forks a new
  // thread.
}

TEST(UniformLinkerTest, LinkUniform_DifferentName) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const HashValue name_hash = 3;
  const HashValue name_hash_2 = 33;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  int update_count = 0;
  int get_data_count = 0;

  manager.LinkUniform(
      test_target, test_source, name_hash,
      [&update_count](const float* data, int dimension, int count,
                      float* target_data) { ++update_count; });
  EXPECT_THAT(update_count, Eq(0));
  EXPECT_THAT(get_data_count, Eq(0));

  // This should not update because there is no link for name_hash_2.
  manager.UpdateLinkedUniforms(
      test_source, name_hash_2, test_source_data, test_dimension, test_count,
      [&get_data_count](Entity target, int dimension, int count) {
    ++get_data_count;
    return nullptr;
  });
  EXPECT_THAT(update_count, Eq(0));
  EXPECT_THAT(get_data_count, Eq(0));
}

TEST(UniformLinkerTest, LinkUniform_DifferentSource) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const Entity test_source_2 = 22;
  const HashValue name_hash = 3;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  int update_count = 0;
  int get_data_count = 0;

  manager.LinkUniform(
      test_target, test_source, name_hash,
      [&update_count](const float* data, int dimension, int count,
                      float* target_data) { ++update_count; });
  EXPECT_THAT(update_count, Eq(0));
  EXPECT_THAT(get_data_count, Eq(0));

  // This should not update because there is no link for test_source_2.
  manager.UpdateLinkedUniforms(
      test_source_2, name_hash, test_source_data, test_dimension, test_count,
      [&get_data_count](Entity target, int dimension, int count) {
    ++get_data_count;
    return nullptr;
  });
  EXPECT_THAT(update_count, Eq(0));
  EXPECT_THAT(get_data_count, Eq(0));
}

TEST(UniformLinkerTest, LinkUniform_MultipleTargets) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_target_2 = 11;
  const Entity test_source = 2;
  const HashValue name_hash = 3;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  std::unordered_set<Entity> update_set;
  std::unordered_set<Entity> get_data_set;

  manager.LinkUniform(
      test_target, test_source, name_hash,
      [&update_set, test_target](const float* data, int dimension, int count,
                                 float* target_data) {
    update_set.emplace(test_target);
  });
  manager.LinkUniform(
      test_target_2, test_source, name_hash,
      [&update_set, test_target_2](const float* data, int dimension, int count,
                                   float* target_data) {
    update_set.emplace(test_target_2);
  });
  std::unordered_set<Entity> expectation;
  EXPECT_THAT(update_set, Eq(expectation));
  EXPECT_THAT(get_data_set, Eq(expectation));

  // All linked targets from a source should be updated.
  manager.UpdateLinkedUniforms(
      test_source, name_hash, test_source_data, test_dimension, test_count,
      [&get_data_set] (Entity target, int dimension, int count) {
    get_data_set.emplace(target);
    return nullptr;
  });
  expectation = {test_target, test_target_2};
  EXPECT_THAT(update_set, Eq(expectation));
  EXPECT_THAT(get_data_set, Eq(expectation));
}

TEST(UniformLinkerTest, LinkUniform_MultipleSources) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const Entity test_source_2 = 22;
  const HashValue name_hash = 3;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  std::unordered_set<Entity> update_set;

  manager.LinkUniform(
      test_target, test_source, name_hash,
      [&update_set, test_source](const float* data, int dimension, int count,
                                 float* target_data) {
    update_set.emplace(test_source);
  });
  // The second link should override the first.
  manager.LinkUniform(
      test_target, test_source_2, name_hash,
      [&update_set, test_source_2](const float* data, int dimension, int count,
                                   float* target_data) {
    update_set.emplace(test_source_2);
  });
  std::unordered_set<Entity> expectation;
  EXPECT_THAT(update_set, Eq(expectation));

  // This will be ignored.
  manager.UpdateLinkedUniforms(test_source, name_hash, test_source_data,
                               test_dimension, test_count,
                               kDummyGetUniformDataFn);
  EXPECT_THAT(update_set, Eq(expectation));

  // The link exists only for test_source_2.
  manager.UpdateLinkedUniforms(test_source_2, name_hash, test_source_data,
                               test_dimension, test_count,
                               kDummyGetUniformDataFn);
  expectation = {test_source_2};
  EXPECT_THAT(update_set, Eq(expectation));
}

TEST(UniformLinkerTest, LinkUniform_IgnoreTarget) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const HashValue name_hash = 3;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  int update_count = 0;
  int get_data_count = 0;
  UniformLinker::GetUniformDataFn get_data_fn =
      [&get_data_count](Entity target, int dimension, int count) {
    ++get_data_count;
    return nullptr;
  };

  manager.LinkUniform(
      test_target, test_source, name_hash,
      [&update_count](const float* data, int dimension, int count,
                      float* target_data) { ++update_count; });
  EXPECT_THAT(update_count, Eq(0));
  EXPECT_THAT(get_data_count, Eq(0));

  // Updating source works.
  manager.UpdateLinkedUniforms(test_source, name_hash, test_source_data,
                               test_dimension, test_count, get_data_fn);
  EXPECT_THAT(update_count, Eq(1));
  EXPECT_THAT(get_data_count, Eq(1));

  // Nothing happens here.
  manager.IgnoreLinkedUniform(test_target, name_hash);
  EXPECT_THAT(update_count, Eq(1));
  EXPECT_THAT(get_data_count, Eq(1));

  // But now, setting source with will leave target unchanged.
  manager.UpdateLinkedUniforms(test_source, name_hash, test_source_data,
                               test_dimension, test_count, get_data_fn);
  EXPECT_THAT(update_count, Eq(1));
  EXPECT_THAT(get_data_count, Eq(1));
}

TEST(UniformLinkerTest, LinkUniform_IgnoreTargetFirst) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const HashValue name_hash = 3;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  int update_count = 0;
  int get_data_count = 0;
  UniformLinker::GetUniformDataFn get_data_fn =
      [&get_data_count](Entity target, int dimension, int count) {
    ++get_data_count;
    return nullptr;
  };

  // Set Ignored uniform on target before any links are created.
  manager.IgnoreLinkedUniform(test_target, name_hash);

  manager.LinkUniform(
      test_target, test_source, name_hash,
      [&update_count](const float* data, int dimension, int count,
                      float* target_data) { ++update_count; });
  EXPECT_THAT(update_count, Eq(0));
  EXPECT_THAT(get_data_count, Eq(0));

  // Target will still be ignored.
  manager.UpdateLinkedUniforms(test_source, name_hash, test_source_data,
                               test_dimension, test_count, get_data_fn);
  EXPECT_THAT(update_count, Eq(0));
  EXPECT_THAT(get_data_count, Eq(0));
}

TEST(UniformLinkerTest, LinkUniform_UnlinkSource) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const HashValue name_hash = 3;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  int update_count = 0;
  int get_data_count = 0;
  UniformLinker::GetUniformDataFn get_data_fn =
      [&get_data_count](Entity target, int dimension, int count) {
    ++get_data_count;
    return nullptr;
  };

  manager.LinkUniform(
      test_target, test_source, name_hash,
      [&update_count](const float* data, int dimension, int count,
                      float* target_data) { ++update_count; });
  EXPECT_THAT(update_count, Eq(0));
  EXPECT_THAT(get_data_count, Eq(0));

  manager.UpdateLinkedUniforms(test_source, name_hash, test_source_data,
                               test_dimension, test_count, get_data_fn);
  EXPECT_THAT(update_count, Eq(1));
  EXPECT_THAT(get_data_count, Eq(1));

  // No more updates after unlinking source.
  manager.UnlinkUniforms(test_source);
  manager.UpdateLinkedUniforms(test_source, name_hash, test_source_data,
                               test_dimension, test_count, get_data_fn);
  EXPECT_THAT(update_count, Eq(1));
  EXPECT_THAT(get_data_count, Eq(1));
}

TEST(UniformLinkerTest, LinkUniform_UnlinkTarget) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const HashValue name_hash = 3;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  int update_count = 0;
  int get_data_count = 0;
  UniformLinker::GetUniformDataFn get_data_fn =
      [&get_data_count](Entity target, int dimension, int count) {
    ++get_data_count;
    return nullptr;
  };

  manager.LinkUniform(
      test_target, test_source, name_hash,
      [&update_count](const float* data, int dimension, int count,
                      float* target_data) { ++update_count; });
  EXPECT_THAT(update_count, Eq(0));
  EXPECT_THAT(get_data_count, Eq(0));

  manager.UpdateLinkedUniforms(test_source, name_hash, test_source_data,
                               test_dimension, test_count, get_data_fn);
  EXPECT_THAT(update_count, Eq(1));
  EXPECT_THAT(get_data_count, Eq(1));

  // No more updates after unlinking target.
  manager.UnlinkUniforms(test_target);
  manager.UpdateLinkedUniforms(test_source, name_hash, test_source_data,
                               test_dimension, test_count, get_data_fn);
  EXPECT_THAT(update_count, Eq(1));
  EXPECT_THAT(get_data_count, Eq(1));
}

TEST(UniformLinkerTest, LinkAllUniforms) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const HashValue name_hash = 3;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  float test_target_data[20];
  int update_count = 0;
  int get_data_count = 0;

  // Ensure that all the arguments passed into the lambdas are populated
  // correctly.
  manager.LinkAllUniforms(
      test_target, test_source,
      [test_dimension, test_count, &test_source_data, &test_target_data,
       &update_count](const float* data, int dimension, int count,
                      float* target_data) {
    EXPECT_THAT(dimension, Eq(test_dimension));
    EXPECT_THAT(count, Eq(test_count));
    EXPECT_THAT(data, Eq(test_source_data));
    EXPECT_THAT(target_data, Eq(test_target_data));
    ++update_count;
  });
  EXPECT_THAT(update_count, Eq(0));
  EXPECT_THAT(get_data_count, Eq(0));

  manager.UpdateLinkedUniforms(
      test_source, name_hash, test_source_data, test_dimension, test_count,
      [test_dimension, test_count, &test_target_data, &get_data_count](
          Entity target, int dimension, int count) {
    EXPECT_THAT(target, Eq(test_target));
    EXPECT_THAT(dimension, Eq(test_dimension));
    EXPECT_THAT(count, Eq(test_count));
    ++get_data_count;
    return test_target_data;
  });
  EXPECT_THAT(update_count, Eq(1));
  EXPECT_THAT(get_data_count, Eq(1));
}

TEST(UniformLinkerTest, LinkAllUniforms_DefaultNullUpdateFn) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const HashValue name_hash = 3;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  float test_target_data[20];
  for (int i = 0; i < 20; ++i) {
    test_source_data[i] = static_cast<float>(i);
  }
  memset(test_target_data, 0., sizeof(test_target_data));
  int get_data_count = 0;

  // Default null update_fn will perform a simple memcpy.
  manager.LinkAllUniforms(test_target, test_source);
  EXPECT_THAT(get_data_count, Eq(0));
  for (int i = 0; i < 20; ++i) {
    EXPECT_THAT(test_target_data[i], Eq(0));
  }

  manager.UpdateLinkedUniforms(
      test_source, name_hash, test_source_data, test_dimension, test_count,
      [&test_target_data, &get_data_count](Entity target, int dimension,
                                           int count) {
    ++get_data_count;
    return test_target_data;
  });
  EXPECT_THAT(get_data_count, Eq(1));
  for (int i = 0; i < 20; ++i) {
    EXPECT_THAT(test_target_data[i], Eq(i));
  }
}

TEST(UniformLinkerDeathTest, LinkAllUniforms_NullGetDataFn) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const HashValue name_hash = 3;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];

  // Default null update_fn will error if GetUniformDataFn returns nullptr.
  manager.LinkAllUniforms(test_target, test_source);

  EXPECT_DEBUG_DEATH(manager.UpdateLinkedUniforms(
                         test_source, name_hash, test_source_data,
                         test_dimension, test_count, kDummyGetUniformDataFn),
                     "Invalid target_data.");
  // We can't check get_data_count because EXPECT_DEBUG_DEATH forks a new
  // thread.
}

TEST(UniformLinkerTest, LinkAllUniforms_DifferentSource) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const Entity test_source_2 = 22;
  const HashValue name_hash = 3;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  int update_count = 0;
  int get_data_count = 0;

  manager.LinkAllUniforms(
      test_target, test_source,
      [&update_count](const float* data, int dimension, int count,
                      float* target_data) { ++update_count; });
  EXPECT_THAT(update_count, Eq(0));
  EXPECT_THAT(get_data_count, Eq(0));

  // This should not update because there is no link for test_source_2.
  manager.UpdateLinkedUniforms(
      test_source_2, name_hash, test_source_data, test_dimension, test_count,
      [&get_data_count](Entity target, int dimension, int count) {
    ++get_data_count;
    return nullptr;
  });
  EXPECT_THAT(update_count, Eq(0));
  EXPECT_THAT(get_data_count, Eq(0));
}

TEST(UniformLinkerTest, LinkAllUniforms_MultipleTargets) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_target_2 = 11;
  const Entity test_source = 2;
  const HashValue name_hash = 3;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  std::unordered_set<Entity> update_set;
  std::unordered_set<Entity> get_data_set;

  manager.LinkAllUniforms(
      test_target, test_source,
      [&update_set, test_target](const float* data, int dimension, int count,
                                 float* target_data) {
    update_set.emplace(test_target);
  });
  manager.LinkAllUniforms(
      test_target_2, test_source,
      [&update_set, test_target_2](const float* data, int dimension, int count,
                                   float* target_data) {
    update_set.emplace(test_target_2);
  });
  std::unordered_set<Entity> expectation;
  EXPECT_THAT(update_set, Eq(expectation));
  EXPECT_THAT(get_data_set, Eq(expectation));

  // All linked targets from a source should be updated.
  manager.UpdateLinkedUniforms(
      test_source, name_hash, test_source_data, test_dimension, test_count,
      [&get_data_set] (Entity target, int dimension, int count) {
    get_data_set.emplace(target);
    return nullptr;
  });
  expectation = {test_target, test_target_2};
  EXPECT_THAT(update_set, Eq(expectation));
  EXPECT_THAT(get_data_set, Eq(expectation));
}

TEST(UniformLinkerTest, LinkAllUniforms_MultipleSources) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const Entity test_source_2 = 22;
  const HashValue name_hash = 3;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  std::unordered_set<Entity> update_set;

  manager.LinkAllUniforms(
      test_target, test_source,
      [&update_set, test_source](const float* data, int dimension, int count,
                                 float* target_data) {
    update_set.emplace(test_source);
  });
  // The second link should override the first.
  manager.LinkAllUniforms(
      test_target, test_source_2,
      [&update_set, test_source_2](const float* data, int dimension, int count,
                                   float* target_data) {
    update_set.emplace(test_source_2);
  });
  std::unordered_set<Entity> expectation;
  EXPECT_THAT(update_set, Eq(expectation));

  // This will be ignored.
  manager.UpdateLinkedUniforms(test_source, name_hash, test_source_data,
                               test_dimension, test_count,
                               kDummyGetUniformDataFn);
  EXPECT_THAT(update_set, Eq(expectation));

  // The link exists only for test_source_2.
  manager.UpdateLinkedUniforms(test_source_2, name_hash, test_source_data,
                               test_dimension, test_count,
                               kDummyGetUniformDataFn);
  expectation = {test_source_2};
  EXPECT_THAT(update_set, Eq(expectation));
}

TEST(UniformLinkerTest, LinkAllUniforms_IgnoreTarget) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const HashValue name_hash = 3;
  const HashValue name_hash_2 = 33;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  int update_count = 0;
  int get_data_count = 0;
  UniformLinker::GetUniformDataFn get_data_fn =
      [&get_data_count](Entity target, int dimension, int count) {
    ++get_data_count;
    return nullptr;
  };

  manager.LinkAllUniforms(
      test_target, test_source,
      [&update_count](const float* data, int dimension, int count,
                      float* target_data) { ++update_count; });
  EXPECT_THAT(update_count, Eq(0));
  EXPECT_THAT(get_data_count, Eq(0));

  // Updating source works.
  manager.UpdateLinkedUniforms(test_source, name_hash, test_source_data,
                               test_dimension, test_count, get_data_fn);
  EXPECT_THAT(update_count, Eq(1));
  EXPECT_THAT(get_data_count, Eq(1));

  // Nothing happens here.
  manager.IgnoreLinkedUniform(test_target, name_hash);
  EXPECT_THAT(update_count, Eq(1));
  EXPECT_THAT(get_data_count, Eq(1));

  // But now, setting source will leave target unchanged.
  manager.UpdateLinkedUniforms(test_source, name_hash, test_source_data,
                               test_dimension, test_count, get_data_fn);
  EXPECT_THAT(update_count, Eq(1));
  EXPECT_THAT(get_data_count, Eq(1));

  // Other name_hashes are still updated.
  manager.UpdateLinkedUniforms(test_source, name_hash_2, test_source_data,
                               test_dimension, test_count, get_data_fn);
  EXPECT_THAT(update_count, Eq(2));
  EXPECT_THAT(get_data_count, Eq(2));
}

TEST(UniformLinkerTest, LinkAllUniforms_IgnoreTargetFirst) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const HashValue name_hash = 3;
  const HashValue name_hash_2 = 33;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  int update_count = 0;
  int get_data_count = 0;
  UniformLinker::GetUniformDataFn get_data_fn =
      [&get_data_count](Entity target, int dimension, int count) {
    ++get_data_count;
    return nullptr;
  };

  // Set Ignored uniform on target before any links are created.
  manager.IgnoreLinkedUniform(test_target, name_hash);
  EXPECT_THAT(update_count, Eq(0));
  EXPECT_THAT(get_data_count, Eq(0));

  manager.LinkAllUniforms(
      test_target, test_source,
      [&update_count](const float* data, int dimension, int count,
                      float* target_data) { ++update_count; });
  EXPECT_THAT(update_count, Eq(0));
  EXPECT_THAT(get_data_count, Eq(0));

  // Target will be ignored.
  manager.UpdateLinkedUniforms(test_source, name_hash, test_source_data,
                               test_dimension, test_count, get_data_fn);
  EXPECT_THAT(update_count, Eq(0));
  EXPECT_THAT(get_data_count, Eq(0));

  // Other name_hashes are still updated.
  manager.UpdateLinkedUniforms(test_source, name_hash_2, test_source_data,
                               test_dimension, test_count, get_data_fn);
  EXPECT_THAT(update_count, Eq(1));
  EXPECT_THAT(get_data_count, Eq(1));
}

TEST(UniformLinkerTest, LinkAllUniforms_UnlinkSource) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const HashValue name_hash = 3;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  int update_count = 0;
  int get_data_count = 0;
  UniformLinker::GetUniformDataFn get_data_fn =
      [&get_data_count](Entity target, int dimension, int count) {
    ++get_data_count;
    return nullptr;
  };

  manager.LinkAllUniforms(
      test_target, test_source,
      [&update_count](const float* data, int dimension, int count,
                      float* target_data) { ++update_count; });
  EXPECT_THAT(update_count, Eq(0));
  EXPECT_THAT(get_data_count, Eq(0));

  manager.UpdateLinkedUniforms(test_source, name_hash, test_source_data,
                               test_dimension, test_count, get_data_fn);
  EXPECT_THAT(update_count, Eq(1));
  EXPECT_THAT(get_data_count, Eq(1));

  // No more updates after unlinking source.
  manager.UnlinkUniforms(test_source);
  manager.UpdateLinkedUniforms(test_source, name_hash, test_source_data,
                               test_dimension, test_count, get_data_fn);
  EXPECT_THAT(update_count, Eq(1));
  EXPECT_THAT(get_data_count, Eq(1));
}

TEST(UniformLinkerTest, LinkAllUniforms_UnlinkTarget) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const HashValue name_hash = 3;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  int update_count = 0;
  int get_data_count = 0;
  UniformLinker::GetUniformDataFn get_data_fn =
      [&get_data_count](Entity target, int dimension, int count) {
    ++get_data_count;
    return nullptr;
  };

  manager.LinkAllUniforms(
      test_target, test_source,
      [&update_count](const float* data, int dimension, int count,
                      float* target_data) { ++update_count; });
  EXPECT_THAT(update_count, Eq(0));
  EXPECT_THAT(get_data_count, Eq(0));

  manager.UpdateLinkedUniforms(test_source, name_hash, test_source_data,
                               test_dimension, test_count, get_data_fn);
  EXPECT_THAT(update_count, Eq(1));
  EXPECT_THAT(get_data_count, Eq(1));

  // No more updates after unlinking target.
  manager.UnlinkUniforms(test_target);
  manager.UpdateLinkedUniforms(test_source, name_hash, test_source_data,
                               test_dimension, test_count, get_data_fn);
  EXPECT_THAT(update_count, Eq(1));
  EXPECT_THAT(get_data_count, Eq(1));
}

TEST(UniformLinkerTest, LinkUniformAndLinkAllUniforms) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const HashValue name_hash = 3;
  const HashValue name_hash_2 = 33;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  int link_uniform_count = 0;
  int link_uniforms_count = 0;

  manager.LinkUniform(
      test_target, test_source, name_hash,
      [&link_uniform_count](const float* data, int dimension, int count,
                            float* target_data) { ++link_uniform_count; });
  manager.LinkAllUniforms(
      test_target, test_source,
      [&link_uniforms_count](const float* data, int dimension, int count,
                             float* target_data) { ++link_uniforms_count; });
  EXPECT_THAT(link_uniform_count, Eq(0));
  EXPECT_THAT(link_uniforms_count, Eq(0));

  // When updating name_hash, LinkUniform takes precedence over LinkAllUniforms.
  manager.UpdateLinkedUniforms(test_source, name_hash, test_source_data,
                               test_dimension, test_count,
                               kDummyGetUniformDataFn);
  EXPECT_THAT(link_uniform_count, Eq(1));
  EXPECT_THAT(link_uniforms_count, Eq(0));

  // LinkAllUniforms will update all other name_hashes.
  manager.UpdateLinkedUniforms(test_source, name_hash_2, test_source_data,
                               test_dimension, test_count,
                               kDummyGetUniformDataFn);
  EXPECT_THAT(link_uniform_count, Eq(1));
  EXPECT_THAT(link_uniforms_count, Eq(1));
}

TEST(UniformLinkerTest, LinkUniformAndLinkAllUniforms_MultipleSources) {
  UniformLinker manager;
  const Entity test_target = 1;
  const Entity test_source = 2;
  const Entity test_source_2 = 22;
  const HashValue name_hash = 3;
  const int test_dimension = 4;
  const int test_count = 5;
  float test_source_data[20];
  int link_uniform_count = 0;
  int link_uniforms_count = 0;

  manager.LinkUniform(
      test_target, test_source, name_hash,
      [&link_uniform_count](const float* data, int dimension, int count,
                            float* target_data) { ++link_uniform_count; });
  // The second link should override the first.
  manager.LinkAllUniforms(
      test_target, test_source_2,
      [&link_uniforms_count](const float* data, int dimension, int count,
                             float* target_data) { ++link_uniforms_count; });
  EXPECT_THAT(link_uniform_count, Eq(0));
  EXPECT_THAT(link_uniforms_count, Eq(0));

  // This will be ignored.
  manager.UpdateLinkedUniforms(test_source, name_hash, test_source_data,
                               test_dimension, test_count,
                               kDummyGetUniformDataFn);
  EXPECT_THAT(link_uniform_count, Eq(0));
  EXPECT_THAT(link_uniforms_count, Eq(0));

  // The link exists only for test_source_2.
  manager.UpdateLinkedUniforms(test_source_2, name_hash, test_source_data,
                               test_dimension, test_count,
                               kDummyGetUniformDataFn);
  EXPECT_THAT(link_uniform_count, Eq(0));
  EXPECT_THAT(link_uniforms_count, Eq(1));
}

}  // namespace
}  // namespace lull
