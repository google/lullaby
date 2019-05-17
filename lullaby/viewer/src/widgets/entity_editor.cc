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

#include "lullaby/viewer/src/widgets/entity_editor.h"

#include "lullaby/viewer/src/builders/build_blueprint.h"
#include "lullaby/viewer/src/file_manager.h"
#include "lullaby/viewer/src/jsonnet_writer.h"
#include "dear_imgui/imgui.h"
#include "fplbase/utilities.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/util/color.h"
#include "lullaby/util/filename.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/tools/common/file_utils.h"

using namespace flatbuffers;

namespace lull {
namespace tool {

bool operator==(const mathfu::quat& lhs, const mathfu::quat& rhs) {
  return lhs.scalar() == rhs.scalar() && lhs.vector() == rhs.vector();
}

void EntityEditor::Open() { open_ = true; }

void EntityEditor::Close() { open_ = false; }

flatbuffers::BaseType GetBaseType(const flatbuffers::Type& type) {
  return type.base_type != BASE_TYPE_VECTOR ? type.base_type : type.element;
}

bool IsValueType(const flatbuffers::Type& type) {
  const BaseType base_type = GetBaseType(type);
  const flatbuffers::StructDef* struct_def = type.struct_def;
  if (struct_def) {
    if (struct_def->name == "Vec3") {
      return true;
    } else if (struct_def->name == "Vec4") {
      return true;
    } else if (struct_def->name == "Quat") {
      return true;
    } else if (struct_def->name == "Color") {
      return true;
    } else if (struct_def->name == "AabbDef") {
      return true;
    }
  } else if (base_type == BASE_TYPE_STRING) {
    return true;
  } else if (flatbuffers::IsScalar(base_type)) {
    return true;
  }
  return false;
}

bool IsVectorType(const flatbuffers::Type& type) {
  return type.base_type == BASE_TYPE_VECTOR && type.element != BASE_TYPE_NONE;
}

bool IsStructType(const flatbuffers::Type& type) {
  return type.struct_def && !IsValueType(type);
}

Variant ToVariant(const flatbuffers::Type& type) {
  const BaseType base_type = GetBaseType(type);
  const flatbuffers::StructDef* struct_def = type.struct_def;
  if (struct_def) {
    if (struct_def->name == "Vec3") {
      return mathfu::kZeros3f;
    } else if (struct_def->name == "Vec4") {
      return mathfu::kZeros4f;
    } else if (struct_def->name == "Quat") {
      return mathfu::kQuatIdentityf;
    } else if (struct_def->name == "Color") {
      return Color4ub();
    } else if (struct_def->name == "AabbDef") {
      return Aabb();
    } else {
      return VariantMap();
    }
  } else if (base_type == BASE_TYPE_STRING) {
    return std::string();
  } else if (flatbuffers::IsInteger(base_type)) {
    return 0;
  } else if (flatbuffers::IsFloat(base_type)) {
    return 0.f;
  } else {
    return Variant();
  }
}

void Input(const char* name, const char* tip, bool* data) {
  int value = *data ? 1 : 0;
  const char* options[] = {"No", "Yes"};
  ImGui::Combo(name, &value, options, 2);
  *data = value != 0;
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", tip);
  }
}

void Input(const char* name, const char* tip, int* data) {
  ImGui::InputInt(name, data);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", tip);
  }
}

void Input(const char* name, const char* tip, float* data) {
  ImGui::InputFloat(name, data);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", tip);
  }
}

void Input(const char* name, const char* tip, mathfu::vec2* data) {
  ImGui::InputFloat2(name, &data->x, 3);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", tip);
  }
}

void Input(const char* name, const char* tip, mathfu::vec2i* data) {
  ImGui::InputInt2(name, &data->x);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", tip);
  }
}

void Input(const char* name, const char* tip, mathfu::vec3* data) {
  ImGui::InputFloat3(name, &data->x, 3);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", tip);
  }
}

