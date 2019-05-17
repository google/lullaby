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

#ifndef LULLABY_TOOLS_SHADER_PIPELINE_BUILD_SHADER_H_
#define LULLABY_TOOLS_SHADER_PIPELINE_BUILD_SHADER_H_

#include <string>
#include <vector>

#include "flatbuffers/flatc.h"
#include "lullaby/util/expected.h"
#include "lullaby/util/span.h"
#include "lullaby/util/string_view.h"

namespace lull {
namespace tool {

/// Params for building a shader def binary.
struct ShaderBuildParams {
  // File path to the shader schema.
  string_view shader_schema_file_path;
  /// Vertex stage jsonnet files.
  Span<string_view> vertex_stages;
  /// Fragment stage jsonnet files.
  Span<string_view> fragment_stages;
};

/// Construct a shader def json string from ShaderBuildParams.
Expected<std::string> BuildShaderJsonString(const ShaderBuildParams& params);

// Construct a shader def flat buffer binary from a shader def json string.
flatbuffers::DetachedBuffer BuildFlatBufferFromShaderJsonString(
    const std::string& shader_json_string, const ShaderBuildParams& params);

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_SHADER_PIPELINE_BUILD_SHADER_H_
