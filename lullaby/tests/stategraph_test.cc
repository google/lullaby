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

#include "lullaby/modules/stategraph/stategraph.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/util/common_types.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/util/time.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

using ::testing::Eq;

HashValue IndexToId(size_t index) { return HashValue(index + 1); }

#define EXPECT_TRANSITION(T, A, B)             \
  EXPECT_THAT(T.from_state, Eq(IndexToId(A))); \
  EXPECT_THAT(T.to_state, Eq(IndexToId(B)))

class TestState : public StategraphState {
 public:
  explicit TestState(HashValue id) : StategraphState(id) {}

  void AddTransition(StategraphTransition transition) {
    StategraphState::AddTransition(std::move(transition));
  }
};

class StategraphPopulator {
 public:
  explicit StategraphPopulator(Stategraph* sg) : stategraph_(sg) {}

  ~StategraphPopulator() {
    for (auto& state : states_) {
      stategraph_->AddState(std::unique_ptr<StategraphState>(state.release()));
    }
  }

  StategraphPopulator& AddStates(size_t n) {
    for (size_t i = 0; i < n; ++i) {
      states_.emplace_back(MakeUnique<TestState>(IndexToId(i)));
    }
    return *this;
  }

  StategraphPopulator& AddTransition(size_t from, size_t to) {
    StategraphTransition transition;
    if (to < states_.size()) {
      transition.to_state = states_[to]->GetId();
    }
    if (from < states_.size()) {
      transition.from_state = states_[from]->GetId();
      states_[from]->AddTransition(transition);
    }
    return *this;
  }

  void PopulateStategraph(Stategraph* stategraph) { states_.clear(); }

 private:
  std::vector<std::unique_ptr<TestState>> states_;
  Stategraph* stategraph_;
};

TEST(StategraphTest, Disconnected) {
  Stategraph sg;
  StategraphPopulator(&sg).AddStates(2);

  auto path = sg.FindPath(IndexToId(0), IndexToId(1));
  EXPECT_TRUE(path.empty());
}

TEST(StategraphTest, Neighbours) {
  Stategraph sg;

  // Create a graph that looks like:
  //
  // [0]--[1]
  StategraphPopulator(&sg).AddStates(2).AddTransition(0, 1);

  auto path = sg.FindPath(IndexToId(0), IndexToId(1));
  EXPECT_THAT(path.size(), Eq(size_t(1)));
  EXPECT_TRANSITION(path[0], 0, 1);
}

TEST(StategraphTest, StraightLine) {
  Stategraph sg;

  // Create a graph that looks like:
  //
  // [0]--[1]--[2]--[3]--[4]
  StategraphPopulator(&sg)
      .AddStates(5)
      .AddTransition(0, 1)
      .AddTransition(1, 2)
      .AddTransition(2, 3)
      .AddTransition(3, 4);

  auto path = sg.FindPath(IndexToId(0), IndexToId(4));
  EXPECT_THAT(path.size(), Eq(size_t(4)));
  EXPECT_TRANSITION(path[0], 0, 1);
  EXPECT_TRANSITION(path[1], 1, 2);
  EXPECT_TRANSITION(path[2], 2, 3);
  EXPECT_TRANSITION(path[3], 3, 4);
}

TEST(StategraphTest, StraightLineWithBranches) {
  Stategraph sg;

  // Create a graph that looks like:
  //
  // [6]  [8]  [12] [14]
  //  |    |    |    |
  // [5]  [7]  [11] [13]
  //  |    |    |    |
  // [0]--[1]--[2]--[3]--[4]
  //       |
  //      [9]
  //       |
  //      [10]
  StategraphPopulator(&sg)
      .AddStates(15)
      .AddTransition(0, 1)  // main path
      .AddTransition(1, 2)
      .AddTransition(2, 3)
      .AddTransition(3, 4)
      .AddTransition(0, 5)  // branches
      .AddTransition(5, 6)
      .AddTransition(1, 7)
      .AddTransition(7, 8)
      .AddTransition(1, 9)
      .AddTransition(9, 10)
      .AddTransition(2, 11)
      .AddTransition(11, 12)
      .AddTransition(3, 13)
      .AddTransition(13, 14);

  auto path = sg.FindPath(IndexToId(0), IndexToId(4));
  EXPECT_THAT(path.size(), Eq(size_t(4)));
  EXPECT_TRANSITION(path[0], 0, 1);
  EXPECT_TRANSITION(path[1], 1, 2);
  EXPECT_TRANSITION(path[2], 2, 3);
  EXPECT_TRANSITION(path[3], 3, 4);
}