void Input(const char* name, const char* tip, mathfu::vec4* data) {
  ImGui::InputFloat4(name, &data->x, 3);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", tip);
  }
}

void Input(const char* name, const char* tip, mathfu::quat* data) {
  ImGui::InputFloat4(name, reinterpret_cast<float*>(data));
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", tip);
  }
}

void Input(const char* name, const char* tip, mathfu::rectf* data) {
  ImGui::InputFloat4(name, &data->pos.x);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", tip);
  }
}

void Input(const char* name, const char* tip, Color4ub* data) {
  auto vec = Color4ub::ToVec4(*data);
  ImGui::ColorEdit4(name, &vec.x);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", tip);
  }
  *data = Color4ub::FromVec4(vec);
}

void Input(const char* name, const char* tip, Aabb* data) {
  std::string min_label = std::string(name) + " (min)";
  std::string max_label = std::string(name) + " (max)";
  ImGui::InputFloat3(min_label.c_str(), &data->min.x, 3);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", tip);
  }
  ImGui::InputFloat3(max_label.c_str(), &data->max.x, 3);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", tip);
  }
}

void Input(const char* name, const char* tip, std::string* data) {
  char buffer[256];
  const size_t end = std::min(data->size(), sizeof(buffer) - 1);
  strncpy(buffer, data->c_str(), end);
  buffer[end] = 0;

  ImGui::InputText(name, buffer, sizeof(buffer));
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", tip);
  }
  *data = buffer;
}

template <typename T>
Variant Edit(Variant* obj, const std::string& label, const std::string& tooltip,
             const T& default_value) {
  T copy = default_value;
  T* ptr = obj ? obj->Get<T>() : &copy;
  Input(label.c_str(), tooltip.c_str(), ptr);

  if (*ptr == default_value) {
    return Variant();
  } else {
    return *ptr;
  }
}

bool HasAttribute(const flatbuffers::Definition* def, const char* name) {
  return def && def->attributes.Lookup(name);
}

std::string GetAttribute(const flatbuffers::Definition* def, const char* name) {
  if (def) {
    const auto* attrib = def->attributes.Lookup(name);
    if (attrib) {
      return attrib->constant;
    }
  }
  return "";
}

Variant EditScalarFieldDef(Variant* obj, const flatbuffers::FieldDef* def,
                           const char* label = nullptr) {
  std::string tooltip;
  for (auto& str : def->doc_comment) {
    tooltip += str + "\n";
  }

  if (label == nullptr) {
    label = def->name.c_str();
  }

  // Remove underscores from labels so a field like "shading_model" will appear
  // as the label "shading model" in the panel.
  std::string name = label;
  while (true) {
    auto pos = name.find("_");
    if (pos == std::string::npos) {
      break;
    }
    name.replace(pos, 1, " ");
  }

  const flatbuffers::Type& type = def->value.type;
  const flatbuffers::BaseType base_type =
      type.base_type == BASE_TYPE_VECTOR ? type.element : type.base_type;

  if (type.struct_def && type.struct_def->name == "Vec3") {
    return Edit(obj, name, tooltip, mathfu::kZeros3f);
  } else if (type.struct_def && type.struct_def->name == "Vec4") {
    return Edit(obj, name, tooltip, mathfu::kZeros4f);
  } else if (type.struct_def && type.struct_def->name == "Quat") {
    return Edit(obj, name, tooltip, mathfu::kQuatIdentityf);
  } else if (type.struct_def && type.struct_def->name == "Color") {
    return Edit(obj, name, tooltip, Color4ub());
  } else if (type.struct_def && type.struct_def->name == "AabbDef") {
    return Edit(obj, name, tooltip, Aabb());
  } else if (base_type == BASE_TYPE_STRING) {
    return Edit(obj, name, tooltip, std::string());
  } else if (base_type == BASE_TYPE_BOOL) {
    return Edit(obj, name, tooltip, false);
  } else if (flatbuffers::IsInteger(base_type)) {
    if (HasAttribute(def, "hashvalue")) {
      return Edit(obj, name, tooltip, std::string());
    } else {
      return Edit(obj, name, tooltip, 0);
    }
  } else if (flatbuffers::IsFloat(base_type)) {
    return Edit(obj, name, tooltip, 0.f);
  } else {
    ImGui::Text("%s", name.c_str());
    return Variant();
  }
}

