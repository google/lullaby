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
#include "redux/systems/transform/transform_system.h"

namespace redux {
namespace {

using ::testing::Eq;

class TransformSystemTest : public testing::Test {
 protected:
  void SetUp() override {
    ScriptEngine::Create(&registry_);
    transform_system_ = registry_.Create<TransformSystem>(&registry_);
    registry_.Initialize();
    script_engine_ = registry_.Get<ScriptEngine>();
  }

  Registry registry_;
  TransformSystem* transform_system_;
  ScriptEngine* script_engine_;
};

TEST_F(TransformSystemTest, NoTransform) {
  const Entity entity(123);
  Transform transform;
  EXPECT_THAT(transform_system_->GetTranslation(entity),
              Eq(transform.translation));
  EXPECT_THAT(transform_system_->GetRotation(entity), Eq(transform.rotation));
  EXPECT_THAT(transform_system_->GetScale(entity), Eq(transform.scale));
}

TEST_F(TransformSystemTest, SetTransform) {
  const Entity entity(123);

  Transform transform;
  transform.translation = vec3(1.f, 2.f, 3.f);
  transform.rotation = quat(0.f, 0.f, 0.f, 1.f);
  transform.scale = vec3(2.f, 3.f, 4.f);
  transform_system_->SetTransform(entity, transform);

  EXPECT_THAT(transform_system_->GetTranslation(entity),
              Eq(transform.translation));
  EXPECT_THAT(transform_system_->GetRotation(entity), Eq(transform.rotation));
  EXPECT_THAT(transform_system_->GetScale(entity), Eq(transform.scale));
}

TEST_F(TransformSystemTest, SetTranslation) {
  const Entity entity(123);

  Transform transform;
  transform.translation = vec3(1.f, 2.f, 3.f);
  transform_system_->SetTranslation(entity, transform.translation);

  EXPECT_THAT(transform_system_->GetTranslation(entity),
              Eq(transform.translation));
}

TEST_F(TransformSystemTest, SetRotation) {
  const Entity entity(123);

  Transform transform;
  transform.rotation = quat(0.f, 0.f, 0.f, 1.f);
  transform_system_->SetRotation(entity, transform.rotation);

  EXPECT_THAT(transform_system_->GetRotation(entity), Eq(transform.rotation));
}

TEST_F(TransformSystemTest, SetScale) {
  const Entity entity(123);

  Transform transform;
  transform.scale = vec3(2.f, 3.f, 4.f);
  transform_system_->SetScale(entity, transform.scale);

  EXPECT_THAT(transform_system_->GetScale(entity), Eq(transform.scale));
}

TEST_F(TransformSystemTest, SetBox) {
  const Entity entity(123);
  transform_system_->SetTransform(entity, Transform());

  const vec3 min(-1.f, -2.f, -3.f);
  const vec3 max(1.f, 2.f, 3.f);
  transform_system_->SetBox(entity, Box(min, max));

  EXPECT_THAT(transform_system_->GetEntityAlignedBox(entity).min, Eq(min));
  EXPECT_THAT(transform_system_->GetEntityAlignedBox(entity).max, Eq(max));
}

TEST_F(TransformSystemTest, Flags) {
  const Entity entity(123);
  transform_system_->SetTransform(entity, Transform());

  const auto flag1 = transform_system_->RequestFlag();
  const auto flag2 = transform_system_->RequestFlag();

  EXPECT_THAT(transform_system_->HasFlag(entity, flag1), Eq(false));
  EXPECT_THAT(transform_system_->HasFlag(entity, flag2), Eq(false));

  transform_system_->SetFlag(entity, flag1);
  EXPECT_THAT(transform_system_->HasFlag(entity, flag1), Eq(true));
  EXPECT_THAT(transform_system_->HasFlag(entity, flag2), Eq(false));

  transform_system_->SetFlag(entity, flag2);
  EXPECT_THAT(transform_system_->HasFlag(entity, flag1), Eq(true));
  EXPECT_THAT(transform_system_->HasFlag(entity, flag2), Eq(true));

  transform_system_->ClearFlag(entity, flag1);
  EXPECT_THAT(transform_system_->HasFlag(entity, flag1), Eq(false));
  EXPECT_THAT(transform_system_->HasFlag(entity, flag2), Eq(true));

  transform_system_->ClearFlag(entity, flag2);
  EXPECT_THAT(transform_system_->HasFlag(entity, flag1), Eq(false));
  EXPECT_THAT(transform_system_->HasFlag(entity, flag2), Eq(false));

  transform_system_->SetFlag(entity, flag1);
  transform_system_->SetFlag(entity, flag2);
  transform_system_->RemoveTransform(entity);
  EXPECT_THAT(transform_system_->HasFlag(entity, flag1), Eq(false));
  EXPECT_THAT(transform_system_->HasFlag(entity, flag2), Eq(false));
}

TEST_F(TransformSystemTest, TooManyFlags) {
  for (int i = 0; i < 31; ++i) {
    transform_system_->RequestFlag();
  }
  EXPECT_DEATH(transform_system_->RequestFlag(), "");
}

}  // namespace
}  // namespace redux
