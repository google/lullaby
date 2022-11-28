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

#include "redux/modules/flatbuffers/var.h"

#include <cstddef>

#include "redux/modules/base/data_buffer.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/flatbuffers/math.h"

namespace redux {

template <typename T>
static bool CastAndRead(const void* in, Var* out) {
  CHECK(in != nullptr) << "Input value is required.";
  CHECK(out != nullptr) << "Out param is required.";

  const auto* data = static_cast<const T*>(in);
  if constexpr (std::is_pointer_v<decltype(data->value())>) {
    if (data->value() != nullptr) {
      *out = ReadFbs(*data->value());
      return true;
    } else {
      return false;
    }
  } else {
    *out = ReadFbs(data->value());
    return true;
  }
}

absl::Span<const std::byte> ToByteSpan(const fbs::DataBytes* data) {
  CHECK(data != nullptr);
  CHECK(data->value() != nullptr);

  const std::byte* bytes =
      reinterpret_cast<const std::byte*>(data->value()->data());
  const std::size_t num_bytes = data->value()->size();
  return {bytes, num_bytes};
}

bool TryReadFbs(fbs::VarDef type, const void* in, Var* out) {
  CHECK(out != nullptr) << "Out param is required.";
  if (in == nullptr) {
    return false;
  }

  switch (type) {
    case fbs::VarDef::DataBool: {
      return CastAndRead<fbs::DataBool>(in, out);
    }
    case fbs::VarDef::DataInt: {
      return CastAndRead<fbs::DataInt>(in, out);
    }
    case fbs::VarDef::DataFloat: {
      return CastAndRead<fbs::DataFloat>(in, out);
    }
    case fbs::VarDef::DataString: {
      return CastAndRead<fbs::DataString>(in, out);
    }
    case fbs::VarDef::DataHashVal: {
      return CastAndRead<fbs::DataHashVal>(in, out);
    }
    case fbs::VarDef::DataHashString: {
      return CastAndRead<fbs::DataHashString>(in, out);
    }
    case fbs::VarDef::DataVec2f: {
      return CastAndRead<fbs::DataVec2f>(in, out);
    }
    case fbs::VarDef::DataVec2i: {
      return CastAndRead<fbs::DataVec2i>(in, out);
    }
    case fbs::VarDef::DataVec3f: {
      return CastAndRead<fbs::DataVec3f>(in, out);
    }
    case fbs::VarDef::DataVec3i: {
      return CastAndRead<fbs::DataVec3i>(in, out);
    }
    case fbs::VarDef::DataVec4f: {
      return CastAndRead<fbs::DataVec4f>(in, out);
    }
    case fbs::VarDef::DataVec4i: {
      return CastAndRead<fbs::DataVec4i>(in, out);
    }
    case fbs::VarDef::DataQuatf: {
      return CastAndRead<fbs::DataQuatf>(in, out);
    }
    case fbs::VarDef::DataBytes: {
      const auto* data = static_cast<const fbs::DataBytes*>(in);
      const absl::Span<const std::byte> span = ToByteSpan(data);
      *out = ByteVector(span.begin(), span.end());
      return true;
    }
    case fbs::VarDef::VarArrayDef: {
      const auto* data = static_cast<const fbs::VarArrayDef*>(in);

      VarArray value;
      TryReadFbs(data, &value);
      *out = std::move(value);
      return true;
    }
    case fbs::VarDef::VarTableDef: {
      const auto* data = static_cast<const fbs::VarTableDef*>(in);

      VarTable value;
      TryReadFbs(data, &value);
      *out = std::move(value);
      return true;
    }
    case fbs::VarDef::NONE: {
      return false;
    }
  }
  LOG(FATAL) << "Unknown type: " << fbs::EnumNameVarDef(type);
}

bool TryReadFbs(const fbs::VarArrayDef* in, VarArray* out) {
  CHECK(out != nullptr) << "Out param is required.";
  if (in == nullptr) {
    return false;
  }

  out->Clear();

  if (in->values() == nullptr) {
    return true;
  }

  for (const fbs::VarArrayDefImpl* values : *in->values()) {
    const void* variant_def = values->value();
    CHECK(variant_def != nullptr) << "No value specified.";

    Var var;
    if (TryReadFbs(values->value_type(), variant_def, &var)) {
      out->PushBack(std::move(var));
    }
  }
  return true;
}

bool TryReadFbs(const fbs::VarTableDef* in, VarTable* out) {
  CHECK(out != nullptr) << "Out param is required.";
  if (in == nullptr) {
    return false;
  }

  out->Clear();

  if (in->values() == nullptr) {
    return true;
  }

  for (const fbs::KeyVarPairDef* pair : *in->values()) {
    CHECK(pair->key());
    const HashValue key_hash = HashValue(pair->key()->hash());

    const void* variant_def = pair->value();
    CHECK(key_hash.get() != 0) << "Invalid/missing key.";
    CHECK(variant_def != nullptr) << "No value specified.";

    Var var;
    if (TryReadFbs(pair->value_type(), variant_def, &var)) {
      (*out)[key_hash] = std::move(var);
    }
  }
  return true;
}

}  // namespace redux
