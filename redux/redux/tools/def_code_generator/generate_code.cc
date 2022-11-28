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

#include "redux/tools/def_code_generator/generate_code.h"

#include <string>

#include "redux/tools/def_code_generator/code_builder.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "redux/modules/base/filepath.h"

namespace redux::tool {

static std::string GetNamespace(const TypeMetadata& info) {
  std::string_view ns = info.name_space;
  if (!ns.empty()) {
    return absl::StrJoin(absl::StrSplit(std::string(ns), '.'), "::");
  } else {
    return "";
  }
}

static std::string GetFullyQualifiedName(const TypeMetadata& info) {
  const std::string ns = GetNamespace(info);
  if (ns.empty()) {
    return info.name;
  } else {
    return ns + "::" + info.name;
  }
}

static std::string GetFieldType(const FieldMetadata& info,
                                const DefDocument& doc) {
  if (info.type == "string") {
    return "std::string";
  } else if (info.type == "hash") {
    return "redux::HashValue";
  } else {
    return info.type;
  }
}

static std::string GetDefaultValue(const FieldMetadata& info) {
  if (auto iter = info.attributes.find(FieldMetadata::kDefaulValue);
      iter != info.attributes.end()) {
    return iter->second;
  } else if (info.type == "vec2") {
    return "vec2::Zero()";
  } else if (info.type == "vec3") {
    return "vec3::Zero()";
  } else if (info.type == "vec4") {
    return "vec4::Zero()";
  } else if (info.type == "quat") {
    return "quat::Identity()";
  } else if (info.type == "bool") {
    return "false";
  } else if (info.type == "int") {
    return "0";
  } else if (info.type == "float") {
    return "0.0f";
  } else {
    return "";
  }
}

static void AppendEnum(CodeBuilder& code, const EnumMetadata& info,
                       const DefDocument& doc) {
  code.SetNamespace(GetNamespace(info));

  // Enum definition start.
  code.AppendComment(info.description);
  code.Append("enum class {0} {{", info.name);
  code.Indent();

  // Values.
  for (const EnumeratorMetadata& e : info.enumerators) {
    code.AppendComment(e.description);
    code.Append("{0},", e.name);
  }
  code.AppendBlankLine();

  // Enum definition end.
  code.Deindent();
  code.Append("}};");
  code.AppendBlankLine();

  // ToString function.
  code.Append("inline const char* ToString({0} e) {{", info.name);
  code.Indent();

  // Switch start.
  code.Append("switch (e) {{");
  code.Indent();

  // Switch cases.
  for (const EnumeratorMetadata& e : info.enumerators) {
    code.Append("case {0}::{1}: return \"{1}\";", info.name, e.name);
  }

  // Switch end.
  code.Deindent();
  code.Append("}}");

  // ToString function end.
  code.Deindent();
  code.Append("}}");
  code.AppendBlankLine();
}

static void AppendStruct(CodeBuilder& code, const StructMetadata& info,
                         const DefDocument& doc) {
  code.SetNamespace(GetNamespace(info));

  // Struct definition start.
  code.AppendComment(info.description);
  code.Append("struct {0} {{", info.name);
  code.Indent();

  // Members.
  for (const FieldMetadata& field : info.fields) {
    code.AppendComment(field.description);
    const std::string value = GetDefaultValue(field);
    const std::string type = GetFieldType(field, doc);
    if (value.empty()) {
      code.Append("{0} {1};", type, field.name);
    } else {
      code.Append("{0} {1} = {2};", type, field.name, value);
    }
  }
  code.AppendBlankLine();

  // Serialize function.
  code.Append("template <typename Archive>");
  code.Append("void Serialize(Archive archive) {{");
  code.Indent();
  for (const FieldMetadata& field : info.fields) {
    code.Append("archive({0}, ConstHash(\"{0}\"));", field.name);
  }
  code.Deindent();
  code.Append("}}");

  // Struct definition end.
  code.Deindent();
  code.Append("}};");
  code.AppendBlankLine();
}

constexpr const char* kCommonIncludes[] = {
    "redux/modules/base/hash.h",
    "redux/modules/base/typeid.h",
    "redux/modules/math/bounds.h",
    "redux/modules/math/quaternion.h",
    "redux/modules/math/vector.h",
    "redux/modules/var/var_table.h",
};

std::string GenerateCode(const DefDocument& doc) {
  CodeBuilder code;
  code.Append("#pragma once");
  code.AppendBlankLine();

  // Include dependencies.
  for (const char* inc : kCommonIncludes) {
    code.Append("#include \"{}\"", inc);
  }
  for (const std::string& include : doc.includes) {
    if (GetExtension(include) == ".h") {
      code.Append("#include \"{}\"", include);
    } else if (GetExtension(include) == ".def") {
      std::string hfile = std::string(GetBasepath(include)) + "_generated.h";
      code.Append("#include \"{}\"", include);
    }
  }
  code.AppendBlankLine();

  // Forward-declare types (if more than 1 type in case there are dependencies).
  if (doc.structs.size() > 1) {
    for (const TypeMetadata& info : doc.structs) {
      code.SetNamespace(GetNamespace(info));
      code.Append("struct {};", info.name);
    }
    code.AppendBlankLine();
  }

  // Enums.
  for (const EnumMetadata& info : doc.enums) {
    AppendEnum(code, info, doc);
  }

  // Types.
  for (const StructMetadata& info : doc.structs) {
    AppendStruct(code, info, doc);
  }

  // Typeid registration.
  code.SetNamespace("");
  for (const EnumMetadata& info : doc.enums) {
    code.Append("REDUX_SETUP_TYPEID({0});", GetFullyQualifiedName(info));
  }
  for (const StructMetadata& info : doc.structs) {
    code.Append("REDUX_SETUP_TYPEID({0});", GetFullyQualifiedName(info));
  }

  return code.FlushToString();
}

}  // namespace redux::tool
