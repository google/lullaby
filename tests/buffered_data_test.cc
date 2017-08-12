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

#include "lullaby/util/buffered_data.h"

#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {

using ::testing::Eq;
using ::testing::Not;

TEST(BufferedDataTest, SingleBufferSynchronousReadWrite) {
  // In this test we have only a single buffer but we use it synchronously, so
  // we always get up to date data and have no issues.
  BufferedData<int, 1> buffered_data;

  int* write_data = buffered_data.LockWriteBuffer();
  *write_data = 5;
  buffered_data.UnlockWriteBuffer();

  int* read_data = buffered_data.LockReadBuffer();
  EXPECT_THAT(*read_data, Eq(5));
  buffered_data.UnlockReadBuffer();

  write_data = buffered_data.LockWriteBuffer();
  *write_data = 2;
  buffered_data.UnlockWriteBuffer();

  read_data = buffered_data.LockReadBuffer();
  EXPECT_THAT(*read_data, Eq(2));
  buffered_data.UnlockReadBuffer();
}

TEST(BufferedDataTest, SingleBufferAsynchronousReadWrite) {
  // In this test we'll attempt locking the only buffer we have when it is
  // already locked and cause and assertion failure.
  BufferedData<int, 1> buffered_data;

  int* write_data = buffered_data.LockWriteBuffer();
  int* read_data = nullptr;

  // Expect death because the only buffer available is already locked.
  PORT_EXPECT_DEBUG_DEATH(read_data = buffered_data.LockReadBuffer(), "");

  *write_data = 5;
  buffered_data.UnlockWriteBuffer();

  // Expect death because the buffer was never locked as a read buffer.
  PORT_EXPECT_DEBUG_DEATH(buffered_data.UnlockReadBuffer(), "");
}

TEST(BufferedDataTest, MultiBufferSynchronousReadWrite) {
  // In this test we always finish updating the data before we lock it as the
  // read buffer.
  BufferedData<int, 2> buffered_data;

  int* write_data = buffered_data.LockWriteBuffer();
  *write_data = 5;
  buffered_data.UnlockWriteBuffer();

  int* read_data = buffered_data.LockReadBuffer();
  EXPECT_THAT(*read_data, Eq(5));
  buffered_data.UnlockReadBuffer();

  write_data = buffered_data.LockWriteBuffer();
  *write_data = 2;
  buffered_data.UnlockWriteBuffer();

  read_data = buffered_data.LockReadBuffer();
  EXPECT_THAT(*read_data, Eq(2));
  buffered_data.UnlockReadBuffer();
}

TEST(BufferedDataTest, MultiBufferAsynchronousReadWrite) {
  // In this test we will lock both buffers, write data, unlock and relock in a
  // way that we are able to get the up to date buffer as our read buffer.
  BufferedData<int, 2> buffered_data;

  // Initialize data into the buffers.
  int* write_data = buffered_data.LockWriteBuffer();
  *write_data = 0;
  buffered_data.UnlockWriteBuffer();
  write_data = buffered_data.LockWriteBuffer();
  *write_data = 0;
  buffered_data.UnlockWriteBuffer();

  write_data = buffered_data.LockWriteBuffer();
  int* read_data = buffered_data.LockReadBuffer();
  *write_data = 5;
  EXPECT_THAT(*read_data, Not(Eq(5)));
  buffered_data.UnlockWriteBuffer();
  buffered_data.UnlockReadBuffer();

  write_data = buffered_data.LockWriteBuffer();
  read_data = buffered_data.LockReadBuffer();
  *write_data = 5;
  EXPECT_THAT(*read_data, Eq(5));
  buffered_data.UnlockReadBuffer();
  buffered_data.UnlockWriteBuffer();
}

