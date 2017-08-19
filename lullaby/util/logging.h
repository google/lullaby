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

#ifndef LULLABY_UTIL_LOGGING_H_
#define LULLABY_UTIL_LOGGING_H_

#include <assert.h>
#include <iostream>
#include <sstream>

namespace lull {

// Provides an "empty" logger used for disabling logging while still supporting
// streaming message expressions.
class NullLogger {
 public:
  std::ostream& Get() {
    static std::ostream kNullStream(nullptr);
    return kNullStream;
  }
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

#define LULLABY_LOG(severity) ::lull::NullLogger().Get()

// If statement prevents unused variable warnings.
#define LULLABY_DCHECK(expr) \
  if (false && (expr))       \
    ;                        \
  else                       \
    ::lull::NullLogger().Get()

#define LULLABY_CHECK(expr)                                      \
  (expr) ? ::lull::NullLogger().Get()                            \
         : ::lull::FatalLogger(__FILE__, __LINE__, #expr).Get()

#define LULLABY_CHECK_NOTNULL(val) ::lull::CheckNotNull(val)

#ifndef LOG
#define LOG LULLABY_LOG
#endif

#ifndef DLOG
#define DLOG LULLABY_LOG
#endif

#ifndef LOG_ONCE
#define LOG_ONCE LULLABY_LOG
#endif

#ifndef DCHECK
#define DCHECK LULLABY_DCHECK
#define DCHECK_OP(val1, val2, op) DCHECK((val1) op (val2))
#define DCHECK_EQ(val1, val2) DCHECK_OP((val1), (val2), ==)
#define DCHECK_NE(val1, val2) DCHECK_OP((val1), (val2), !=)
#define DCHECK_LE(val1, val2) DCHECK_OP((val1), (val2), <=)
#define DCHECK_LT(val1, val2) DCHECK_OP((val1), (val2), <)
#define DCHECK_GE(val1, val2) DCHECK_OP((val1), (val2), >=)
#define DCHECK_GT(val1, val2) DCHECK_OP((val1), (val2), >)
#endif

#ifndef CHECK
#define CHECK LULLABY_CHECK
#define CHECK_OP(val1, val2, op) CHECK((val1) op (val2))
#define CHECK_EQ(val1, val2) CHECK_OP((val1), (val2), ==)
#define CHECK_NE(val1, val2) CHECK_OP((val1), (val2), !=)
#define CHECK_LE(val1, val2) CHECK_OP((val1), (val2), <=)
#define CHECK_LT(val1, val2) CHECK_OP((val1), (val2), <)
#define CHECK_GE(val1, val2) CHECK_OP((val1), (val2), >=)
#define CHECK_GT(val1, val2) CHECK_OP((val1), (val2), >)
#endif

#ifndef CHECK_NOTNULL
#define CHECK_NOTNULL LULLABY_CHECK_NOTNULL
#endif

#endif  // LULLABY_UTIL_LOGGING_H_