void EditScalarFieldDef(const flatbuffers::FieldDef* def, VariantMap* obj) {
  const HashValue key = Hash(def->name);

  auto iter = obj->find(key);
  Variant* in = iter != obj->end() ? &iter->second : nullptr;

  const Variant out = EditScalarFieldDef(in, def);
  if (out.Empty()) {
    obj->erase(key);
  } else if (in == nullptr) {
    obj->emplace(key, out);
  }
}

void EditStructDef(const flatbuffers::StructDef* def, VariantMap* obj);

void EditArrayDef(const flatbuffers::FieldDef* def, VariantArray* arr) {
  ImGui::PushID(arr);
  const flatbuffers::Type& type = def->value.type;

  ImGui::Text("%s", def->name.c_str());
  ImGui::SameLine();
  if (ImGui::SmallButton("+")) {
    Variant var = ToVariant(type);
    if (!var.Empty()) {
      arr->emplace_back(std::move(var));
    }
  }

  int index = 0;
  for (auto iter = arr->begin(); iter != arr->end(); ++iter) {
    Variant& var = *iter;
    ImGui::PushID(&var);
    bool del = false;
    if (IsValueType(type)) {
      if (ImGui::SmallButton("x")) {
        del = true;
      }
      std::string label = std::to_string(index);
      ImGui::SameLine();
      EditScalarFieldDef(&var, def, label.c_str());
    } else if (IsStructType(type)) {
      if (ImGui::SmallButton("x")) {
        del = true;
      }
      ImGui::SameLine();
      EditStructDef(type.struct_def, var.Get<VariantMap>());
    } else {
      ImGui::Text("?: %s", def->name.c_str());
    }
    ImGui::PopID();
    if (del) {
      arr->erase(iter);
      break;
    }
    ++index;
  }
  ImGui::PopID();
}

void EditFieldDef(const flatbuffers::FieldDef* def, VariantMap* obj) {
  ImGui::PushID(def->name.c_str());
  const flatbuffers::Type& type = def->value.type;
  if (IsVectorType(type)) {
    Variant child = VariantArray();
    auto iter = obj->emplace(Hash(def->name), std::move(child)).first;
    EditArrayDef(def, iter->second.Get<VariantArray>());
  } else if (IsStructType(type)) {
    auto iter = obj->find(Hash(def->name));

    ImGui::Text("%s", def->name.c_str());
    if (iter == obj->end()) {
      ImGui::SameLine();
      if (ImGui::SmallButton("+")) {
        VariantMap child = VariantMap();
        iter = obj->emplace(Hash(def->name), std::move(child)).first;
      }
    }
    if (iter != obj->end()) {
      bool del = false;
      if (ImGui::SmallButton("x")) {
        del = true;
      }
      ImGui::SameLine();
      EditStructDef(type.struct_def, iter->second.Get<VariantMap>());
      if (del) {
        obj->erase(iter);
      }
    }
  } else if (IsValueType(type)) {
    EditScalarFieldDef(def, obj);
  } else {
    ImGui::Text("Cannot edit field (%s)?", def->name.c_str());
  }
  ImGui::PopID();
}

