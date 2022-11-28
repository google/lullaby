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
#include "redux/modules/base/dependency_graph.h"

namespace redux {
namespace {

using ::testing::Eq;

TEST(DependencyGraphTest, NoEdges) {
  DependencyGraph<uint32_t> graph;
  graph.AddNode(1);
  graph.AddNode(2);

  absl::flat_hash_set<uint32_t> visited;
  graph.Traverse([&](uint32_t node) { visited.emplace(node); });

  EXPECT_THAT(visited.size(), Eq(2));
  EXPECT_TRUE(visited.contains(1));
  EXPECT_TRUE(visited.contains(2));
}

TEST(DependencyGraphTest, OneEdge) {
  DependencyGraph<uint32_t> graph;
  graph.AddNode(1);
  graph.AddNode(2);
  graph.AddDependency(2, 1);

  std::vector<uint32_t> visited;
  graph.Traverse([&](uint32_t node) { visited.emplace_back(node); });

  EXPECT_THAT(visited.size(), Eq(2));
  EXPECT_THAT(visited[0], Eq(1));
  EXPECT_THAT(visited[1], Eq(2));
}

TEST(DependencyGraphTest, OneEdgeReversed) {
  DependencyGraph<uint32_t> graph;
  graph.AddNode(2);
  graph.AddNode(1);
  graph.AddDependency(2, 1);

  std::vector<uint32_t> visited;
  graph.Traverse([&](uint32_t node) { visited.emplace_back(node); });

  EXPECT_THAT(visited.size(), Eq(2));
  EXPECT_THAT(visited[0], Eq(1));
  EXPECT_THAT(visited[1], Eq(2));
}

TEST(DependencyGraphTest, Chain) {
  DependencyGraph<uint32_t> graph;
  graph.AddNode(1);
  graph.AddNode(2);
  graph.AddNode(3);
  graph.AddDependency(2, 1);
  graph.AddDependency(3, 2);

  std::vector<uint32_t> visited;
  graph.Traverse([&](uint32_t node) { visited.emplace_back(node); });

  EXPECT_THAT(visited.size(), Eq(3));
  EXPECT_THAT(visited[0], Eq(1));
  EXPECT_THAT(visited[1], Eq(2));
  EXPECT_THAT(visited[2], Eq(3));
}

TEST(DependencyGraphTest, Diamon) {
  DependencyGraph<uint32_t> graph;
  graph.AddNode(1);
  graph.AddNode(2);
  graph.AddNode(3);
  graph.AddNode(4);
  graph.AddDependency(2, 1);
  graph.AddDependency(3, 1);
  graph.AddDependency(4, 2);
  graph.AddDependency(4, 3);

  std::vector<uint32_t> visit_order;
  absl::flat_hash_set<uint32_t> visit_set;
  graph.Traverse([&](uint32_t node) {
    visit_order.emplace_back(node);
    visit_set.emplace(node);
  });

  EXPECT_THAT(visit_order.size(), Eq(4));
  EXPECT_THAT(visit_set.size(), Eq(4));
  EXPECT_THAT(visit_order[0], Eq(1));
  EXPECT_TRUE(visit_set.contains(2));
  EXPECT_TRUE(visit_set.contains(3));
  EXPECT_THAT(visit_order[3], Eq(4));
}

TEST(DependencyGraphTest, Cycle) {
  DependencyGraph<uint32_t> graph;
  graph.AddNode(1);
  graph.AddNode(2);
  graph.AddDependency(2, 1);
  graph.AddDependency(1, 2);
  EXPECT_DEATH(graph.Traverse([&](uint32_t node) {}), "");
}
}  // namespace
}  // namespace redux
