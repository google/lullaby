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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/ecs/blueprint_reader.h"
#include "lullaby/modules/ecs/blueprint_tree.h"
#include "lullaby/modules/ecs/blueprint_writer.h"
#include "lullaby/modules/ecs/component_handlers.h"
#include "lullaby/tests/test_def_generated.h"

namespace lull {
namespace testing {

using ::testing::Eq;

TEST(BlueprintReader, ReadChildren) {
  ComponentHandlers handlers;
  handlers.RegisterComponentDefT<ValueDefT>();

  // Create a BlueprintTree with some children.
  BlueprintTree blueprint_root;
  {
    ValueDefT value;
    value.name = "root";
    blueprint_root.Write(&value);
  }
  BlueprintTree* blueprint_child = blueprint_root.NewChild();
  {
    ValueDefT value;
    value.name = "child";
    blueprint_child->Write(&value);
  }
  {
    BlueprintTree* blueprint_grandchild = blueprint_child->NewChild();
    ValueDefT value;
    value.name = "grandchild 1";
    blueprint_grandchild->Write(&value);
  }
  {
    BlueprintTree* blueprint_grandchild = blueprint_child->NewChild();
    ValueDefT value;
    value.name = "grandchild 2";
    blueprint_grandchild->Write(&value);
  }

  // Write it to a buffer then read it back out.
  BlueprintWriter writer(&handlers);
  BlueprintReader reader(&handlers);
  auto buffer = writer.WriteBlueprintTree(&blueprint_root);
  Optional<BlueprintTree> root =
      reader.ReadFlatbuffer({buffer.data(), buffer.size()});
  EXPECT_TRUE(root);

  // Check the root.
  {
    ValueDefT value;
    EXPECT_TRUE(root->Read(&value));
    EXPECT_THAT(value.name, Eq("root"));
    EXPECT_THAT(root->Children()->size(), Eq(1));
  }

  // Check the child.
  BlueprintTree& child = root->Children()->front();
  {
    ValueDefT value;
    EXPECT_TRUE(child.Read(&value));
    EXPECT_THAT(value.name, Eq("child"));
    EXPECT_THAT(child.Children()->size(), Eq(2));
  }

  // Check the grandchildren.
  {
    BlueprintTree& grandchild = child.Children()->front();
    ValueDefT value;
    EXPECT_TRUE(grandchild.Read(&value));
    EXPECT_THAT(value.name, Eq("grandchild 1"));
    EXPECT_THAT(grandchild.Children()->size(), Eq(0));
  }
  {
    BlueprintTree& grandchild = child.Children()->back();
    ValueDefT value;
    EXPECT_TRUE(grandchild.Read(&value));
    EXPECT_THAT(value.name, Eq("grandchild 2"));
    EXPECT_THAT(grandchild.Children()->size(), Eq(0));
  }
}

TEST(BlueprintReader, ReadComponents) {
  ComponentHandlers handlers;
  handlers.RegisterComponentDefT<ValueDefT>();
  handlers.RegisterComponentDefT<ComplexDefT>();

  // Create a BlueprintTree with some children.
  BlueprintTree blueprint_root;
  {
    ValueDefT value;
    value.name = "root 1";
    value.value = 101;
    blueprint_root.Write(&value);
    ComplexDefT complex;
    complex.name = "root 2";
    complex.data.value = 102;
    blueprint_root.Write(&complex);
  }
  {
    BlueprintTree* blueprint_child = blueprint_root.NewChild();
    ValueDefT value;
    value.name = "child 1";
    value.value = 201;
    blueprint_child->Write(&value);
    ComplexDefT complex;
    complex.name = "child 2";
    complex.data.value = 202;
    blueprint_child->Write(&complex);
  }

  // Write it to a buffer then read it back out.
  BlueprintWriter writer(&handlers);
  BlueprintReader reader(&handlers);
  auto buffer = writer.WriteBlueprintTree(&blueprint_root);
  Optional<BlueprintTree> root =
      reader.ReadFlatbuffer({buffer.data(), buffer.size()});
  EXPECT_TRUE(root);

  // Check the root.
  int count = 0;
  root->ForEachComponent([&](const Blueprint& bp) {
    if (count == 0) {
      EXPECT_TRUE(bp.Is<ValueDefT>());
      ValueDefT value;
      bp.Read(&value);
      EXPECT_THAT(value.name, Eq("root 1"));
      EXPECT_THAT(value.value, Eq(101));
    } else if (count == 1) {
      EXPECT_TRUE(bp.Is<ComplexDefT>());
      ComplexDefT complex;
      bp.Read(&complex);
      EXPECT_THAT(complex.name, Eq("root 2"));
      EXPECT_THAT(complex.data.value, Eq(102));
    }
    ++count;
  });
  EXPECT_THAT(count, Eq(2));
  EXPECT_THAT(root->Children()->size(), Eq(1));

  // Check the child.
  BlueprintTree& child = root->Children()->front();
  count = 0;
  child.ForEachComponent([&](const Blueprint& bp) {
    if (count == 0) {
      EXPECT_TRUE(bp.Is<ValueDefT>());
      ValueDefT value;
      bp.Read(&value);
      EXPECT_THAT(value.name, Eq("child 1"));
      EXPECT_THAT(value.value, Eq(201));
    } else if (count == 1) {
      EXPECT_TRUE(bp.Is<ComplexDefT>());
      ComplexDefT complex;
      bp.Read(&complex);
      EXPECT_THAT(complex.name, Eq("child 2"));
      EXPECT_THAT(complex.data.value, Eq(202));
    }
    ++count;
  });
  EXPECT_THAT(count, Eq(2));
  EXPECT_THAT(child.Children()->size(), Eq(0));
}

}  // namespace testing
}  // namespace lull
