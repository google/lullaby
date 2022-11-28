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
#include "redux/engines/script/redux/script_types.h"
#include "redux/modules/ecs/blueprint_factory.h"
#include "redux/modules/var/var.h"
#include "redux/modules/var/var_array.h"
#include "redux/modules/var/var_table.h"

namespace redux {
namespace {

using ::testing::Eq;

TEST(BlueprintFactoryTest, Basic) {
  const char* txt =
      "{"
      "  'TransformDef': {"
      "    'position': (vec3 0 2 0),"
      "    'box': {"
      "      'min': (vec3 -1 -1 -1),"
      "      'max': (vec3  1  1  1),"
      "    },"
      "  },"
      "  'RenderDef': {"
      "    'shading_model': 'flat',"
      "  },"
      "  'RigidBodyDef': {"
      "    'mass': 5,"
      "    'friction': 0.5,"
      "    'restitution': 0.5,"
      "  },"
      "  'SphereShapeDef': {"
      "    'radius': 2,"
      "  },"
      "}";

#define EXPECT_TYPE(Obj, Type)                              \
  EXPECT_THAT(Obj[ConstHash("$type")].ValueOr(HashValue()), \
              Eq(ConstHash(Type)));

  BlueprintFactory factory(nullptr);

  const BlueprintPtr blueprint = factory.ReadBlueprint(txt);
  EXPECT_THAT(blueprint->GetNumComponents(), Eq(4));

  const Var& transform = blueprint->GetComponent(0);
  EXPECT_TRUE(transform.Is<VarTable>());
  EXPECT_TYPE(transform, "TransformDef");
  EXPECT_TRUE(transform[ConstHash("position")].Is<ScriptValue>());
  EXPECT_TRUE(transform[ConstHash("box")].Is<VarTable>());
  EXPECT_TRUE(transform[ConstHash("box")][ConstHash("min")].Is<ScriptValue>());
  EXPECT_TRUE(transform[ConstHash("box")][ConstHash("max")].Is<ScriptValue>());

  const Var& render = blueprint->GetComponent(1);
  EXPECT_TRUE(render.Is<VarTable>());
  EXPECT_TYPE(render, "RenderDef");
  EXPECT_TRUE(render[ConstHash("shading_model")].Is<std::string>());

  const Var& rigid_body = blueprint->GetComponent(2);
  EXPECT_TRUE(rigid_body.Is<VarTable>());
  EXPECT_TYPE(rigid_body, "RigidBodyDef");
  EXPECT_TRUE(rigid_body[ConstHash("mass")].Is<double>());
  EXPECT_TRUE(rigid_body[ConstHash("friction")].Is<double>());
  EXPECT_TRUE(rigid_body[ConstHash("restitution")].Is<double>());

  const Var& shape = blueprint->GetComponent(3);
  EXPECT_TRUE(shape[ConstHash("$type")].Is<HashValue>());
  EXPECT_TYPE(shape, "SphereShapeDef");
  EXPECT_TRUE(shape.Is<VarTable>());
  EXPECT_TRUE(shape[ConstHash("radius")].Is<double>());
}


}  // namespace
}  // namespace redux
