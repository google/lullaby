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

#include "lullaby/util/inward_buffer.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

using ::testing::Eq;

TEST(InwardBuffer, InitialState) {
  InwardBuffer buffer(32);
  EXPECT_THAT(buffer.FrontSize(), Eq(0u));
  EXPECT_THAT(buffer.BackSize(), Eq(0u));
}

TEST(InwardBuffer, WriteFront) {
  InwardBuffer buffer(32);
  const char data[] = "hi";
  buffer.WriteFront(data, sizeof(data));

  EXPECT_THAT(buffer.FrontSize(), Eq(3u));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(0)), Eq('h'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(1)), Eq('i'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(2)), Eq('\0'));
}

TEST(InwardBuffer, WriteFrontT) {
  InwardBuffer buffer(32);
  const uint8_t data[] = {0x12, 0x34, 0x56, 0x78};
  buffer.WriteFront(data[0]);
  buffer.WriteFront(data[1]);
  buffer.WriteFront(data[2]);
  buffer.WriteFront(data[3]);

  EXPECT_THAT(buffer.FrontSize(), Eq(sizeof(data)));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(0)), Eq(data[0]));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(1)), Eq(data[1]));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(2)), Eq(data[2]));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(3)), Eq(data[3]));
}

TEST(InwardBuffer, AllocFront) {
  InwardBuffer buffer(32);
  const char data[] = "hi";

  void* dst = buffer.AllocFront(sizeof(data));
  memcpy(dst, data, sizeof(data));

  EXPECT_THAT(buffer.FrontSize(), Eq(3u));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(0)), Eq('h'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(1)), Eq('i'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(2)), Eq('\0'));
}

TEST(InwardBuffer, EraseFront) {
  InwardBuffer buffer(32);
  const char data[] = "hello";
  buffer.WriteFront(data, sizeof(data));

  EXPECT_THAT(buffer.FrontSize(), Eq(6u));
  buffer.EraseFront(3);
  EXPECT_THAT(buffer.FrontSize(), Eq(3u));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(0)), Eq('h'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(1)), Eq('e'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(2)), Eq('l'));
}

TEST(InwardBuffer, FrontAt) {
  InwardBuffer buffer(32);
  const char data[] = "hi";
  buffer.WriteFront(data, sizeof(data));

  EXPECT_THAT(buffer.FrontSize(), Eq(3u));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(0)), Eq('h'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(1)), Eq('i'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(2)), Eq('\0'));

  const InwardBuffer* const_buffer = &buffer;
  EXPECT_THAT(*reinterpret_cast<const uint8_t*>(const_buffer->FrontAt(0)),
              Eq('h'));
  EXPECT_THAT(*reinterpret_cast<const uint8_t*>(const_buffer->FrontAt(1)),
              Eq('i'));
  EXPECT_THAT(*reinterpret_cast<const uint8_t*>(const_buffer->FrontAt(2)),
              Eq('\0'));
}

TEST(InwardBuffer, WriteBack) {
  InwardBuffer buffer(32);
  const char data[] = "hi";
  buffer.WriteBack(data, sizeof(data));

  EXPECT_THAT(buffer.BackSize(), Eq(3u));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(1)), Eq('\0'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(2)), Eq('i'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(3)), Eq('h'));
}

TEST(InwardBuffer, WriteBackT) {
  InwardBuffer buffer(32);
  const uint8_t data[] = {0x12, 0x34, 0x56, 0x78};
  buffer.WriteBack(data[0]);
  buffer.WriteBack(data[1]);
  buffer.WriteBack(data[2]);
  buffer.WriteBack(data[3]);

  EXPECT_THAT(buffer.BackSize(), Eq(sizeof(data)));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(1)), Eq(data[0]));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(2)), Eq(data[1]));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(3)), Eq(data[2]));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(4)), Eq(data[3]));
}

