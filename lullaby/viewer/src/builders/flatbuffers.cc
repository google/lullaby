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

#include "lullaby/viewer/src/builders/flatbuffers.h"

#include "flatbuffers/flatbuffers.h"
#include "lullaby/util/logging.h"

namespace lull {
namespace tool {

flatbuffers::DetachedBuffer ConvertJsonToFlatbuffer(const std::string& json,
                                                    const std::string& schema) {
  flatbuffers::IDLOptions opts;
  opts.generate_name_strings = true;
  opts.union_value_namespacing = false;
  flatbuffers::Parser parser(opts);
  parser.Parse(schema.c_str());
  parser.Parse(json.c_str());
  return parser.builder_.ReleaseBufferPointer();
}

}  // namespace tool
}  // namespace lull
