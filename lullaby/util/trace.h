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

#ifndef LULLABY_UTIL_TRACE_H_
#define LULLABY_UTIL_TRACE_H_

#if defined(PROFILE_LULLABY)
// As Android devices have limited memory for a trace buffer we use the ALWAYS
// tag to ensure that we can capture as much of that data as possible by
// disabling every other category.
#define ION_ATRACE_TAG ION_ATRACE_TAG_APP
#include "ion/port/trace.h"
#define LULLABY_CPU_TRACE_CALL() ION_ATRACE_NAME(__PRETTY_FUNCTION__)
#define LULLABY_CPU_TRACE(name) ION_ATRACE_NAME(name)
#define LULLABY_CPU_TRACE_INT(name, value) ION_ATRACE_INT(name, value)
#else
#define LULLABY_CPU_TRACE_CALL()
#define LULLABY_CPU_TRACE(name)
#define LULLABY_CPU_TRACE_INT(name, value)
#endif

#endif  // LULLABY_UTIL_TRACE_H_
