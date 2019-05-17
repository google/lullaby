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

#include "lullaby/tools/common/log.h"

#include <stdio.h>
#include <stdarg.h>

namespace lull {

namespace {
  FILE* g_log_handle = nullptr;
}

void LogOpen(const char* log_file) {
  g_log_handle = fopen(log_file, "wt");
}

void LogClose() {
  if(g_log_handle == nullptr) {
    return;
  }

  fclose(g_log_handle);
  g_log_handle = nullptr;
}

void LogWrite(const char* format, ...) {
  if(g_log_handle == nullptr) {
    return;
  }

  va_list arglist;
  va_start(arglist, format);
  vfprintf(g_log_handle, format, arglist);
  va_end(arglist);
}

}
