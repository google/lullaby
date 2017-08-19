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

#ifndef LULLABY_UTIL_MACROS_H_
#define LULLABY_UTIL_MACROS_H_

// This is a fork of the CLANG_WARN_UNUSED_RESULT macro from util/macros.h for
// use in lullaby (which doesn't have a //base dependency).
#if defined(__clang__)
# if defined(LANG_CXX11) && __has_feature(cxx_attributes)
#  define LULLABY_WARN_UNUSED_RESULT [[clang::warn_unused_result]]
# else
#  define LULLABY_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
# endif
#else
# define LULLABY_WARN_UNUSED_RESULT
#endif

#endif  // LULLABY_UTIL_MACROS_H_
