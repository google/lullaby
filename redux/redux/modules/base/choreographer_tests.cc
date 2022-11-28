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
#include "redux/modules/base/choreographer.h"

namespace redux {
namespace {

using ::testing::Eq;

struct Tracker {
  void Track(std::string_view name) {
    ordered_calls.push_back(std::string(name));
    calls.emplace(std::string(name));
  }
  std::vector<std::string> ordered_calls;
  absl::flat_hash_set<std::string> calls;
};

struct TestObject {
  explicit TestObject(Tracker& tracker) : tracker(tracker) {}
  void Step(absl::Duration dt) { tracker.Track("TestObject::Step"); }

  Tracker& tracker;
};

struct TestObjectNoDt {
  explicit TestObjectNoDt(Tracker& tracker) : tracker(tracker) {}
  void Step() { tracker.Track("TestObjectNoDt::Step"); }
  Tracker& tracker;
};

}  // namespace
}  // namespace redux
REDUX_SETUP_TYPEID(redux::TestObject);
REDUX_SETUP_TYPEID(redux::TestObjectNoDt);
namespace redux {
namespace {

TEST(ChoreographerTest, NoEdges) {
  Tracker tracker;
  Registry registry;
  registry.Create<TestObject>(tracker);
  registry.Create<TestObjectNoDt>(tracker);

  Choreographer choreo(&registry);
  choreo.Add<&TestObject::Step>(Choreographer::Stage::kPrologue);
  choreo.Add<&TestObjectNoDt::Step>(Choreographer::Stage::kPrologue);

  choreo.Step(absl::ZeroDuration());
  EXPECT_TRUE(tracker.calls.contains("TestObject::Step"));
  EXPECT_TRUE(tracker.calls.contains("TestObjectNoDt::Step"));
}

TEST(ChoreographerTest, Before) {
  Tracker tracker;
  Registry registry;
  registry.Create<TestObject>(tracker);
  registry.Create<TestObjectNoDt>(tracker);

  Choreographer choreo(&registry);
  choreo.Add<&TestObject::Step>(Choreographer::Stage::kPrologue)
      .Before<&TestObjectNoDt::Step>();
  choreo.Add<&TestObjectNoDt::Step>(Choreographer::Stage::kPrologue);

  choreo.Step(absl::ZeroDuration());
  EXPECT_THAT(tracker.ordered_calls.size(), Eq(2));
  EXPECT_THAT(tracker.ordered_calls[0], Eq("TestObject::Step"));
  EXPECT_THAT(tracker.ordered_calls[1], Eq("TestObjectNoDt::Step"));
}

TEST(ChoreographerTest, After) {
  Tracker tracker;
  Registry registry;
  registry.Create<TestObject>(tracker);
  registry.Create<TestObjectNoDt>(tracker);

  Choreographer choreo(&registry);
  choreo.Add<&TestObject::Step>(Choreographer::Stage::kPrologue)
      .After<&TestObjectNoDt::Step>();
  choreo.Add<&TestObjectNoDt::Step>(Choreographer::Stage::kPrologue);

  choreo.Step(absl::ZeroDuration());
  EXPECT_THAT(tracker.ordered_calls.size(), Eq(2));
  EXPECT_THAT(tracker.ordered_calls[0], Eq("TestObjectNoDt::Step"));
  EXPECT_THAT(tracker.ordered_calls[1], Eq("TestObject::Step"));
}

}  // namespace
}  // namespace redux
