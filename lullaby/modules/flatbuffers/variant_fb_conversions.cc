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

#include "lullaby/modules/flatbuffers/variant_fb_conversions.h"

#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/util/common_types.h"

namespace lull {

bool VariantFromFbVariant(VariantDef type, const void* in, Variant* out) {
  if (!in || !out) {
    return false;
  }

  switch (type) {
    case VariantDef_DataBool: {
      const auto* data = static_cast<const DataBool*>(in);
      *out = data->value();
      return true;
    }
    case VariantDef_DataInt: {
      const auto* data = static_cast<const DataInt*>(in);
      *out = data->value();
      return true;
    }
    case VariantDef_DataFloat: {
      const auto* data = static_cast<const DataFloat*>(in);
      *out = data->value();
      return true;
    }
    case VariantDef_DataHashValue: {
      const auto* data = static_cast<const DataHashValue*>(in);
      *out = data->value();
      return true;
    }
    case VariantDef_DataString: {
      const auto* data = static_cast<const DataString*>(in);
      *out = data->value()->str();
      return true;
    }
    case VariantDef_DataVec2: {
      const auto* data = static_cast<const DataVec2*>(in);
      mathfu::vec2 mathfu_value;
      MathfuVec2FromFbVec2(data->value(), &mathfu_value);
      *out = mathfu_value;
      return true;
    }
    case VariantDef_DataVec3: {
      const auto* data = static_cast<const DataVec3*>(in);
      mathfu::vec3 mathfu_value;
      MathfuVec3FromFbVec3(data->value(), &mathfu_value);
      *out = mathfu_value;
      return true;
    }
    case VariantDef_DataVec4: {
      const auto* data = static_cast<const DataVec4*>(in);
      mathfu::vec4 mathfu_value;
      MathfuVec4FromFbVec4(data->value(), &mathfu_value);
      *out = mathfu_value;
      return true;
    }
    case VariantDef_DataQuat: {
      const auto* data = static_cast<const DataQuat*>(in);
      mathfu::quat mathfu_value;
      MathfuQuatFromFbQuat(data->value(), &mathfu_value);
      *out = mathfu_value;
      return true;
    }
    default: {
      const char* label = EnumNameVariantDef(type);
      LOG(ERROR) << "Unknown data variant type: " << label;
      return false;
    }
  }
}

bool VariantArrayFromFbVariantArray(const VariantArrayDef* in,
                                    VariantArray* out) {
  if (in == nullptr || out == nullptr) {
    return false;
  }
  if (in->values() == nullptr) {
    return true;
  }
  for (const VariantArrayDefImpl* values : *in->values()) {
    const void* variant_def = values->value();
    if (variant_def == nullptr) {
      LOG(DFATAL) << "No value specified, skipping array insertion.";
      continue;
    }

    Variant var;
    if (VariantFromFbVariant(values->value_type(), variant_def, &var)) {
      out->emplace_back(std::move(var));
    }
  }
  return true;
}

bool VariantMapFromFbVariantMap(const VariantMapDef* in, VariantMap* out) {
  if (in == nullptr || out == nullptr) {
    return false;
  }
  if (in->values() == nullptr) {
    return true;
  }
  for (const KeyVariantPairDef* pair : *in->values()) {
    const flatbuffers::String* key = pair->key();
    const HashValue key_hash = key ? Hash(key->c_str()) : pair->hash_key();

    const void* variant_def = pair->value();
    if (key_hash == 0) {
      LOG(DFATAL) << "Invalid key, skipping map insertion.";
      continue;
    } else if (variant_def == nullptr) {
      LOG(DFATAL) << "No value specified, skipping map insertion.";
      continue;
    }

    Variant var;
    if (VariantFromFbVariant(pair->value_type(), variant_def, &var)) {
      (*out)[key_hash] = std::move(var);
    }
  }
  return true;
}

}  // namespace lull