bool IsExpandableField(const flatbuffers::FieldDef* field) {
  const flatbuffers::Type& type = field->value.type;
  if (type.base_type == BASE_TYPE_VECTOR) {
    return true;
  }
  if (type.base_type == BASE_TYPE_STRING) {
    return false;
  }
  if (type.struct_def) {
    if (type.struct_def->name == "Vec3") {
      return false;
    } else if (type.struct_def->name == "Vec4") {
      return false;
    } else if (type.struct_def->name == "Quat") {
      return false;
    } else if (type.struct_def->name == "Color") {
      return false;
    } else if (type.struct_def->name == "AabbDef") {
      return false;
    } else {
      return true;
    }
  } else if (flatbuffers::IsScalar(type.base_type)) {
    return false;
  }
  return false;
}

void EditStructDef(const flatbuffers::StructDef* def, VariantMap* obj) {
  if (ImGui::TreeNode(def->name.c_str())) {
    ImGui::PushID(obj);
    for (const auto* field : def->fields.vec) {
      if (!IsExpandableField(field)) {
        EditFieldDef(field, obj);
      }
    }
    for (const auto* field : def->fields.vec) {
      if (IsExpandableField(field)) {
        EditFieldDef(field, obj);
      }
    }
    ImGui::PopID();
    ImGui::TreePop();
  }
}

const EnumDef* GetComponentUnionFromEntityDef(const StructDef* entity_def) {
  if (entity_def == nullptr) {
    LOG(ERROR) << "No EntityDef";
    return nullptr;
  }
  const FieldDef* components_field = entity_def->fields.Lookup("components");
  if (components_field == nullptr) {
    LOG(ERROR) << "No 'components' field in EntityDef";
    return nullptr;
  }
  if (components_field->value.type.base_type != BASE_TYPE_VECTOR) {
    LOG(ERROR) << "The 'components' field in EntityDef is not a vector";
    return nullptr;
  }
  const StructDef* components_def = components_field->value.type.struct_def;
  if (components_def == nullptr) {
    LOG(ERROR) << "The 'components' field in EntityDef is not a table";
    return nullptr;
  }
  const FieldDef* union_field = components_def->fields.Lookup("def");
  if (union_field == nullptr) {
    LOG(ERROR) << "No 'def' field in ComponentDef.";
    return nullptr;
  }
  if (union_field->value.type.base_type != BASE_TYPE_UNION) {
    LOG(ERROR) << "The 'def' field in ComponentDef is not a union.";
    return nullptr;
  }
  const EnumDef* component_union = union_field->value.type.enum_def;
  if (component_union == nullptr) {
    LOG(ERROR) << "The 'def' field in ComponentDef is not a union.";
    return nullptr;
  }
  return component_union;
}

void WriteStruct(JsonnetWriter* jsonnet, const StructDef* def,
                 const VariantMap& values);

