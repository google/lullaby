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

#include "redux/tools/common/log_utils.h"

namespace redux::tool {

Logger::Logger(const LoggerOptions& opts) : opts_(opts) {
  if (!opts_.logfile.empty()) {
    file_ = fopen(opts_.logfile.c_str(), "wt");
  }
}

Logger::~Logger() {
  if (file_) {
    fclose(file_);
  }
}

void Logger::Flush() {
  const std::string str = ss_.str();
  if (file_) {
    fprintf(file_, "%s\n", str.c_str());
  }
  if (opts_.log_to_stdout) {
    fprintf(stderr, "%s\n", str.c_str());
  }
  ss_.str(std::string());
}

}  // namespace redux::tool
