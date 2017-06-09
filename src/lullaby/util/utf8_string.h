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

#ifndef LULLABY_UTIL_UTF8_STRING_H_
#define LULLABY_UTIL_UTF8_STRING_H_

#include <string>
#include <vector>

namespace lull {

// Some helper classes for working with UTF8 strings, making it easier to
// process the strings as character sequences, rather than byte sequences.
// UTF8String basically contains a string and a vector of character
// start positions.
class UTF8String {
 public:
  UTF8String();
  explicit UTF8String(const char* cstr);
  explicit UTF8String(std::string str);

  bool operator==(const UTF8String& rhs) const {
    return string_ == rhs.string_;
  }

  bool operator!=(const UTF8String& rhs) const {
    return string_ != rhs.string_;
  }

  // Gets UTF8 character count.
  size_t CharSize() const;
  // Gets byte size of the string.
  size_t ByteSize() const;
  // Deletes a UTF8 character at a given index. If index is out of range
  // this method becomes no-op.
  void DeleteChars(size_t index, size_t count);
  // Deletes a single UTF8 character at the end.
  void DeleteLast();
  // Appends text in the end.
  void Append(const std::string& str);
  // Appends text at the specified index. If index is out of range, this method
  // will noop. Returns UTF8 character count that has been inserted.
  size_t Insert(size_t index, const std::string& str);
  // Sets text.
  void Set(std::string str);
  // Gets a given UTF8 character at a given index. Returns empty if index is
  // out of bounds.
  std::string CharAt(size_t index) const;
  // Returns raw bytes of underlying string.
  const char* c_str() const;
  // Returns underlying std string.
  const std::string& str() const;
  // Returns true if empty.
  const bool empty() const;

 private:
  std::string string_;
  std::vector<size_t> char_offsets_;

  void AppendOffsets(size_t offset, const char* cstr);
  static size_t OneCharLen(const char* s);
};

}  // namespace lull

#endif  // LULLABY_UTIL_UTF8_STRING_H_