void WriteScalar(JsonnetWriter* jsonnet, const FieldDef* def,
                 const Variant& value) {
  const flatbuffers::Type& type = def->value.type;
  const flatbuffers::BaseType base_type =
      type.base_type == BASE_TYPE_VECTOR ? type.element : type.base_type;
  if (type.struct_def && type.struct_def->name == "Vec3") {
    auto v = value.Get<mathfu::vec3>();
    if (v) {
      jsonnet->BeginMap();
      jsonnet->FieldAndValue("x", v->x);
      jsonnet->FieldAndValue("y", v->y);
      jsonnet->FieldAndValue("z", v->z);
      jsonnet->EndMap();
    }
  } else if (type.struct_def && type.struct_def->name == "Vec4") {
    auto v = value.Get<mathfu::vec4>();
    if (v) {
      jsonnet->BeginMap();
      jsonnet->FieldAndValue("x", v->x);
      jsonnet->FieldAndValue("y", v->y);
      jsonnet->FieldAndValue("z", v->z);
      jsonnet->FieldAndValue("w", v->w);
      jsonnet->EndMap();
    }
  } else if (type.struct_def && type.struct_def->name == "Quat") {
    auto v = value.Get<mathfu::vec4>();
    if (v) {
      jsonnet->BeginMap();
      jsonnet->FieldAndValue("x", v->x);
      jsonnet->FieldAndValue("y", v->y);
      jsonnet->FieldAndValue("z", v->z);
      jsonnet->FieldAndValue("w", v->w);
      jsonnet->EndMap();
    }
  } else if (type.struct_def && type.struct_def->name == "AabbDef") {
    auto v = value.Get<Aabb>();
    if (v) {
      jsonnet->Value("todo");
    }
  } else if (type.struct_def && type.struct_def->name == "Color") {
    auto v = value.Get<Color4ub>();
    if (v) {
      jsonnet->Value("todo");
    }
  } else if (base_type == BASE_TYPE_STRING) {
    auto v = value.Get<std::string>();
    if (v) {
      jsonnet->Value(*v, true);
    }
  } else if (base_type == BASE_TYPE_BOOL) {
    auto v = value.Get<bool>();
    if (v) {
      if (*v) {
        jsonnet->Value("true");
      } else {
        jsonnet->Value("false");
      }
    }
  } else if (flatbuffers::IsInteger(base_type)) {
    if (HasAttribute(def, "hashvalue")) {
      auto v = value.Get<std::string>();
      if (v) {
        jsonnet->Value("utils.hash(\"" + *v + "\")");
      }
    } else {
      auto v = value.Get<int>();
      if (v) {
        jsonnet->Value(*v);
      }
    }
  } else if (flatbuffers::IsFloat(base_type)) {
    auto v = value.Get<float>();
    if (v) {
      jsonnet->Value(*v);
    }
  } else {
    LOG(ERROR) << "Unknown scalar field: " << def->name;
  }
}

void WriteField(JsonnetWriter* jsonnet, const FieldDef* def,
                const Variant& value) {
  const flatbuffers::Type& type = def->value.type;
  if (IsVectorType(type)) {
    const VariantArray* array = value.Get<VariantArray>();
    if (array && !array->empty()) {
      jsonnet->Field(def->name);
      jsonnet->BeginArray();

      const flatbuffers::Type& element_type = def->value.type;
      for (const Variant& var : *array) {
        if (IsValueType(element_type)) {
          WriteScalar(jsonnet, def, var);
        } else if (IsStructType(element_type)) {
          const VariantMap* values = var.Get<VariantMap>();
          if (values) {
            jsonnet->BeginMap();
            WriteStruct(jsonnet, element_type.struct_def, *values);
            jsonnet->EndMap();
          }
        } else {
          LOG(ERROR) << "Uknown vector element type.";
        }
      }
      jsonnet->EndArray();
    }
  } else if (IsStructType(type)) {
    const VariantMap* values = value.Get<VariantMap>();
    if (values && !values->empty()) {
      jsonnet->Field(def->name);
      jsonnet->BeginMap();
      WriteStruct(jsonnet, type.struct_def, *values);
      jsonnet->EndMap();
    } else {
      LOG(ERROR) << values;
    }
  } else if (IsValueType(type)) {
    jsonnet->Field(def->name);
    WriteScalar(jsonnet, def, value);
  } else {
    LOG(ERROR) << "Unknown field: " << def->name;
  }
}

void WriteStruct(JsonnetWriter* jsonnet, const StructDef* def,
                 const VariantMap& values) {
  for (auto field : def->fields.vec) {
    const HashValue key = Hash(field->name);
    auto iter = values.find(key);
    if (iter == values.end()) {
      continue;
    }
    if (iter->second.Empty()) {
      continue;
    }
    WriteField(jsonnet, field, iter->second);
  }
}


