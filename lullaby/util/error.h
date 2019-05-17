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

#ifndef LULLABY_UTIL_ERROR_H_
#define LULLABY_UTIL_ERROR_H_

#include <string>
#include "lullaby/util/string_view.h"
#include "lullaby/util/typeid.h"

namespace lull {

// This enum contains predefined error codes.  To represent a successful
// operation, the code kErrorCode_Ok should be used.  Custom error codes should
// be greater than or equal to kErrorCode_UserDefined.
enum ErrorCode {
  kErrorCode_Ok,
  kErrorCode_Cancelled,
  kErrorCode_Unknown,
  kErrorCode_InvalidArgument,
  kErrorCode_DeadlineExceeded,
  kErrorCode_NotFound,
  kErrorCode_AlreadyExists,
  kErrorCode_PermissionDenied,
  kErrorCode_ResourceExhausted,
  kErrorCode_FailedPrecondition,
  kErrorCode_Aborted,
  kErrorCode_OutOfRange,
  kErrorCode_Unimplemented,
  kErrorCode_Internal,
  kErrorCode_Unavailable,
  kErrorCode_DataLoss,
  kErrorCode_Unauthenticated,

  kErrorCode_UserDefined = 100,
};

// The Error class contains an integer error code and a string error message.
// Error messages will be compiled out in release builds, so for user readable
// error messages, this mechanism should not be used.
class Error {
 public:
  Error() = default;
  Error(ErrorCode error_code, string_view msg) : error_code_(error_code) {
#ifndef NDEBUG
    error_message_ = std::string(msg);
#endif
  }

  // Returns true if the error represents success (kErrorCode_Ok).
  bool Ok() const { return error_code_ == kErrorCode_Ok; }

  // Returns the error code.
  int GetErrorCode() const { return error_code_; }

  // Returns the error message, or empty string in release builds.
  const char* GetErrorMessage() const {
#ifndef NDEBUG
    return error_message_.c_str();
#else
    return "";
#endif
  }

 private:
  int error_code_ = kErrorCode_Ok;
#ifndef NDEBUG
  std::string error_message_;
#endif
};

#ifndef NDEBUG
#define LULL_ERROR(Code, Msg) ::lull::Error(Code, Msg)
#else
#define LULL_ERROR(Code, Msg) ::lull::Error(Code, "")
#endif

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ErrorCode);

#endif  // LULLABY_UTIL_ERROR_H_
