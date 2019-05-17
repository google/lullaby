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

#include "lullaby/tools/compile_blueprint_from_json/blueprint_from_json_compiler.h"

#include "flatbuffers/flatbuffers.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/util/optional.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace lull {
namespace tool {
namespace {
// TODO replace with string_view after conversion to absl.
// Outputs more useful error messages such as line numbers and reminders for
// commonly encountered errors by using laxer JSON syntax that flatc usually
// allows.
void PrintJsonError(const std::string& contents,
                    const rapidjson::Document& json) {
  static constexpr char kNewline[] = "\n";
  size_t target_offset = json.GetErrorOffset();
  size_t current_offset = contents.find(kNewline);
  size_t line = 1;
  while (current_offset != std::string::npos &&
         current_offset < target_offset) {
    current_offset = contents.find(kNewline, current_offset + 1);
    ++line;
  }
  const char* additional_msg = "";
  switch (json.GetParseError()) {
    case rapidjson::ParseErrorCode::kParseErrorObjectMissName:
      additional_msg =
          "This could be due to a trailing comma, maybe from the previous "
          "line. Or, the object key is not quoted.";
      break;
    case rapidjson::ParseErrorCode::kParseErrorValueInvalid:
      additional_msg = "This could be due to an enum value that isn't quoted.";
      break;
    default:
      break;
  }
  LOG(ERROR) << "Error on line " << line << ": "
             << GetParseError_En(json.GetParseError()) << " " << additional_msg;
}
}  // namespace

BlueprintFromJsonCompiler::BlueprintFromJsonCompiler() {
  // Matches arguments in dev/utils_common.bzl:
  // FLATC_DEFAULT_ARGS = [
  //     "--no-union-value-namespacing",
  //     "--gen-name-strings",
  // ]
  // And, "flatc -b" in dev/build_entity.bzl.
  flatbuffers::IDLOptions opts;
  opts.union_value_namespacing = false;
  opts.generate_name_strings = true;
  opts.lang_to_generate |= flatbuffers::IDLOptions::kBinary;
  fb_parser_ = MakeUnique<flatbuffers::Parser>(opts);
}

bool BlueprintFromJsonCompiler::ParseFbs(string_view fbs_contents,
                                         const char** include_paths,
                                         string_view fbs_filename) {
  std::string fbs_contents_str(fbs_contents);
  std::string fbs_filename_str(fbs_filename);
  if (!fb_parser_->Parse(fbs_contents_str.c_str(), include_paths,
                         fbs_filename_str.c_str())) {
    LOG(ERROR) << fb_parser_->error_;
    return false;
  }
  return true;
}

flatbuffers::DetachedBuffer BlueprintFromJsonCompiler::ParseJson(
    string_view json_contents) {
  flatbuffers::DetachedBuffer empty;
  rapidjson::Document json;
  json.Parse(json_contents.data(), json_contents.size());
  if (json.HasParseError()) {
    PrintJsonError(std::string(json_contents), json);
    return empty;
  }

  if (!json.IsObject()) {
    LOG(ERROR) << "Not a json object.";
    return empty;
  }

  if (!ParseJsonEntity(json)) {
    return empty;
  }

  return blueprint_builder_.Finish();
}

bool BlueprintFromJsonCompiler::ParseJsonEntity(
    const rapidjson::Value& json_entity) {
  if (!json_entity.HasMember("components")) {
    LOG(ERROR) << "No components in json.";
    return false;
  }
  auto& json_components = json_entity["components"];
  if (!json_components.IsArray()) {
    LOG(ERROR) << "Expected components array.";
    return false;
  }

  if (json_entity.HasMember("children")) {
    auto& json_children = json_entity["children"];
    if (!json_children.IsArray()) {
      LOG(ERROR) << "Expected children field to be an array.";
      return false;
    }
    blueprint_builder_.StartChildren();
    for (auto iter = json_children.Begin(); iter != json_children.End();
         ++iter) {
      if (!ParseJsonEntity(*iter)) {
        return false;
      }
      blueprint_builder_.FinishChild();
    }
    blueprint_builder_.FinishChildren();
  }

  for (auto iter = json_components.Begin(); iter != json_components.End();
       ++iter) {
    if (!iter->HasMember("def_type") || !iter->HasMember("def")) {
      LOG(ERROR) << "Missing def_type or def.";
      return false;
    }
    const char* def_type = (*iter)["def_type"].GetString();
    auto& def = (*iter)["def"];
    rapidjson::StringBuffer json_buffer;
    rapidjson::Writer<rapidjson::StringBuffer> json_writer(json_buffer);
    def.Accept(json_writer);
    const char* def_str = json_buffer.GetString();
    const std::string def_type_name = GetDefTypeName(def_type);
    if (!fb_parser_->SetRootType(def_type_name.c_str())) {
      LOG(ERROR) << "Unknown def_type: " << def_type;
      return false;
    }
    if (!fb_parser_->Parse(def_str)) {
      LOG(ERROR) << "Could not parse def: " << def_type;
      return false;
    }
    if (!fb_parser_->root_struct_def_) {
      LOG(ERROR) << "No root_struct_def: " << def_type;
      return false;
    }

    blueprint_builder_.AddComponent(fb_parser_->root_struct_def_->name,
                                    {fb_parser_->builder_.GetBufferPointer(),
                                     fb_parser_->builder_.GetSize()});
  }
  return true;
}

std::string BlueprintFromJsonCompiler::GetDefTypeName(string_view def_type) {
  if (fb_parser_->structs_.Lookup(std::string(def_type))) {
    return std::string(def_type);
  }
  for (flatbuffers::StructDef* struct_def : fb_parser_->structs_.vec) {
    if (def_type == struct_def->name) {
      if (struct_def->defined_namespace) {
        return struct_def->defined_namespace->GetFullyQualifiedName(
            struct_def->name);
      } else {
        return struct_def->name;
      }
    }
  }
  return std::string(def_type);
}

}  // namespace tool
}  // namespace lull
