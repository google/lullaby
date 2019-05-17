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

#ifndef LULLABY_TOOLS_COMMON_LOG_H_
#define LULLABY_TOOLS_COMMON_LOG_H_

namespace lull {

void LogOpen(const char* log_file);

void LogClose();

void LogWrite(const char* format, ...);

}

#endif  // LULLABY_TOOLS_COMMON_LOG_H_
