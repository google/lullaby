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

#ifndef LULLABY_UTIL_ALIGNED_ALLOC_H_
#define LULLABY_UTIL_ALIGNED_ALLOC_H_

#ifdef _MSC_VER
#include <malloc.h>
#endif
#include <stdlib.h>
#include <algorithm>
#include <cstddef>

namespace lull {

// Allocates a block of memory of the given size and alignment.  This memory
// must be freed by calling lull::AlignedFree.
inline void* AlignedAlloc(size_t size, size_t align) {
  const size_t min_align = std::max(align, sizeof(max_align_t));
#ifdef _MSC_VER
  return _aligned_malloc(size, min_align);
#else
  void* ptr = nullptr;
  posix_memalign(&ptr, min_align, size);
  return ptr;
#endif
}

// Frees memory allocated using lull::AlignedAlloc.
inline void AlignedFree(void*ptr) {
#ifdef _MSC_VER
  _aligned_free(ptr);
#else
  free(ptr);
#endif
}

}  // namespace lull

#endif  // LULLABY_UTIL_ALIGNED_ALLOC_H_