TEST(InwardBuffer, AllocBack) {
  InwardBuffer buffer(32);
  const char data[] = "hi";

  void* dst = buffer.AllocBack(sizeof(data));
  memcpy(dst, data, sizeof(data));

  EXPECT_THAT(buffer.BackSize(), Eq(3u));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(1)), Eq('\0'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(2)), Eq('i'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(3)), Eq('h'));
}

TEST(InwardBuffer, EraseBack) {
  InwardBuffer buffer(32);
  const char data[] = "hello";
  buffer.WriteBack(data, sizeof(data));

  EXPECT_THAT(buffer.BackSize(), Eq(6u));
  buffer.EraseBack(3);
  EXPECT_THAT(buffer.BackSize(), Eq(3u));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(1)), Eq('\0'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(2)), Eq('o'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(3)), Eq('l'));
}

TEST(InwardBuffer, BackAt) {
  InwardBuffer buffer(32);
  const char data[] = "hi";
  buffer.WriteBack(data, sizeof(data));

  EXPECT_THAT(buffer.BackSize(), Eq(3u));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(1)), Eq('\0'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(2)), Eq('i'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(3)), Eq('h'));

  const InwardBuffer* const_buffer = &buffer;
  EXPECT_THAT(*reinterpret_cast<const uint8_t*>(const_buffer->BackAt(1)),
              Eq('\0'));
  EXPECT_THAT(*reinterpret_cast<const uint8_t*>(const_buffer->BackAt(2)),
              Eq('i'));
  EXPECT_THAT(*reinterpret_cast<const uint8_t*>(const_buffer->BackAt(3)),
              Eq('h'));
}

TEST(InwardBuffer, MixedWrite) {
  InwardBuffer buffer(32);
  const char data[] = "hi";
  buffer.WriteFront(data, sizeof(data));
  buffer.WriteBack(data, sizeof(data));

  EXPECT_THAT(buffer.FrontSize(), Eq(3u));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(0)), Eq('h'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(1)), Eq('i'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(2)), Eq('\0'));
  EXPECT_THAT(buffer.BackSize(), Eq(3u));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(1)), Eq('\0'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(2)), Eq('i'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(3)), Eq('h'));
}

TEST(InwardBuffer, Realloc) {
  InwardBuffer buffer(4);
  const char front_data[] = "hello";
  const char back_data[] = "world";
  buffer.WriteFront(front_data, sizeof(front_data));
  buffer.WriteBack(back_data, sizeof(back_data));

  EXPECT_THAT(buffer.FrontSize(), Eq(6u));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(0)), Eq('h'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(1)), Eq('e'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(2)), Eq('l'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(3)), Eq('l'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(4)), Eq('o'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.FrontAt(5)), Eq('\0'));
  EXPECT_THAT(buffer.BackSize(), Eq(6u));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(1)), Eq('\0'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(2)), Eq('d'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(3)), Eq('l'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(4)), Eq('r'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(5)), Eq('o'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(buffer.BackAt(6)), Eq('w'));
}

TEST(InwardBuffer, MoveConstructor) {
  InwardBuffer buffer(32);
  const char data[] = "hi";
  buffer.WriteFront(data, sizeof(data));

  InwardBuffer other(std::move(buffer));

  EXPECT_THAT(buffer.FrontSize(), Eq(0u));
  EXPECT_THAT(other.FrontSize(), Eq(3u));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(other.FrontAt(0)), Eq('h'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(other.FrontAt(1)), Eq('i'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(other.FrontAt(2)), Eq('\0'));
}

TEST(InwardBuffer, MoveAssign) {
  InwardBuffer buffer(32);
  const char data[] = "hi";
  buffer.WriteFront(data, sizeof(data));

  InwardBuffer other(16);
  other = std::move(buffer);

  EXPECT_THAT(buffer.FrontSize(), Eq(0u));
  EXPECT_THAT(other.FrontSize(), Eq(3u));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(other.FrontAt(0)), Eq('h'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(other.FrontAt(1)), Eq('i'));
  EXPECT_THAT(*reinterpret_cast<uint8_t*>(other.FrontAt(2)), Eq('\0'));
}

}  // namespace
}  // namespace lull
