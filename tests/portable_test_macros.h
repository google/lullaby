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

#ifndef LULLABY_TESTS_PORTABLE_TEST_MACROS_H_
#define LULLABY_TESTS_PORTABLE_TEST_MACROS_H_

#define PORT_EXPECT_DEATH(expr, msg) EXPECT_DEATH_IF_SUPPORTED(expr, msg)

// msvc fastbuilds are classified as NDEBUG, which is inconsistent with other
// platforms.  Confusingly, ION_DEBUG is still defined for them.
#if !defined(NDEBUG) || (ION_PLATFORM_WINDOWS && ION_DEBUG)
#define PORT_EXPECT_DEBUG_DEATH(expr, msg) EXPECT_DEATH_IF_SUPPORTED(expr, msg)
#else  // NDEBUG
#define PORT_EXPECT_DEBUG_DEATH(expr, msg) (void) msg; expr
#endif  // NDEBUG ... else ...

#endif  // LULLABY_TESTS_PORTABLE_TEST_MACROS_H_
