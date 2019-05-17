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

#ifndef LULLABY_UTIL_LOGGING_H_
#define LULLABY_UTIL_LOGGING_H_

#include <assert.h>
#include <iostream>
#include <sstream>

namespace lull {

enum LogSeverity {
  INFO,
  WARNING,
  ERROR,
  FATAL,
  DFATAL,
};

// Provides an "empty" logger used for disabling logging while still supporting
// streaming message expressions.
class NullLogger {
 public:
  std::ostream& Get() {
    static std::ostream kNullStream(nullptr);
    return kNullStream;
  }
};

// Provides a logger that writes to std::cout.
class SimpleLogger {
 public:
  explicit SimpleLogger(LogSeverity severity) : severity_(severity) {}

  ~SimpleLogger() {
    std::cerr << str_.str() << std::endl;
    if (severity_ == FATAL) {
      assert(false);
    }
#ifndef NDEBUG
    if (severity_ == DFATAL) {
      assert(false);
    }
#endif
  }

  std::ostream& Get() { return str_; }

 private:
  LogSeverity severity_ = INFO;
  std::ostringstream str_;
};

// Provides a logger that writes to std::cerr and aborts execution.
class FatalLogger {
 public:
  FatalLogger(const char* file, int line, const char* expression) {
    str_ << file << ":" << line << ", Failed: " << expression << std::endl;
  }

  ~FatalLogger() {
    std::cerr << str_.str() << std::endl;
    assert(false);
  }

  std::ostream& Get() { return str_; }

 private:
  std::ostringstream str_;
};

// Implements CHECK_NOTNULL().
template <typename T>
T CheckNotNull(T&& t) {
  assert(t != nullptr);
  return std::forward<T>(t);
}

}  // namespace lull

#ifndef LOG
#define LOG(severity) ::lull::SimpleLogger(::lull::severity).Get()
#endif

#ifndef DLOG
#ifndef NDEBUG
#define DLOG(severity) LOG(severity)
#else
#define DLOG(severity) ::lull::NullLogger().Get()
#endif
#endif

#ifndef LOG_ONCE
#define LOG_ONCE DLOG
#endif

#ifndef CHECK_NOTNULL
#define CHECK_NOTNULL(val) ::lull::CheckNotNull(val)
#endif

#ifndef CHECK
#define CHECK(expr) \
  (expr) ? ::lull::NullLogger().Get() \
         : ::lull::FatalLogger(__FILE__, __LINE__, #expr).Get()

#define CHECK_OP(val1, val2, op) CHECK((val1) op (val2))
#define CHECK_EQ(val1, val2) CHECK_OP((val1), (val2), ==)
#define CHECK_NE(val1, val2) CHECK_OP((val1), (val2), !=)
#define CHECK_LE(val1, val2) CHECK_OP((val1), (val2), <=)
#define CHECK_LT(val1, val2) CHECK_OP((val1), (val2), <)
#define CHECK_GE(val1, val2) CHECK_OP((val1), (val2), >=)
#define CHECK_GT(val1, val2) CHECK_OP((val1), (val2), >)
#endif


#ifndef DCHECK
#ifndef NDEBUG
#define DCHECK(expr) CHECK(expr)
#else
#define DCHECK(expr) ::lull::NullLogger().Get()
#endif

#define DCHECK_OP(val1, val2, op) DCHECK((val1) op (val2))
#define DCHECK_EQ(val1, val2) DCHECK_OP((val1), (val2), ==)
#define DCHECK_NE(val1, val2) DCHECK_OP((val1), (val2), !=)
#define DCHECK_LE(val1, val2) DCHECK_OP((val1), (val2), <=)
#define DCHECK_LT(val1, val2) DCHECK_OP((val1), (val2), <)
#define DCHECK_GE(val1, val2) DCHECK_OP((val1), (val2), >=)
#define DCHECK_GT(val1, val2) DCHECK_OP((val1), (val2), >)
#endif

#endif  // LULLABY_UTIL_LOGGING_H_
