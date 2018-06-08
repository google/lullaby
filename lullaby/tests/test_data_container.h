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

#ifndef LULLABY_TESTS_TEST_DATA_CONTAINER_H_
#define LULLABY_TESTS_TEST_DATA_CONTAINER_H_

#include <memory>

#include "lullaby/util/data_container.h"

namespace lull {
namespace testing {

// Helper methods for creating DataContainer instances with specific
// permissions and pre-populated data.
inline DataContainer CreateTestDataContainer(
    uint8_t* data, size_t capacity, DataContainer::AccessFlags access) {
  return DataContainer(
      DataContainer::DataPtr(
          data,
          [](const void* ptr) { delete[] static_cast<const uint8_t*>(ptr); }),
      capacity, access);
}

inline DataContainer CreateTestDataContainer(
    size_t capacity, DataContainer::AccessFlags access) {
  return CreateTestDataContainer(new uint8_t[capacity], capacity, access);
}

inline DataContainer CreateReadDataContainer(size_t capacity) {
  return CreateTestDataContainer(capacity, DataContainer::AccessFlags::kRead);
}

inline DataContainer CreateReadDataContainer(uint8_t* data, size_t capacity) {
  return CreateTestDataContainer(data, capacity,
                                 DataContainer::AccessFlags::kRead);
}

inline DataContainer CreateWriteDataContainer(size_t capacity) {
  return CreateTestDataContainer(capacity, DataContainer::AccessFlags::kWrite);
}

}  // namespace testing
}  // namespace lull

#endif  // LULLABY_TESTS_TEST_DATA_CONTAINER_H_
