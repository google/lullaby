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

#include "redux/modules/base/data_reader.h"

#include <algorithm>

#include "redux/modules/base/logging.h"

namespace redux {

DataReader::DataReader(DataStream stream) : stream_(std::move(stream)) {
  length_ = InvokeHandler(Operation::kSeekFromEnd);
  InvokeHandler(Operation::kSeekFromHead);
}

DataReader::DataReader(DataReader&& rhs) noexcept
    : stream_(std::move(rhs.stream_)), length_(rhs.length_) {
  rhs.stream_ = nullptr;
  rhs.length_ = 0;
}

DataReader& DataReader::operator=(DataReader&& rhs) noexcept {
  Close();

  stream_ = std::move(rhs.stream_);
  length_ = rhs.length_;
  rhs.stream_ = nullptr;
  rhs.length_ = 0;
  return *this;
}

DataReader::~DataReader() { Close(); }

bool DataReader::IsOpen() const { return stream_ != nullptr; }

void DataReader::Close() {
  if (stream_) {
    InvokeHandler(Operation::kClose);
    stream_ = nullptr;
    length_ = 0;
  }
}

std::size_t DataReader::Read(std::byte* buffer, std::size_t num_bytes) {
  CHECK(IsOpen());
  return InvokeHandler(Operation::kRead, num_bytes, buffer);
}

std::size_t DataReader::Read(void* buffer, std::size_t num_bytes) {
  return Read(reinterpret_cast<std::byte*>(buffer), num_bytes);
}

std::size_t DataReader::GetTotalLength() const { return length_; }

std::size_t DataReader::GetCurrentPosition() const {
  CHECK(IsOpen());
  return InvokeHandler(Operation::kSeek);
}

std::size_t DataReader::SetCurrentPosition(std::size_t position) {
  CHECK(IsOpen());
  return InvokeHandler(Operation::kSeekFromHead, position);
}

std::size_t DataReader::Advance(std::size_t offset) {
  CHECK(IsOpen());
  std::size_t curr = GetCurrentPosition();
  return InvokeHandler(Operation::kSeek, offset) - curr;
}

bool DataReader::IsAtEndOfStream() const {
  CHECK(IsOpen());
  return GetCurrentPosition() == GetTotalLength();
}

std::size_t DataReader::InvokeHandler(Operation op, int64_t num,
                                      void* buffer) const {
  return stream_ != nullptr ? stream_(op, num, buffer) : 0;
}

DataReader DataReader::FromCFile(FILE* file) {
  if (file == nullptr) {
    return DataReader();
  }

  auto handler = [file](DataReader::Operation op, int64_t num,
                        void* buffer) mutable -> std::size_t {
    switch (op) {
      case DataReader::Operation::kRead: {
        auto result = fread(buffer, 1, num, file);
        return result;
      }
      case DataReader::Operation::kSeek: {
        fseek(file, num, SEEK_CUR);
        break;
      }
      case DataReader::Operation::kSeekFromHead: {
        fseek(file, num, SEEK_SET);
        break;
      }
      case DataReader::Operation::kSeekFromEnd: {
        fseek(file, num, SEEK_END);
        break;
      }
      case DataReader::Operation::kClose: {
        if (file) {
          fclose(file);
          file = nullptr;
        }
        break;
      }
    }
    return file ? ftell(file) : 0;
  };
  return DataReader(handler);
}

DataReader DataReader::FromByteSpan(absl::Span<const std::byte> bytes) {
  std::size_t offset = 0;
  auto handler = [bytes, offset](Operation op, int64_t num,
                                 void* buffer) mutable -> std::size_t {
    switch (op) {
      case Operation::kRead: {
        int64_t bytes_left = static_cast<int64_t>(bytes.size() - offset);
        num = std::clamp<int64_t>(num, 0L, bytes_left);
        memcpy(buffer, bytes.data() + offset, num);
        offset += num;
        return static_cast<std::size_t>(num);
      }
      case Operation::kSeek: {
        offset += num;
        break;
      }
      case Operation::kSeekFromHead: {
        offset = num;
        break;
      }
      case Operation::kSeekFromEnd: {
        offset = bytes.size() + num;
        break;
      }
      case Operation::kClose: {
        offset = 0;
        break;
      }
    }
    offset = std::clamp(offset, std::size_t(0), bytes.size());
    return offset;
  };

  return DataReader(handler);
}

}  // namespace redux
