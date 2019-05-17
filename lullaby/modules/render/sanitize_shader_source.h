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

#ifndef LULLABY_MODULES_RENDER_SHADER_PREPROCESSOR_H_
#define LULLABY_MODULES_RENDER_SHADER_PREPROCESSOR_H_

#include <string>
#include "lullaby/util/span.h"
#include "lullaby/util/string_view.h"
#include "lullaby/generated/shader_def_generated.h"

namespace lull {

/// This function does several things to try to meet both GLSL and GLSL-ES
/// specs using the same source:
/// 1. Keep #version directives first, and translate version numbers between
///    the two standards since they use different numbering schemes.
/// 2. Add preprocessor definitions next so they can be used by the following
///    code.
/// 3. Identify the first non-empty, non-comment, non-preprocessor line, and
///    insert a default precision float specifier before it if necessary.
std::string SanitizeShaderSource(string_view code, ShaderLanguage language);

/// Converts shader version number from GLSL ES to GLSL.
int ConvertShaderVersionFromEsToCore(int version);
/// Converts shader version number from GLSL to GLSL ES.
int ConvertShaderVersionFromCoreToEs(int version);
/// Converts shader version from OpenGL Compat shader version.
int ConvertShaderVersionFromCompat(int version, ShaderLanguage to);

}  // namespace lull

#endif  // LULLABY_MODULES_RENDER_SHADER_PREPROCESSOR_H_
