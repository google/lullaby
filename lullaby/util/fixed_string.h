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

#ifndef LULLABY_UTIL_FIXED_STRING_H_
#define LULLABY_UTIL_FIXED_STRING_H_

#include <stdarg.h>
#include <algorithm>
#include <cstring>
#include <iterator>
#include <ostream>
#include <string>

#include "lullaby/util/hash.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/string_view.h"

namespace lull {

// A string that has a fixed length provided via template.
//
// Owns fixed sized char array of capacity N + 1 for a null terminating
// character. Template N specifies the maximum number of non null terminated
// characters this string can hold.
template <size_t N>
class FixedString {
 public:
  using iterator = char*;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_iterator = const char*;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  FixedString() : len_(0) { str_[0] = '\0'; }
  FixedString(const FixedString<N>& str) = default;
  FixedString(string_view str) { assign(str); }
  FixedString(const char* str) { assign(str); }

  iterator begin() const { return const_cast<char*>(str_); }
  iterator end() const { return const_cast<char*>(str_ + len_); }
  reverse_iterator rbegin() const {
    return reverse_iterator(const_cast<char*>(str_ + len_));
  }
  reverse_iterator rend() const {
    return reverse_iterator(const_cast<char*>(str_));
  }

  const_iterator cbegin() const { return str_; }
  const_iterator cend() const { return str_ + len_; }
  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(str_ + len_);
  }
  const_reverse_iterator crend() const { return const_reverse_iterator(str_); }

  size_t size() const { return len_; }
  size_t length() const { return len_; }
  bool empty() const { return len_ == 0; }
  size_t max_size() const { return N; }
  size_t capacity() const { return kCapacity; }
  void clear() {
    str_[0] = '\0';
    len_ = 0;
  }

  const char& operator[](size_t i) const {
    if (i >= kCapacity) {
      LOG(WARNING) << "Index out of bounds!";
      return str_[len_];
    }
    return str_[i];
  }

  const char& at(size_t i) const {
    if (i >= kCapacity) {
      LOG(WARNING) << "Index out of bounds!";
      return str_[len_];
    }
    return str_[i];
  }

  const char& front() const { return str_[0]; }
  const char& back() const { return len_ == 0 ? str_[0] : str_[len_ - 1]; }

  void assign(string_view str) {
    len_ = str.size();
    if (len_ > N) {
      LOG(ERROR) << "Cannot exceed max number of non null chars " << N;
      len_ = N;
    }
    strncpy(str_, str.data(), len_);
    str_[len_] = '\0';
  }

  void append(string_view str) {
    size_t strlength = str.length();
    if (strlength + len_ > kCapacity) {
      LOG(ERROR) << "Exceeded max number of non null chars " << N
                 << ". String will be trimmed.";
      strlength = kCapacity - len_ - 1;
    }
    memcpy(str_ + len_, str.data(), strlength);
    len_ += strlength;
    str_[len_] = '\0';
  }

  void push_back(char c) {
    if (len_ + 1 >= kCapacity) {
      LOG(ERROR) << "Cannot exceed max num of non null chars " << N;
      return;
    }
    if (c == '\0') {
      return;
    }
    str_[len_] = c;
    len_++;
    str_[len_] = '\0';
  }

  const char* data() const { return str_; }
  std::string to_string() const { return std::string(str_, len_); }
  string_view to_string_view() const { return string_view(str_, len_); }
  explicit operator std::string() const { return to_string(); }
  operator string_view() const {
    return to_string_view();
  }

  string_view substr(size_t pos, size_t n = N) const {
    pos = std::min(pos, len_);
    const size_t substr_len = len_ - pos;
    n = std::min(n, substr_len);
    return string_view(str_ + pos, substr_len);
  }

  int compare(string_view str) const {
    if (len_ < str.size()) {
      return -1;
    }
    if (len_ > str.size()) {
      return 1;
    }
    const int cmp = strncmp(str_, str.data(), std::min(len_, str.size()));
    if (cmp < 0) {
      return -1;
    }
    if (cmp > 0) {
      return 1;
    }
    return 0;
  }

  void format(const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    len_ = vsnprintf(str_, N, msg, args);
    str_[len_] = '\0';
    va_end(args);
  }

