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

#include "lullaby/util/utf8_string.h"

#include <algorithm>

namespace lull {

UTF8String::UTF8String() {}

UTF8String::UTF8String(const char* cstr) : string_(cstr) {
  AppendOffsets(0, cstr);
}

UTF8String::UTF8String(std::string str) : string_(std::move(str)) {
  AppendOffsets(0, string_.c_str());
}

size_t UTF8String::OneCharLen(const char* p) {
  if (0xfc == (0xfe & *p)) {
    return 6;
  } else if (0xf8 == (0xfc & *p)) {
    return 5;
  } else if (0xf0 == (0xf8 & *p)) {
    return 4;
  } else if (0xe0 == (0xf0 & *p)) {
    return 3;
  } else if (0xc0 == (0xe0 & *p)) {
    return 2;
  }
  return 1;
}

void UTF8String::AppendOffsets(size_t offset, const char* cstr) {
  const char* p = cstr;
  while (*p) {
    char_offsets_.push_back(offset + p - cstr);
    p += OneCharLen(p);
  }
}

size_t UTF8String::CharSize() const {
  return char_offsets_.size();
}

size_t UTF8String::ByteSize() const {
  return string_.size();
}

void UTF8String::DeleteChars(size_t index, size_t count) {
  const size_t size = CharSize();
  if (index >= size) {
    return;
  }
  const size_t start = char_offsets_[index];
  size_t utf8_index = index;
  size_t num_utf8_chars = 0;
  size_t num_bytes = 0;
  while (utf8_index < size && num_utf8_chars < count) {
    const size_t offset = char_offsets_[utf8_index];
    num_bytes += OneCharLen(string_.c_str() + offset);
    ++num_utf8_chars;
    ++utf8_index;
  }
  string_.erase(start, num_bytes);
  // Remove the char offsets for the deleted characters and shift any offsets
  // after those that are being removed.
  const size_t delta_end_index =
      std::min(index + num_utf8_chars, char_offsets_.size() - 1);
  const size_t delta = char_offsets_[delta_end_index] - char_offsets_[index];
  for (size_t i = index + num_utf8_chars; i < char_offsets_.size(); ++i) {
    char_offsets_[i] -= delta;
  }
  char_offsets_.erase(char_offsets_.begin() + index,
                      char_offsets_.begin() + index + num_utf8_chars);
}

size_t UTF8String::Insert(size_t index, const std::string& str) {
  const size_t size = CharSize();
  if (index > size) {
    return 0;
  }

  const size_t start_offset = index < size ? char_offsets_[index] : ByteSize();
  size_t utf8_index = index;

  // Insert new offsets.
  const char* p = str.c_str();
  while (*p) {
    char_offsets_.insert(char_offsets_.begin() + utf8_index++,
                         start_offset + p - str.c_str());
    p += OneCharLen(p);
  }
  // Shift values after |str| insertion.
  while (utf8_index < char_offsets_.size()) {
    char_offsets_[utf8_index++] += str.size();
  }
  string_.insert(start_offset, str);

  return CharSize() - size;
}

void UTF8String::DeleteLast() {
  if (CharSize() == 0) {
    return;
  }
  const size_t offset = char_offsets_[CharSize() - 1];
  string_.resize(offset);
  char_offsets_.pop_back();
}

void UTF8String::Append(const std::string& str) {
  const size_t byte_len = ByteSize();
  string_.append(str);
  AppendOffsets(byte_len, str.c_str());
}

void UTF8String::Set(std::string str) {
  string_ = std::move(str);
  char_offsets_.clear();
  AppendOffsets(0, string_.c_str());
}

std::string UTF8String::CharAt(size_t index) const {
  if (index >= CharSize()) {
    return "";
  }

  size_t start = char_offsets_[index];
  size_t end = index == CharSize() - 1 ? ByteSize() : char_offsets_[index + 1];
  return string_.substr(start, end - start);
}

const char* UTF8String::c_str() const {
  return string_.c_str();
}

const std::string& UTF8String::str() const {
  return string_;
}

const bool UTF8String::empty() const {
  return string_.empty();
}

}  // namespace lull
