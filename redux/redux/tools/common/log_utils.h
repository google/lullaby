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

#ifndef REDUX_TOOLS_COMMON_LOG_UTILS_
#define REDUX_TOOLS_COMMON_LOG_UTILS_

#include <stdarg.h>
#include <stdio.h>

#include <ostream>

#include "redux/modules/base/hash.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/flatbuffers/common.h"
#include "redux/modules/flatbuffers/math.h"
#include "redux/modules/flatbuffers/var.h"
#include "redux/modules/graphics/enums.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/vector.h"

namespace redux {

// ostream operators for redux types used for logging.
inline std::ostream& operator<<(std::ostream& os, const HashValue& v) {
  return os << v.get();
}
inline std::ostream& operator<<(std::ostream& os, const vec2& v) {
  return os << v.x << "," << v.y;
}
inline std::ostream& operator<<(std::ostream& os, const vec2i& v) {
  return os << v.x << "," << v.y;
}
inline std::ostream& operator<<(std::ostream& os, const fbs::Vec2f& v) {
  return os << v.x() << "," << v.y();
}
inline std::ostream& operator<<(std::ostream& os, const fbs::Vec2i& v) {
  return os << v.x() << "," << v.y();
}
inline std::ostream& operator<<(std::ostream& os, const vec3& v) {
  return os << v.x << "," << v.y << "," << v.z;
}
inline std::ostream& operator<<(std::ostream& os, const vec3i& v) {
  return os << v.x << "," << v.y << "," << v.z;
}
inline std::ostream& operator<<(std::ostream& os, const fbs::Vec3f& v) {
  return os << v.x() << "," << v.y() << "," << v.z();
}
inline std::ostream& operator<<(std::ostream& os, const fbs::Vec3i& v) {
  return os << v.x() << "," << v.y() << "," << v.z();
}
inline std::ostream& operator<<(std::ostream& os, const vec4& v) {
  return os << v.x << "," << v.y << "," << v.z << "," << v.w;
}
inline std::ostream& operator<<(std::ostream& os, const vec4i& v) {
  return os << v.x << "," << v.y << "," << v.z << "," << v.w;
}
inline std::ostream& operator<<(std::ostream& os, const fbs::Vec4f& v) {
  return os << v.x() << "," << v.y() << "," << v.z() << "," << v.w();
}
inline std::ostream& operator<<(std::ostream& os, const fbs::Vec4i& v) {
  return os << v.x() << "," << v.y() << "," << v.z() << "," << v.w();
}
inline std::ostream& operator<<(std::ostream& os, const quat& v) {
  return os << v.x << "," << v.y << "," << v.z << "," << v.w;
}
inline std::ostream& operator<<(std::ostream& os, const fbs::Quatf& v) {
  return os << v.x() << "," << v.y() << "," << v.z() << "," << v.w();
}
inline std::ostream& operator<<(std::ostream& os, const fbs::Mat3x4f& v) {
  return os << v.col0() << " ; " << v.col1() << " ; " << v.col2() << " ; "
            << v.col3();
}
inline std::ostream& operator<<(std::ostream& os, const fbs::HashStringT& v) {
  return os << v.name;
}
inline std::ostream& operator<<(std::ostream& os, const fbs::HashVal& v) {
  return os << v.value();
}
template <typename T>
inline std::ostream& operator<<(std::ostream& os, const std::unique_ptr<T>& v) {
  if (v) {
    return os << *v;
  } else {
    return os << "[nil]";
  }
}
inline std::ostream& operator<<(std::ostream& os, const fbs::VarDefUnion& v) {
  switch (v.type) {
    case fbs::VarDef::DataBool:
      return os << v.AsDataBool()->value;
    case fbs::VarDef::DataInt:
      return os << v.AsDataInt()->value;
    case fbs::VarDef::DataFloat:
      return os << v.AsDataFloat()->value;
    case fbs::VarDef::DataString:
      return os << v.AsDataString()->value;
    case fbs::VarDef::DataHashVal:
      return os << v.AsDataHashVal()->value;
    case fbs::VarDef::DataHashString:
      return os << v.AsDataHashString()->value;
    case fbs::VarDef::DataVec2f:
      return os << v.AsDataVec2f()->value;
    case fbs::VarDef::DataVec2i:
      return os << v.AsDataVec2i()->value;
    case fbs::VarDef::DataVec3f:
      return os << v.AsDataVec3f()->value;
    case fbs::VarDef::DataVec3i:
      return os << v.AsDataVec3i()->value;
    case fbs::VarDef::DataVec4f:
      return os << v.AsDataVec4f()->value;
    case fbs::VarDef::DataVec4i:
      return os << v.AsDataVec4i()->value;
    case fbs::VarDef::DataQuatf:
      return os << v.AsDataQuatf()->value;
    case fbs::VarDef::DataBytes:
      return os << "[" << v.AsDataBytes()->value.size() << " bytes]";
    case fbs::VarDef::VarArrayDef:
      return os << "[array]";  // v.AsVarArrayDef()->value;
    case fbs::VarDef::VarTableDef:
      return os << "[table]";  // v.AsVarTableDef()->value;
    case fbs::VarDef::NONE:
      return os << "[nil]";
  }
  return os;
}

namespace tool {

struct LoggerOptions {
  LoggerOptions() = default;

  std::string logfile;
  bool log_to_stdout = false;
};

// A simple logging class that outputs to stdout and/or a log file.
class Logger {
 public:
  Logger(const LoggerOptions& opts = LoggerOptions());
  ~Logger();

  // Converts all the arguments into strings and appends them to the log. Usage
  // is something like;
  //   glog("The name of this object is: ", name);
  template <typename... Args>
  void operator()(Args&&... args) {
    int dummy[] = {
        (Append(std::forward<Args>(args)), 0)...,
    };
    (void)dummy;
    Flush();
  }

 private:
  void Flush();

  template <typename T>
  void Append(T&& arg) {
    ss_ << arg;
  }

  LoggerOptions opts_;
  FILE* file_ = nullptr;
  std::stringstream ss_;
};

}  // namespace tool
}  // namespace redux

#endif  // REDUX_TOOLS_COMMON_LOG_UTILS_
