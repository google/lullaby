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

#include <sstream>
#include "lullaby/util/logging.h"

namespace lull {
namespace {
static constexpr int kUnspecifiedVersion = 0;
}  // namespace

static const char kVersionTag[] = "version";
static const char kIfTag[] = "if";
static const char kEndIfTag[] = "endif";
static const char kNewlineCharacterSet[] = "\n\r";

static const size_t kVersionTagLength = sizeof(kVersionTag) - 1;
static const size_t kIfTagLength = sizeof(kIfTag) - 1;
static const size_t kEndIfTagLength = sizeof(kEndIfTag) - 1;

//    core, es
#define LULLABY_GL_VERSION_MAP \
  MAP(110, 100)                \
  MAP(120, 100)                \
  MAP(130, 100)                \
  MAP(140, 100)                \
  MAP(150, 100)                \
  MAP(330, 300)                \
  MAP(400, 300)                \
  MAP(410, 300)                \
  MAP(420, 300)                \
  MAP(430, 300)

int ConvertShaderVersionFromEsToCore(int version) {
#define MAP(_core, _es) \
  if (version == _es) return _core;
  LULLABY_GL_VERSION_MAP
#undef MAP
  return version;
}

int ConvertShaderVersionFromCoreToEs(int version) {
#define MAP(_core, _es) \
  if (version == _core) return _es;
  LULLABY_GL_VERSION_MAP
#undef MAP
  return version;
}

int ConvertShaderVersionFromCompat(int version, ShaderLanguage to) {
  if (version == kUnspecifiedVersion) {
    return kUnspecifiedVersion;
  }
  if (to == ShaderLanguage_GLSL) {
    return ConvertShaderVersionFromEsToCore(version);
  }
  if (to == ShaderLanguage_GLSL_ES) {
    // OpenGL Compat uses the same shader versions as GLSL ES. Currently 100 and
    // 300 are the only shader version in GLSL ES.
    if (version == 100 || version == 300) {
      return version;
    } else {
      LOG(DFATAL) << "Unknown GLCompat version: " << version;
      return kUnspecifiedVersion;
    }
  }
  return version;
}

static int SanitizeVersionNumber(ShaderLanguage target_language,
                                 ShaderLanguage found_language,
                                 int version_number) {
  if (version_number != 0 && target_language != found_language) {
    if (target_language == ShaderLanguage_GLSL) {
      version_number = ConvertShaderVersionFromEsToCore(version_number);
    } else {
      version_number = ConvertShaderVersionFromCoreToEs(version_number);
    }
  }
  if (version_number == 0 && target_language == ShaderLanguage_GLSL) {
    // This version fixing logic should only apply in the case of .fplshaders.
    // If you're running into shader version issues here, please switch to
    // lullshaders.  Changing these version numbers may break existing clients.
#if defined(GL_CORE_PROFILE) || defined(PLATFORM_OSX)
    version_number = 330;
#else
    version_number = 120;
#endif
  }
  return version_number;
}

static int ReadVersionNumber(const char* str, ShaderLanguage* language_out) {
  char spec[3];
  int version = 0;
  const int num_scanned = sscanf(str, " %d %2[es]", &version, spec);
  if (num_scanned == 1) {
    *language_out = ShaderLanguage_GLSL;
  } else if (num_scanned == 2) {
    *language_out = ShaderLanguage_GLSL_ES;
  } else {
    LOG(ERROR) << "Invalid version identifier: " << str;
  }
  return version;
}

static const char* SkipWhitespaceInLine(const char* str) {
  // Whitespace which doesn't end a line: space, horizontal & vertical tabs.
  // GLSL ES spec: https://www.khronos.org/files/opengles_shading_language.pdf
  return str + strspn(str, " \t\v");
}

static const char* FindNextLine(const char* str) {
  // Newlines are \n, \r, \r\n or \n\r, except when immediately preceded by a
  // backslash.
  const char* next_line = str + strcspn(str, kNewlineCharacterSet);
  while (next_line > str && next_line[-1] == '\\') {
    next_line += strspn(next_line, kNewlineCharacterSet);
    next_line += strcspn(next_line, kNewlineCharacterSet);
  }
  next_line += strspn(next_line, kNewlineCharacterSet);
  return next_line;
}

static bool IsEmptyLine(const char* str) {
  return strcspn(str, kNewlineCharacterSet) == 0;
}

// Returns the start of a multiline comment within a line.
static const char* FindUnterminatedCommentInLine(const char* str, size_t len) {
  // Search backwards.  If we find /*, return its location unless we've already
  // seen */.
  for (const char* ptr = str + len - 1; ptr > str; --ptr) {
    if (ptr[-1] == '*' && ptr[0] == '/') {
      return nullptr;
    }
    if (ptr[-1] == '/' && ptr[0] == '*') {
      return ptr - 1;
    }
  }
  return nullptr;
}

std::string SanitizeShaderSource(string_view code, ShaderLanguage language) {
  const std::string code_string(code);
  int if_depth = 0;
  int version_number = 0;
  bool found_precision = false;
  const char* remaining_code = code_string.data();
  ShaderLanguage found_language = ShaderLanguage_GLSL;

  std::vector<string_view> preamble;

  // Iterate through the code until we find the first non-empty, non-comment,
  // non-preprocessor line. Strip out the #version line (we'll manually add it),
  // and remember lines we've seen so they can be added before the precision
  // specifier.
  const char* iter = code_string.data();
  while (iter && *iter) {
    const char* start = SkipWhitespaceInLine(iter);
    iter = FindNextLine(start);
    if (iter == start) {
      break;
    } else if (IsEmptyLine(start)) {
      continue;
    } else if (start[0] == '/' && start[1] == '/') {
      continue;
    }

    if (if_depth == 0) {
      remaining_code = start;
    }

    // If this line ends inside a comment block, have iter start where
    // the comment block starts.
    const char* comment_start =
        FindUnterminatedCommentInLine(start, iter - start);
    if (comment_start) {
      const char* comment_end = strstr(comment_start, "*/");
      if (comment_start != start) {
        // By setting next to the comment_start, we should see that
        // |comment_start == start| in the next iteration of this loop.
        // In the meantime, process everything between the start and the
        // comment_start.
        iter = comment_start;
      } else if (comment_end) {
        // Continue processing after the comment.
        iter = comment_end + 2;
        continue;
      } else {
        // Unfinished comment block, exit the loop.
        iter = nullptr;
        break;
      }
    }

    if (start[0] == '#') {
      bool should_append_line = true;

      // The actual directive can be separated from # by spaces and tabs.
      const char* directive = SkipWhitespaceInLine(start + 1);
      if (strncmp(directive, kVersionTag, kVersionTagLength) == 0) {
        // Do not append this version to the preamble as we will do that
        // explicitly ourselves.
        should_append_line = false;

        if (if_depth != 0) {
          LOG(ERROR) << "Found #version directive within an #if";
        }
        if (version_number != 0) {
          LOG(ERROR) << "More than one #version found in shader.";
        } else {
          const char* version_str =
              SkipWhitespaceInLine(directive + kVersionTagLength);
          version_number = ReadVersionNumber(version_str, &found_language);
        }
        remaining_code = iter;
      } else if (strncmp(directive, kIfTag, kIfTagLength) == 0) {
        // Handles both #if and #ifdef.
        ++if_depth;
      } else if (strncmp(directive, kEndIfTag, kEndIfTagLength) == 0) {
        if (if_depth == 0) {
          LOG(ERROR) << "Found #endif without #if.";
        } else {
          --if_depth;
        }
      } else {
        remaining_code = iter;
      }

      if (should_append_line) {
        const char* end = iter - 1;
        while (*end == '\r' || *end == '\n' || *end == ' ' || *end == '\t') {
          --end;
        }
        preamble.emplace_back(start, end - start + 1);
      }

      // Keep processing.
      continue;
    }

    // At this point, we have found the first line of non-empty, non-comment,
    // non-preprocessor code, so we can stop.
    if (strncmp(start, "precision", 9) == 0) {
      found_precision = true;
    }
    break;
  }

  // Assemble the sanitized shader source.
  std::stringstream ss;

  // Version number must come first.
  version_number =
      SanitizeVersionNumber(language, found_language, version_number);
  if (version_number != 0) {
    if (language == ShaderLanguage_GLSL) {
      ss << "#version " << version_number << std::endl;
    } else {
      ss << "#version " << version_number << " es" << std::endl;
    }
  }

  // Add per-platform definitions.
  if (language == ShaderLanguage_GLSL) {
    ss << "#define lowp" << std::endl;
    ss << "#define mediump" << std::endl;
    ss << "#define highp" << std::endl;
  }

  // Add the preamble (lines before any code). Make sure we don't go past
  // remaining_code, otherwise we end up with a partial dupe.
  for (string_view line : preamble) {
    if (line.data() >= remaining_code) {
      break;
    }
    const size_t len = remaining_code - line.data();
    ss << std::string(line.data(), std::min(len, line.size())) << std::endl;
  }

  // Add a default precision specifier if one hasn't been specified.
  if (!found_precision && language == ShaderLanguage_GLSL_ES) {
    ss << "precision highp float;" << std::endl;
  }

  // Add the rest of the code.
  ss << remaining_code;
  return ss.str();
}

}  // namespace lull
