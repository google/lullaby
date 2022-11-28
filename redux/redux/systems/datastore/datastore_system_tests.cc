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
#include "redux/systems/datastore/datastore_system.h"

namespace redux {
namespace {

using ::testing::Eq;

class DatastoreSystemTest : public testing::Test {
 protected:
  void SetUp() override {
    ScriptEngine::Create(&registry_);
    datastore_system_ = registry_.Create<DatastoreSystem>(&registry_);
    registry_.Initialize();
    script_engine_ = registry_.Get<ScriptEngine>();
  }

  Registry registry_;
  DatastoreSystem* datastore_system_;
  ScriptEngine* script_engine_;
};

TEST_F(DatastoreSystemTest, GetValue) {
  const Entity entity(123);
  const HashValue key = ConstHash("key");

  datastore_system_->Add(entity, key, 456.0f);
  const Var value = datastore_system_->GetValue(entity, key);
  EXPECT_THAT(value.ValueOr(0.0f), Eq(456.0f));
}

TEST_F(DatastoreSystemTest, Remove) {
  const Entity entity(123);
  const HashValue key = ConstHash("key");

  datastore_system_->Add(entity, key, 456.0f);
  Var value = datastore_system_->GetValue(entity, key);
  EXPECT_THAT(value.ValueOr(0.0f), Eq(456.0f));

  datastore_system_->Remove(entity, key);
  value = datastore_system_->GetValue(entity, key);
  EXPECT_TRUE(value.Empty());
}

TEST_F(DatastoreSystemTest, RemoveAll) {
  const Entity entity(123);
  const HashValue key1 = ConstHash("key1");
  const HashValue key2 = ConstHash("key2");

  datastore_system_->Add(entity, key1, 456);
  datastore_system_->Add(entity, key2, 789.0f);
  Var value1 = datastore_system_->GetValue(entity, key1);
  Var value2 = datastore_system_->GetValue(entity, key2);
  EXPECT_THAT(value1.ValueOr(0), Eq(456));
  EXPECT_THAT(value2.ValueOr(0.0f), Eq(789.0f));

  datastore_system_->RemoveAll(entity);
  value1 = datastore_system_->GetValue(entity, key1);
  value2 = datastore_system_->GetValue(entity, key2);
  EXPECT_TRUE(value1.Empty());
  EXPECT_TRUE(value2.Empty());
}

TEST_F(DatastoreSystemTest, SetFromDatastoreDef) {
  const Entity entity(123);
  const HashValue key1 = ConstHash("key1");
  const HashValue key2 = ConstHash("key2");

  DatastoreDef def;
  def.data.Insert(key1, 456);
  def.data.Insert(key2, 789.0f);
  datastore_system_->SetFromDatastoreDef(entity, def);

  const Var value1 = datastore_system_->GetValue(entity, key1);
  const Var value2 = datastore_system_->GetValue(entity, key2);
  EXPECT_THAT(value1.ValueOr(0), Eq(456));
  EXPECT_THAT(value2.ValueOr(0.0f), Eq(789.0f));
}

TEST_F(DatastoreSystemTest, AddFromScript) {
  Var result =
      script_engine_->RunNow("(rx.Datastore.Add (entity 123) :key 456.0f)");

  const Entity entity(123);
  const HashValue key = Hash("key");

  result = datastore_system_->GetValue(entity, key);
  EXPECT_THAT(result.ValueOr(0.f), Eq(456.0f));

  result = script_engine_->RunNow("(rx.Datastore.Get (entity 123) :key)");
  EXPECT_THAT(result.ValueOr(0.f), Eq(456.0f));

  result = script_engine_->RunNow("(rx.Datastore.Remove (entity 123) :key)");
  result = datastore_system_->GetValue(entity, key);
  EXPECT_TRUE(result.Empty());
}

}  // namespace
}  // namespace redux
