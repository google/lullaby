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

#include "lullaby/modules/render/sanitize_shader_source.h"

#include "lullaby/util/logging.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

TEST(SanitizeShaderSourceTest, DefaultPrecisionSpecifier) {
  const char* src = "void main() { gl_FragColor = vec4(0); }";

  const std::string gles = SanitizeShaderSource(src, ShaderLanguage_GLSL_ES);
  const std::string core = SanitizeShaderSource(src, ShaderLanguage_GLSL);

  // The source should appear unaltered.
  EXPECT_NE(gles.find(src), std::string::npos);
  EXPECT_NE(gles.find(src), std::string::npos);

  // A precision specifier should be added to GL gles shaders and should appear
  // before any other code.
  EXPECT_NE(gles.find("precision"), std::string::npos);
  EXPECT_LT(gles.find("precision"), gles.find("void"));

  // No precision specifier is needed for CORE shaders.
  EXPECT_EQ(core.find("precision"), std::string::npos);
}

TEST(SanitizeShaderSourceTest, VersionFirst) {
  const char* src = "#define foo\n"
                    "#extension ex : enable\n"
                    "#version 100\n";
  const std::string gles = SanitizeShaderSource(src, ShaderLanguage_GLSL_ES);
  const std::string core = SanitizeShaderSource(src, ShaderLanguage_GLSL);

  // Only a single "version" and "extension" statement should be present, and
  // the "version" should appear before the others.
  EXPECT_EQ(gles.find("version"), gles.rfind("version"));
  EXPECT_EQ(gles.find("extension"), gles.rfind("extension"));
  EXPECT_LT(gles.find("version"), gles.find("define"));
  EXPECT_LT(gles.find("version"), gles.find("extension"));

  EXPECT_EQ(core.find("version"), core.rfind("version"));
  EXPECT_EQ(core.find("extension"), core.rfind("extension"));
  EXPECT_LT(core.find("version"), core.find("define"));
  EXPECT_LT(core.find("version"), core.find("extension"));
}

std::string SanitizeVersionHelper(ShaderLanguage language,
                                  const std::string& version) {
  const size_t version_tag_len = strlen("#version ");

  const std::string src = "#version " + version + "\n";
  std::string res = SanitizeShaderSource(src.c_str(), language);
  EXPECT_EQ(res.find("#version"), size_t(0));
  EXPECT_GT(res.size(), version_tag_len);
  return res.substr(version_tag_len, res.find("\n") - version_tag_len);
}

TEST(SanitizeShaderSourceTest, VersionNumbers) {
  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL_ES, "100"), "100 es");
  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL_ES, "110"), "100 es");
  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL_ES, "300"), "300 es");
  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL_ES, "330"), "300 es");
  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL_ES, "500"), "500 es");

  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL_ES, "100 es"), "100 es");
  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL_ES, "110 es"), "110 es");
  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL_ES, "300 es"), "300 es");
  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL_ES, "330 es"), "330 es");
  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL_ES, "500 es"), "500 es");

  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL, "100"), "100");
  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL, "110"), "110");
  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL, "300"), "300");
  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL, "330"), "330");
  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL, "500"), "500");

  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL, "100 es"), "110");
  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL, "110 es"), "110");
  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL, "300 es"), "330");
  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL, "330 es"), "330");
  EXPECT_EQ(SanitizeVersionHelper(ShaderLanguage_GLSL, "500 es"), "500");
}

TEST(SanitizeShaderSourceTest, IgnoreComments) {
  const char* src =
      "#define foo\n"
      "// #extension ex1 : enable\n"
      "#define bar\n"
      "// #extension ex2 : enable\n"
      "// comment with continuation\\\n"
      "   #extension ex3 : enable\n"
      "/* multiline\n"
      "#extension ex4 : enable\n"
      "*/"
      "#define baz /* multiline\n"
      "#extension ex5 : enable\n"
      "*/"
      "void main() {}";

  const std::string gles = SanitizeShaderSource(src, ShaderLanguage_GLSL_ES);
  const std::string core = SanitizeShaderSource(src, ShaderLanguage_GLSL);

  LOG(ERROR) << "gles" << std::endl << gles;
  LOG(ERROR) << "core" << std::endl << core;

  // No extensions should appear anywhere.
  EXPECT_EQ(gles.find("#extension"), std::string::npos);
  EXPECT_EQ(core.find("#extension"), std::string::npos);
}

}  // namespace
}  // namespace lull