TEST(StategraphTest, StraightLineWithCycles) {
  Stategraph sg;

  // Create a graph that looks like:
  //
  // [6]  [8]<-[12] [14]
  //  |    |    |    |
  // [5]<-[7]<-[11] [13]
  //  |    |    |    |
  // [0]<>[1]<>[2]<>[3]--[4]
  //       |
  //      [9]
  //       |
  //      [10]
  StategraphPopulator(&sg)
      .AddStates(15)
      .AddTransition(0, 1)  // main path
      .AddTransition(1, 2)
      .AddTransition(2, 3)
      .AddTransition(3, 4)
      .AddTransition(0, 5)  // branches
      .AddTransition(5, 6)
      .AddTransition(1, 7)
      .AddTransition(7, 8)
      .AddTransition(1, 9)
      .AddTransition(9, 10)
      .AddTransition(2, 11)
      .AddTransition(11, 12)
      .AddTransition(3, 13)
      .AddTransition(13, 14)
      .AddTransition(1, 0)  // cycles
      .AddTransition(2, 1)
      .AddTransition(3, 2)
      .AddTransition(7, 5)
      .AddTransition(11, 7)
      .AddTransition(12, 8);

  auto path = sg.FindPath(IndexToId(0), IndexToId(4));
  EXPECT_THAT(path.size(), Eq(size_t(4)));
  EXPECT_TRANSITION(path[0], 0, 1);
  EXPECT_TRANSITION(path[1], 1, 2);
  EXPECT_TRANSITION(path[2], 2, 3);
  EXPECT_TRANSITION(path[3], 3, 4);
}

TEST(StategraphTest, StraightLineWithCyclesAndTwoPaths) {
  Stategraph sg;

  // Create a graph that looks like:
  //
  // [6]  [8]<-[12] [14]
  //  |    |    |    |
  // [5]<-[7]<-[11] [13]
  //  |    |    |    |
  // [0]<>[1]<>[2]<>[3]--[4]
  //       | \           /
  //      [9] \_________/
  //       |
  //      [10]
  StategraphPopulator(&sg)
      .AddStates(15)
      .AddTransition(0, 1)  // main path
      .AddTransition(1, 2)
      .AddTransition(2, 3)
      .AddTransition(3, 4)
      .AddTransition(0, 5)  // branches
      .AddTransition(5, 6)
      .AddTransition(1, 7)
      .AddTransition(7, 8)
      .AddTransition(1, 9)
      .AddTransition(9, 10)
      .AddTransition(2, 11)
      .AddTransition(11, 12)
      .AddTransition(3, 13)
      .AddTransition(13, 14)
      .AddTransition(1, 0)  // cycles
      .AddTransition(2, 1)
      .AddTransition(3, 2)
      .AddTransition(7, 5)
      .AddTransition(11, 7)
      .AddTransition(12, 8)
      .AddTransition(1, 4);  // shortcut

  auto path = sg.FindPath(IndexToId(0), IndexToId(4));
  EXPECT_THAT(path.size(), Eq(size_t(2)));
  EXPECT_TRANSITION(path[0], 0, 1);
  EXPECT_TRANSITION(path[1], 1, 4);
}

TEST(StategraphDeathTest, InvalidState) {
  Stategraph sg;
  StategraphPopulator(&sg).AddStates(2).AddTransition(0, 1);

  PORT_EXPECT_DEBUG_DEATH(sg.FindPath(IndexToId(0), IndexToId(2)), "");
  PORT_EXPECT_DEBUG_DEATH(sg.FindPath(IndexToId(2), IndexToId(0)), "");
}

}  // namespace
}  // namespace lull