TEST(BufferedDataTest, MultiBufferAsynchronousReadWriteBlocked) {
  // In this test we will lock both buffers, write data to the write buffer, but
  // unlock and relock the two buffers in such a way that the read buffer lock
  // will never be able to get the updated buffer.
  BufferedData<int, 2> buffered_data;

  // Initialize data into the buffers.
  int* write_data = buffered_data.LockWriteBuffer();
  *write_data = 0;
  buffered_data.UnlockWriteBuffer();
  write_data = buffered_data.LockWriteBuffer();
  *write_data = 0;
  buffered_data.UnlockWriteBuffer();

  int* read_data = buffered_data.LockReadBuffer();
  write_data = buffered_data.LockWriteBuffer();
  *write_data = 5;
  buffered_data.UnlockReadBuffer();
  read_data = buffered_data.LockReadBuffer();
  EXPECT_THAT(*read_data, Not(Eq(5)));
  buffered_data.UnlockWriteBuffer();
  write_data = buffered_data.LockWriteBuffer();
  buffered_data.UnlockReadBuffer();
  read_data = buffered_data.LockReadBuffer();
  EXPECT_THAT(*read_data, Not(Eq(5)));
  buffered_data.UnlockReadBuffer();
  buffered_data.UnlockWriteBuffer();
}

TEST(BufferedDataTest, TripleBufferAsynchronousReadWrite) {
  // In this test we will have 3 buffers, so no matter however we lock them, we
  // should be able to get the most fresh data that is not being worked on.
  BufferedData<int, 3> buffered_data;

  // Write some data.
  int* write_data = buffered_data.LockWriteBuffer();
  *write_data = 1;
  buffered_data.UnlockWriteBuffer();
  write_data = buffered_data.LockWriteBuffer();
  *write_data = 2;
  buffered_data.UnlockWriteBuffer();
  write_data = buffered_data.LockWriteBuffer();
  *write_data = 3;
  buffered_data.UnlockWriteBuffer();

  // Expect most up to date data because all buffers are unlocked.
  int* read_data = buffered_data.LockReadBuffer();
  EXPECT_THAT(*read_data, Eq(3));

  // Write data twice, but read while writing the second time.
  write_data = buffered_data.LockWriteBuffer();
  *write_data = 4;
  buffered_data.UnlockWriteBuffer();
  write_data = buffered_data.LockWriteBuffer();
  *write_data = 5;
  buffered_data.UnlockReadBuffer();
  read_data = buffered_data.LockReadBuffer();
  // Expect slightly old data because newest buffer is still locked.
  EXPECT_THAT(*read_data, Eq(4));

  // Write more data, but let the read buffer be ready twice. Expect getting
  // the same data in both, which is the previous write that was finished.
  buffered_data.UnlockWriteBuffer();
  write_data = buffered_data.LockWriteBuffer();
  *write_data = 6;
  buffered_data.UnlockReadBuffer();
  read_data = buffered_data.LockReadBuffer();
  EXPECT_THAT(*read_data, Eq(5));
  buffered_data.UnlockReadBuffer();
  read_data = buffered_data.LockReadBuffer();
  EXPECT_THAT(*read_data, Eq(5));
  buffered_data.UnlockReadBuffer();
  buffered_data.UnlockWriteBuffer();
}

// Using template to accept N buffers.
template <size_t N>
void WriteThreadFunc(BufferedData<int, N>* buffered_data) {
  for (int i = 0; i < 1000; ++i) {
    int* write_data = buffered_data->LockWriteBuffer();
    *write_data = i;
    buffered_data->UnlockWriteBuffer();
  }
}

// Using template to accept N buffers.
template <size_t N>
void ProcessThreadFunc(BufferedData<int, N>* buffered_data) {
  for (int i = 0; i < 1000; ++i) {
    int* read_data = buffered_data->LockReadBuffer();
    *read_data += i;
    buffered_data->UnlockReadBuffer();
  }
}

TEST(BufferedDataTest, ThreadSanitizerTwoBuffers) {
  BufferedData<int, 2> buffered_data;

  std::thread write_thread(WriteThreadFunc<2>, &buffered_data);
  std::thread read_thread(ProcessThreadFunc<2>, &buffered_data);

  write_thread.join();
  read_thread.join();
}

TEST(BufferedDataTest, ThreadSanitizerThreeBuffers) {
  BufferedData<int, 3> buffered_data;

  std::thread write_thread(WriteThreadFunc<3>, &buffered_data);
  std::thread read_thread(ProcessThreadFunc<3>, &buffered_data);

  write_thread.join();
  read_thread.join();
}

}  // namespace lull
