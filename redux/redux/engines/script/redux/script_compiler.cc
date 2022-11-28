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

#include "redux/engines/script/redux/script_compiler.h"

#include "redux/engines/script/redux/script_env.h"
#include "redux/engines/script/redux/script_types.h"

namespace redux {

static const uint8_t kByteCodeMarker[] = {')', ')', '(', '('};

class ByteCodeWriter {
 public:
  explicit ByteCodeWriter(ScriptCompiler::CodeBuffer* code) : code_(code) {
    if (code_->empty()) {
      code_->emplace_back(kByteCodeMarker[0]);
      code_->emplace_back(kByteCodeMarker[1]);
      code_->emplace_back(kByteCodeMarker[2]);
      code_->emplace_back(kByteCodeMarker[3]);
    }
  }

  template <typename T>
  void operator()(const T& value) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
    code_->insert(code_->end(), bytes, bytes + sizeof(T));
  }

  void operator()(std::string_view str) {
    (*this)(str.size());
    if (!str.empty()) {
      code_->insert(code_->end(), str.begin(), str.end());
    }
  }

 private:
  ScriptCompiler::CodeBuffer* code_ = nullptr;
};

class ByteCodeReader {
 public:
  explicit ByteCodeReader(ScriptCompiler::CodeBuffer* code,
                          ParserCallbacks* builder)
      : code_(code), builder_(builder) {
    if (code_->size() <= 4) {
      LOG(ERROR) << "Bytecode is empty.";
      return;
    }
    CHECK((*code_)[0] == kByteCodeMarker[0]);
    CHECK((*code_)[1] == kByteCodeMarker[1]);
    CHECK((*code_)[2] == kByteCodeMarker[2]);
    CHECK((*code_)[3] == kByteCodeMarker[3]);
    read_head_ = 4;
  }

  template <typename T>
  T Process(ParserCallbacks::TokenType type) {
    if constexpr (std::is_same_v<T, std::nullptr_t>) {
      builder_->Process(type, nullptr, "");
      return nullptr;
    } else if constexpr (std::is_same_v<T, HashValue>) {
      T value = HashValue(Read<HashValue::Rep>());
      builder_->Process(type, &value, "");
      return value;
    } else if constexpr (std::is_same_v<T, Symbol>) {
      T value = Symbol(HashValue(Read<HashValue::Rep>()));
      builder_->Process(type, &value, "");
      return value;
    } else if constexpr (std::is_same_v<T, std::string_view>) {
      std::string_view view;
      const size_t size = Read<size_t>();
      if (size > 0) {
        const char* str = reinterpret_cast<const char*>(&(*code_)[read_head_]);
        view = std::string_view(str, size);
        read_head_ += size;
      }
      builder_->Process(type, &view, "");
      return view;
    } else {
      T value = Read<T>();
      builder_->Process(type, &value, "");
      return value;
    }
  }

  template <typename T>
  T Read() {
    const size_t size = sizeof(T);
    const T* ptr = reinterpret_cast<const T*>(&(*code_)[read_head_]);
    read_head_ += size;
    return *ptr;
  }

 private:
  ScriptCompiler::CodeBuffer* code_ = nullptr;
  ParserCallbacks* builder_ = nullptr;
  size_t read_head_ = 0;
};

ScriptCompiler::ScriptCompiler(CodeBuffer* code) : code_(code) {}

void ScriptCompiler::Process(TokenType type, const void* ptr,
                             std::string_view token) {
  if (error_) {
    return;
  }

  ByteCodeWriter writer(code_);

  const int code_value = type;
  writer(code_value);

  switch (type) {
    case kBool:
      writer(*reinterpret_cast<const bool*>(ptr));
      break;
    case kInt8:
      writer(*reinterpret_cast<const int8_t*>(ptr));
      break;
    case kUint8:
      writer(*reinterpret_cast<const uint8_t*>(ptr));
      break;
    case kInt16:
      writer(*reinterpret_cast<const int16_t*>(ptr));
      break;
    case kUint16:
      writer(*reinterpret_cast<const uint16_t*>(ptr));
      break;
    case kInt32:
      writer(*reinterpret_cast<const int32_t*>(ptr));
      break;
    case kUint32:
      writer(*reinterpret_cast<const uint32_t*>(ptr));
      break;
    case kInt64:
      writer(*reinterpret_cast<const int64_t*>(ptr));
      break;
    case kUint64:
      writer(*reinterpret_cast<const uint64_t*>(ptr));
      break;
    case kFloat:
      writer(*reinterpret_cast<const float*>(ptr));
      break;
    case kDouble:
      writer(*reinterpret_cast<const double*>(ptr));
      break;
    case kHashValue:
      writer(reinterpret_cast<const HashValue*>(ptr)->get());
      break;
    case kSymbol:
      writer(reinterpret_cast<const Symbol*>(ptr)->value.get());
      break;
    case kString:
      writer(*reinterpret_cast<const std::string_view*>(ptr));
      break;
    case kNull:
    case kEof:
    case kPush:
    case kPop:
    case kPushArray:
    case kPopArray:
    case kPushMap:
    case kPopMap:
      break;
  }
}

void ScriptCompiler::Build(ParserCallbacks* builder) {
  ByteCodeReader reader(code_, builder);

  bool done = false;
  while (!done) {
    int code = reader.Read<int>();

    const TokenType type = static_cast<TokenType>(code);
    switch (type) {
      case kBool:
        reader.Process<bool>(type);
        break;
      case kInt8:
        reader.Process<int8_t>(type);
        break;
      case kUint8:
        reader.Process<uint8_t>(type);
        break;
      case kInt16:
        reader.Process<int16_t>(type);
        break;
      case kUint16:
        reader.Process<uint16_t>(type);
        break;
      case kInt32:
        reader.Process<int32_t>(type);
        break;
      case kUint32:
        reader.Process<uint32_t>(type);
        break;
      case kInt64:
        reader.Process<int64_t>(type);
        break;
      case kUint64:
        reader.Process<uint64_t>(type);
        break;
      case kFloat:
        reader.Process<float>(type);
        break;
      case kDouble:
        reader.Process<double>(type);
        break;
      case kHashValue:
        reader.Process<HashValue>(type);
        break;
      case kSymbol:
        reader.Process<Symbol>(type);
        break;
      case kString:
        reader.Process<std::string_view>(type);
        break;
      case kNull:
      case kPush:
      case kPop:
      case kPushArray:
      case kPopArray:
      case kPushMap:
      case kPopMap:
      case kEof:
        reader.Process<std::nullptr_t>(type);
        break;
    }

    if (type == kEof) {
      done = true;
    }
  }
}

void ScriptCompiler::Error(std::string_view token, std::string_view message) {
  LOG(WARNING) << "Error parsing " << token << ": " << message;
  code_->clear();
  error_ = true;
}

bool ScriptCompiler::IsByteCode(absl::Span<const uint8_t> bytes) {
  return bytes.size() > 4 && bytes[0] == kByteCodeMarker[0] &&
         bytes[1] == kByteCodeMarker[1] && bytes[2] == kByteCodeMarker[2] &&
         bytes[3] == kByteCodeMarker[3];
}

}  // namespace redux
