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

#ifndef REDUX_TOOLS_COMMON_FLATBUFFER_UTILS_
#define REDUX_TOOLS_COMMON_FLATBUFFER_UTILS_

#include "flatbuffers/flatc.h"
#include "redux/modules/base/data_container.h"

namespace redux::tool {

// Convert a `json` string to a flatbuffer binary.
flatbuffers::DetachedBuffer JsonToFlatbuffer(const char* json,
                                             const char* schema_file,
                                             const char* schema_name);

// Writes a flatbuffer native object into a flatbuffer binary blob.
template <typename T>
DataContainer BuildFlatbuffer(const T& def) {
  flatbuffers::FlatBufferBuilder* fbb = new flatbuffers::FlatBufferBuilder();
  auto deleter = [=](const std::byte* ptr) { delete fbb; };

  using Table = typename T::TableType;
  fbb->Finish(Table::Pack(*fbb, &def, nullptr));

  const std::byte* buffer =
      reinterpret_cast<const std::byte*>(fbb->GetBufferPointer());
  DataContainer::DataPtr ptr(buffer, deleter);
  return DataContainer(std::move(ptr), fbb->GetSize());
}

}  // namespace redux::tool

#endif  // REDUX_TOOLS_COMMON_FLATBUFFER_UTILS_