void EntityEditor::AdvanceFrame() {
  if (!open_) {
    return;
  }

  if (!parser_) {
    parser_ = MakeUnique<flatbuffers::Parser>(idl_opts_);

    std::string data;
    const bool status = fplbase::LoadFile("entity_schema.fbs", &data);
    if (!status) {
      return;
    }
    parser_->Parse(data.c_str());
  }

  if (ImGui::Begin("Entity Editor")) {
    if (ImGui::Button("Create New Entity")) {
      JsonnetWriter jsonnet;
      jsonnet.Code("local utils = import \"utils.jsonnet\";");
      jsonnet.BeginMap();
      jsonnet.Field("components");
      jsonnet.BeginArray();
      for (const ComponentData& it : entity_data_) {
        const StructDef* struct_def =
          parser_->structs_.Lookup(it.name.c_str());
        if (struct_def) {
          jsonnet.BeginMap();
          jsonnet.Field("def_type");
          jsonnet.Value(it.name.substr(5), true);
          jsonnet.Field("def");
          jsonnet.BeginMap();
          WriteStruct(&jsonnet, struct_def, it.data);
          jsonnet.EndMap(it.name);
          jsonnet.EndMap();
        }
      }
      jsonnet.EndArray();
      jsonnet.EndMap();

      const std::string data = jsonnet.ToString();
      LOG(ERROR) << data;

      // Create an arbitrary name for this entity.
      static int entity_name_suffix = 0;
      const std::string entity_name =
          "entity" + std::to_string(entity_name_suffix);
      ++entity_name_suffix;

      const std::string out_dir = FileManager::MakeTempFolder(entity_name);
      const std::string filename = out_dir + entity_name + ".jsonnet";
      SaveFile(data.c_str(), data.size(), filename.c_str(), false);
      registry_->Get<FileManager>()->ImportFile(filename);

      auto* entity_factory = registry_->Get<EntityFactory>();
      entity_factory->Create(entity_name);
      entity_data_.clear();
    }
    ImGui::Separator();

    const StructDef* entity_def = parser_->structs_.Lookup("EntityDef");
    const EnumDef* components_def = GetComponentUnionFromEntityDef(entity_def);
    if (components_def) {
      std::vector<const char*> names;
      for (const EnumVal* val : components_def->vals.vec) {
        if (val && val->union_type.struct_def) {
          const std::string& name = val->union_type.struct_def->name;
          names.push_back(name.c_str());
        }
      }

      static int item2 = -1;
      ImGui::Combo("", &item2, names.data(), names.size());
      ImGui::SameLine();
      if (ImGui::Button("Add Component") && item2 != -1) {
        entity_data_.emplace_back();
        entity_data_.back().name = std::string("lull.") + names[item2];
      }
    }

    enum Options {
      kNothing,
      kMoveUp,
      kMoveDown,
      kClear,
      kDelete
    };
    for (auto iter = entity_data_.begin(); iter != entity_data_.end(); ++iter) {
      ImGui::PushID(&iter->data);
      const StructDef* struct_def =
          parser_->structs_.Lookup(iter->name.c_str());
      Options opt = kNothing;
      if (struct_def) {
        if (ImGui::SmallButton("\uf062")) {
          opt = kMoveUp;
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Move Up");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("\uf063")) {
          opt = kMoveDown;
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Move Down");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("\uf12d")) {
          opt = kClear;
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Clear Data");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("\uf1f8")) {
          opt = kDelete;
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Delete Component");
        }
        ImGui::SameLine();
        EditStructDef(struct_def, &iter->data);
      }
      ImGui::PopID();

      if (opt == kMoveUp) {
        if (iter != entity_data_.begin()) {
          std::swap(*iter, *(iter - 1));
          break;
        }
      } else if (opt == kMoveDown) {
        if (iter + 1 != entity_data_.end()) {
          std::swap(*iter, *(iter + 1));
          break;
        }
      } else if (opt == kClear) {
        iter->data.clear();
      } else if (opt == kDelete) {
        entity_data_.erase(iter);
        break;
      }
    }
  }
  ImGui::End();
}

}  // namespace tool
}  // namespace lull
