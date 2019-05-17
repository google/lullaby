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
#include "flatbuffers/flatbuffers.h"
#include "lullaby/generated/flatbuffers/blueprint_def_generated.h"
#include "lullaby/modules/ecs/blueprint_tree.h"
#include "lullaby/modules/ecs/blueprint_writer.h"
#include "lullaby/modules/ecs/component_handlers.h"
#include "lullaby/util/hash.h"
#include "lullaby/tests/test_def_generated.h"

namespace lull {
namespace testing {

using ::testing::Eq;
using ::testing::Gt;
using ::testing::IsNull;

TEST(BlueprintWriter, WriteChildren) {
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

  // Write it into a flatbuffer binary.
  BlueprintWriter writer(&handlers);
  auto buffer = writer.WriteBlueprintTree(&blueprint_root);
  EXPECT_THAT(buffer.size(), Gt(0));
  {
    flatbuffers::Verifier verifier(buffer.data(), buffer.size());
    EXPECT_TRUE(verifier.VerifyBuffer<BlueprintDef>());
  }

  // Check the root.
  const BlueprintDef* root = flatbuffers::GetRoot<BlueprintDef>(buffer.data());
  EXPECT_THAT(root->children()->size(), Eq(1));
  EXPECT_THAT(root->components()->size(), Eq(1));
  {
    EXPECT_THAT(root->components()->Get(0)->type(), Eq(Hash("ValueDef")));
    flatbuffers::Verifier verifier(root->components()->Get(0)->def()->Data(),
                                   root->components()->Get(0)->def()->size());
    EXPECT_TRUE(verifier.VerifyBuffer<ValueDef>());
    const ValueDef* value = flatbuffers::GetRoot<ValueDef>(
        root->components()->Get(0)->def()->Data());
    EXPECT_THAT(value->name()->str(), Eq("root"));
  }

  // Check the child.
  const BlueprintDef* child = root->children()->Get(0);
  EXPECT_THAT(child->children()->size(), Eq(2));
  EXPECT_THAT(child->components()->size(), Eq(1));
  {
    EXPECT_THAT(child->components()->Get(0)->type(), Eq(Hash("ValueDef")));
    flatbuffers::Verifier verifier(child->components()->Get(0)->def()->Data(),
                                   child->components()->Get(0)->def()->size());
    EXPECT_TRUE(verifier.VerifyBuffer<ValueDef>());
    const ValueDef* value = flatbuffers::GetRoot<ValueDef>(
        child->components()->Get(0)->def()->Data());
    EXPECT_THAT(value->name()->str(), Eq("child"));
  }

  // Check the grandchildren.
  {
    const BlueprintDef* grandchild = child->children()->Get(0);
    EXPECT_THAT(grandchild->children(), IsNull());
    EXPECT_THAT(grandchild->components()->size(), Eq(1));
    EXPECT_THAT(grandchild->components()->Get(0)->type(), Eq(Hash("ValueDef")));
    flatbuffers::Verifier verifier(
        grandchild->components()->Get(0)->def()->Data(),
        grandchild->components()->Get(0)->def()->size());
    EXPECT_TRUE(verifier.VerifyBuffer<ValueDef>());
    const ValueDef* value = flatbuffers::GetRoot<ValueDef>(
        grandchild->components()->Get(0)->def()->Data());
    EXPECT_THAT(value->name()->str(), Eq("grandchild 1"));
  }
  {
    const BlueprintDef* grandchild = child->children()->Get(1);
    EXPECT_THAT(grandchild->children(), IsNull());
    EXPECT_THAT(grandchild->components()->size(), Eq(1));
    EXPECT_THAT(grandchild->components()->Get(0)->type(), Eq(Hash("ValueDef")));
    flatbuffers::Verifier verifier(
        grandchild->components()->Get(0)->def()->Data(),
        grandchild->components()->Get(0)->def()->size());
    EXPECT_TRUE(verifier.VerifyBuffer<ValueDef>());
    const ValueDef* value = flatbuffers::GetRoot<ValueDef>(
        grandchild->components()->Get(0)->def()->Data());
    EXPECT_THAT(value->name()->str(), Eq("grandchild 2"));
  }
}

TEST(BlueprintWriter, WriteComponents) {
  ComponentHandlers handlers;
  handlers.RegisterComponentDefT<ValueDefT>();
  handlers.RegisterComponentDefT<ComplexDefT>();

  // Create a BlueprintTree with some components.
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

  // Write it into a flatbuffer binary.
  BlueprintWriter writer(&handlers);
  auto buffer = writer.WriteBlueprintTree(&blueprint_root);
  EXPECT_THAT(buffer.size(), Gt(0));
  {
    flatbuffers::Verifier verifier(buffer.data(), buffer.size());
    EXPECT_TRUE(verifier.VerifyBuffer<BlueprintDef>());
  }

  // Check the root.
  const BlueprintDef* root = flatbuffers::GetRoot<BlueprintDef>(buffer.data());
  EXPECT_THAT(root->children()->size(), Eq(1));
  EXPECT_THAT(root->components()->size(), Eq(2));
  {
    EXPECT_THAT(root->components()->Get(0)->type(), Eq(Hash("ValueDef")));
    flatbuffers::Verifier verifier(root->components()->Get(0)->def()->Data(),
                                   root->components()->Get(0)->def()->size());
    EXPECT_TRUE(verifier.VerifyBuffer<ValueDef>());
    const ValueDef* value = flatbuffers::GetRoot<ValueDef>(
        root->components()->Get(0)->def()->Data());
    EXPECT_THAT(value->name()->str(), Eq("root 1"));
    EXPECT_THAT(value->value(), Eq(101));
  }
  {
    EXPECT_THAT(root->components()->Get(1)->type(), Eq(Hash("ComplexDef")));
    flatbuffers::Verifier verifier(root->components()->Get(1)->def()->Data(),
                                   root->components()->Get(1)->def()->size());
    EXPECT_TRUE(verifier.VerifyBuffer<ValueDef>());
    const ComplexDef* complex = flatbuffers::GetRoot<ComplexDef>(
        root->components()->Get(1)->def()->Data());
    EXPECT_THAT(complex->name()->str(), Eq("root 2"));
    EXPECT_THAT(complex->data()->value(), Eq(102));
  }

  // Check the child.
  const BlueprintDef* child = root->children()->Get(0);
  EXPECT_THAT(child->children(), IsNull());
  EXPECT_THAT(child->components()->size(), Eq(2));
  {
    EXPECT_THAT(child->components()->Get(0)->type(), Eq(Hash("ValueDef")));
    flatbuffers::Verifier verifier(child->components()->Get(0)->def()->Data(),
                                   child->components()->Get(0)->def()->size());
    EXPECT_TRUE(verifier.VerifyBuffer<ValueDef>());
    const ValueDef* value = flatbuffers::GetRoot<ValueDef>(
        child->components()->Get(0)->def()->Data());
    EXPECT_THAT(value->name()->str(), Eq("child 1"));
    EXPECT_THAT(value->value(), Eq(201));
  }
  {
    EXPECT_THAT(child->components()->Get(1)->type(), Eq(Hash("ComplexDef")));
    flatbuffers::Verifier verifier(child->components()->Get(1)->def()->Data(),
                                   child->components()->Get(1)->def()->size());
    EXPECT_TRUE(verifier.VerifyBuffer<ValueDef>());
    const ComplexDef* complex = flatbuffers::GetRoot<ComplexDef>(
        child->components()->Get(1)->def()->Data());
    EXPECT_THAT(complex->name()->str(), Eq("child 2"));
    EXPECT_THAT(complex->data()->value(), Eq(202));
  }
}

}  // namespace testing
}  // namespace lull
