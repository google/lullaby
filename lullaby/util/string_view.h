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

#ifndef LULLABY_UTIL_STRING_VIEW_H_
#define LULLABY_UTIL_STRING_VIEW_H_

#include <algorithm>
#include <cstring>
#include <ostream>
#include <string>

namespace lull {

// A string_view points to a sequence of characters contained in a const char*,
// or a std::string. It does not own the memory, and is not null-terminated.
// This class cannot modify the characters it points to.

// It should be used in place of std::string when writing functions that take a
// string as an argument. That way the user can pass in either a std::string or
// string literal, without incurring the cost of building a new std::string.
// Non-explicit single argument constructors are used to automatically convert
// a const char* or std::string to a string_view.

// This class implements a strict subset of C++17's std::string_view, so that
// once that class is standardized we can easily switch to it.
class string_view {
 public:
  static constexpr size_t npos = size_t(-1);

  // Constructors.
  string_view() : str_(nullptr), len_(0) {}
  string_view(const string_view& str) = default;
  string_view(const char* str)
      : str_(str), len_(std::strlen(str)) {}
  string_view(const char* str, size_t len) : str_(str), len_(len) {}
  string_view(const std::string& str)
      : str_(str.data()), len_(str.size()) {}

  // Returns the number of characters in the string_view.
  size_t size() const { return len_; }
  size_t length() const { return len_; }

  // Returns whether the string_view is empty.
  bool empty() const { return len_ == 0; }

  // Returns a character from the string_view. Does not do bounds checking.
  const char& operator[](size_t i) const { return str_[i]; }

  // Get the raw data of the string_view. Note: This char* will not necessarily
  // be null-terminated.
  const char* data() const { return str_; }

  // Iteration methods.
  const char* begin() const { return str_; }
  const char* end() const { return str_ + len_; }

  // Convert to a std::string. This will make a copy of the memory.
  std::string to_string() const {
    if (str_ == nullptr) {
      return std::string();
    }
    return std::string(str_, len_);
  }
  explicit operator std::string() const { return to_string(); }

  // Returns a string_view pointing to a sub string of this string_view. The new
  // string view will point to the same chunk of memory, no copy is made.
  string_view substr(size_t pos, size_t n = std::string::npos) const {
    pos = std::min(pos, len_);
    n = std::min(n, len_ - pos);
    return string_view(str_ + pos, n);
  }

  // Returns <0 if this < str, 0 if this == str, and >0 if this > str.
  int compare(string_view str) const {
    int cmp = memcmp(str_, str.str_, std::min(len_, str.len_));
    if (cmp < 0) {
      return -1;
    }
    if (cmp > 0) {
      return 1;
    }
    if (len_ < str.len_) {
      return -1;
    }
    if (len_ > str.len_) {
      return 1;
    }
    return 0;
  }

 private:
  const char* str_;
  size_t len_;
};

inline bool operator==(string_view str1, string_view str2) {
  return str1.compare(str2) == 0;
}

inline bool operator!=(string_view str1, string_view str2) {
  return str1.compare(str2) != 0;
}

inline bool operator<(string_view str1, string_view str2) {
  return str1.compare(str2) < 0;
}

inline bool operator<=(string_view str1, string_view str2) {
  return str1.compare(str2) <= 0;
}

inline bool operator>(string_view str1, string_view str2) {
  return str1.compare(str2) > 0;
}

inline bool operator>=(string_view str1, string_view str2) {
  return str1.compare(str2) >= 0;
}

inline std::ostream& operator<<(std::ostream& o, const string_view& str) {
  return o.write(str.data(), str.size());
}

}  // namespace lull

#endif  // LULLABY_UTIL_STRING_VIEW_H_