  FixedString<N>& operator=(string_view str) {
    assign(str);
    return *this;
  }

  FixedString<N>& operator=(const char* str) {
    assign(str);
    return *this;
  }

  template <size_t T>
  FixedString<N>& operator+=(const FixedString<T>& str) {
    append(str.data());
    return *this;
  }

  FixedString<N>& operator+=(string_view str) {
    append(str);
    return *this;
  }

  struct Hash {
    std::size_t operator()(const lull::FixedString<N>& target) const {
      return lull::Hash(target.data(), target.size());
    }
  };

 private:
  static const size_t kCapacity = N + 1;
  char str_[kCapacity];
  size_t len_;
};

template <size_t N, size_t T>
inline auto operator+(const FixedString<N>& lhs, const FixedString<T>& rhs)
    -> FixedString<N + T> {
  FixedString<N + T> res;
  res.append(lhs.data());
  res.append(rhs.data());
  return res;
}

template <size_t N>
inline bool operator==(const FixedString<N>& lhs, const FixedString<N>& rhs) {
  return lhs.compare(rhs.data()) == 0;
}
template <size_t N, size_t T>
inline bool operator==(const FixedString<N>& lhs, const FixedString<T>& rhs) {
  return lhs.compare(rhs.data()) == 0;
}
template <size_t N>
inline bool operator==(const FixedString<N>& lhs, string_view rhs) {
  return lhs.compare(rhs) == 0;
}

template <size_t N>
inline bool operator!=(const FixedString<N>& lhs, const FixedString<N>& rhs) {
  return lhs.compare(rhs.data()) != 0;
}
template <size_t N, size_t T>
inline bool operator!=(const FixedString<N>& lhs, const FixedString<T>& rhs) {
  return lhs.compare(rhs.data()) != 0;
}
template <size_t N>
inline bool operator!=(const FixedString<N>& lhs, string_view rhs) {
  return lhs.compare(rhs) != 0;
}

template <size_t N>
inline bool operator<(const FixedString<N>& lhs, const FixedString<N>& rhs) {
  return lhs.compare(rhs.data()) < 0;
}
template <size_t N, size_t T>
inline bool operator<(const FixedString<N>& lhs, const FixedString<T>& rhs) {
  return lhs.compare(rhs.data()) < 0;
}
template <size_t N>
inline bool operator<(const FixedString<N>& lhs, string_view rhs) {
  return lhs.compare(rhs) < 0;
}

template <size_t N>
inline bool operator<=(const FixedString<N>& lhs, const FixedString<N>& rhs) {
  return lhs.compare(rhs.data()) <= 0;
}
template <size_t N, size_t T>
inline bool operator<=(const FixedString<N>& lhs, const FixedString<T>& rhs) {
  return lhs.compare(rhs.data()) <= 0;
}
template <size_t N>
inline bool operator<=(const FixedString<N>& lhs, string_view rhs) {
  return lhs.compare(rhs) <= 0;
}

template <size_t N>
inline bool operator>(const FixedString<N>& lhs, const FixedString<N>& rhs) {
  return lhs.compare(rhs.data()) > 0;
}
template <size_t N, size_t T>
inline bool operator>(const FixedString<N>& lhs, const FixedString<T>& rhs) {
  return lhs.compare(rhs.data()) > 0;
}
template <size_t N>
inline bool operator>(const FixedString<N>& lhs, string_view rhs) {
  return lhs.compare(rhs) > 0;
}

template <size_t N>
inline bool operator>=(const FixedString<N>& lhs, const FixedString<N>& rhs) {
  return lhs.compare(rhs.data()) >= 0;
}
template <size_t N, size_t T>
inline bool operator>=(const FixedString<N>& lhs, const FixedString<T>& rhs) {
  return lhs.compare(rhs.data()) >= 0;
}
template <size_t N>
inline bool operator>=(const FixedString<N>& lhs, string_view rhs) {
  return lhs.compare(rhs) >= 0;
}

template <size_t N>
inline std::ostream& operator<<(std::ostream& o, const FixedString<N>& str) {
  return o.write(str.data(), str.size());
}

}  // namespace lull

#endif  // LULLABY_UTIL_FIXED_STRING_H_
