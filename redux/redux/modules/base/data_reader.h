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

#ifndef REDUX_MODULES_BASE_DATA_READER_H_
#define REDUX_MODULES_BASE_DATA_READER_H_

#include <cstdint>
#include <functional>

#include "absl/types/span.h"

namespace redux {

// An abstraction over a sequence of bytes with a read-only streaming-like API.
//
// It uses std::function to type-erase the actual object containing the byte
// sequence. This `DataStream` abstraction need to support read, seek, and close
// functionality using a single API. The DataReader class then provides a more
// extensive and easy-to-use API over this single API.
class DataReader {
 public:
  // The type of streaming operation to perform. See DataStream for more
  // information.
  enum class Operation {
    kSeek,
    kSeekFromHead,
    kSeekFromEnd,
    kRead,
    kClose,
  };

  // Seek:
  //   Advances the "read head" by `num` bytes. Returns the new position of the
  //   "read head" within the stream.
  //
  // SeekFromHead:
  //   Seets the "read head" to a position `num` bytes from the start of the
  //   source data. Returns the new position of the "read head" within the
  //   stream.
  //
  // SeekFromEnd:
  //   Seets the "read head" to a position `num` bytes from the end of the
  //   source data. Returns the new position of the "read head" within the
  //   stream.
  //
  // Read:
  //   Attempts to read `num` bytes from the source into the given `buffer`.
  //   Returns the actual number of bytes read.
  //
  // Close:
  //   Closes the underlying data source, preventing any further operations.
  using DataStream =
      std::function<std::size_t(Operation op, int64_t num, void* buffer)>;

  explicit DataReader(DataStream stream = nullptr);

  DataReader(const DataReader&) = delete;
  DataReader& operator=(const DataReader&) = delete;

  DataReader(DataReader&& rhs) noexcept;
  DataReader& operator=(DataReader&& rhs) noexcept;

  ~DataReader();

  // Returns true if the DataReader is backed by an active data source.
  bool IsOpen() const;

  // Closes the underlying data source, preventing further reads.
  void Close();

  // Reads the next `num_bytes` of data from the data source into the `buffer`.
  // Returns the actualy number of bytes read.
  std::size_t Read(std::byte* buffer, std::size_t num_bytes);

  // A void* override of the `Read` function above.
  std::size_t Read(void* buffer, std::size_t num_bytes);

  // Returns the total size of the underlying data source. May return
  // std::numeric_limits<size>t>::max if length is unknown.
  std::size_t GetTotalLength() const;

  // Returns the current byte offset of the stream from the start.
  std::size_t GetCurrentPosition() const;

  // Sets the position at which the next read will occur. Returns the actual
  // position that was set.
  std::size_t SetCurrentPosition(std::size_t position);

  // Moves the byte stream ahead by the given offset. Returns the actual number
  // of bytes moved.
  std::size_t Advance(std::size_t offset);

  // Returns true if the byte stream is at the end of the buffer.
  bool IsAtEndOfStream() const;

  // Creates a DataReader around a C FILE object. Note: The reader will take
  // ownership of the FILE object, i.e. calling Reader::Close() will close the
  // file.
  static DataReader FromCFile(FILE* file);

  // Creates a DataReader around a span of bytes. Assumes that the lifetime of
  // the ByteSpan will outlive the reader itself.
  static DataReader FromByteSpan(absl::Span<const std::byte> bytes);

 private:
  std::size_t InvokeHandler(Operation op, int64_t num = 0,
                            void* buffer = nullptr) const;

  DataStream stream_;
  std::size_t length_ = 0;
};

}  // namespace redux

#endif  // REDUX_MODULES_BASE_DATA_READER_H_
