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

#include "lullaby/modules/script/lull/script_compiler.h"
#include "lullaby/modules/script/lull/script_env.h"
#include "lullaby/modules/script/lull/script_types.h"

namespace lull {

static const uint8_t kByteCodeMarker = 0;

ScriptCompiler::ScriptCompiler(ScriptByteCode* code)
    : code_(code), writer_(code) {}

void ScriptCompiler::Process(TokenType type, const void* ptr,
                             string_view token) {
  if (error_) {
    return;
  }

  if (code_->empty()) {
    writer_(&kByteCodeMarker, 0);
  }

  int code = type;
  writer_(&code, 0);

  switch (type) {
    case kBool:
      writer_(reinterpret_cast<const bool*>(ptr), 0);
      break;
    case kInt8:
      writer_(reinterpret_cast<const int8_t*>(ptr), 0);
      break;
    case kUint8:
      writer_(reinterpret_cast<const uint8_t*>(ptr), 0);
      break;
    case kInt16:
      writer_(reinterpret_cast<const int16_t*>(ptr), 0);
      break;
    case kUint16:
      writer_(reinterpret_cast<const uint16_t*>(ptr), 0);
      break;
    case kInt32:
      writer_(reinterpret_cast<const int32_t*>(ptr), 0);
      break;
    case kUint32:
      writer_(reinterpret_cast<const uint32_t*>(ptr), 0);
      break;
    case kInt64:
      writer_(reinterpret_cast<const int64_t*>(ptr), 0);
      break;
    case kUint64:
      writer_(reinterpret_cast<const uint64_t*>(ptr), 0);
      break;
    case kFloat:
      writer_(reinterpret_cast<const float*>(ptr), 0);
      break;
    case kDouble:
      writer_(reinterpret_cast<const double*>(ptr), 0);
      break;
    case kHashValue:
      writer_(reinterpret_cast<const HashValue*>(ptr), 0);
      break;
    case kSymbol: {
      const Symbol* symbol = reinterpret_cast<const Symbol*>(ptr);
      writer_(&symbol->name, 0);
    } break;
    case kString:
      writer_(reinterpret_cast<const string_view*>(ptr), 0);
      break;
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

template <typename Value>
static void DoProcess(ParserCallbacks::TokenType type, ParserCallbacks* builder,
                      LoadFromBuffer* reader, Value value) {
  (*reader)(&value, 0);
  builder->Process(type, &value, "");
}

static void DoProcess(ParserCallbacks::TokenType type, ParserCallbacks* builder,
                      LoadFromBuffer* reader, Symbol value) {
  (*reader)(&value.name, 0);
  value.value = Hash(value.name);
  builder->Process(type, &value, "");
}

void ScriptCompiler::Build(ParserCallbacks* builder) {
  LoadFromBuffer reader(code_);

  if (code_->empty()) {
    LOG(ERROR) << "Bytecode is empty.";
    return;
  }

  uint8_t marker = kByteCodeMarker;
  reader(&marker, 0);
  if (marker != kByteCodeMarker) {
    LOG(ERROR) << "Missing marker at start of bytecode.";
    return;
  }

  bool done = false;
  while (!done) {
    int code = 0;
    reader(&code, 0);

    const TokenType type = static_cast<TokenType>(code);
    switch (type) {
      case kBool: {
        DoProcess(type, builder, &reader, false);
        break;
      }
      case kInt8: {
        DoProcess(type, builder, &reader, int8_t(0));
        break;
      }
      case kUint8: {
        DoProcess(type, builder, &reader, uint8_t(0));
        break;
      }
      case kInt16: {
        DoProcess(type, builder, &reader, int16_t(0));
        break;
      }
      case kUint16: {
        DoProcess(type, builder, &reader, uint16_t(0));
        break;
      }
      case kInt32: {
        DoProcess(type, builder, &reader, int32_t(0));
        break;
      }
      case kUint32: {
        DoProcess(type, builder, &reader, uint32_t(0));
        break;
      }
      case kInt64: {
        DoProcess(type, builder, &reader, int64_t(0));
        break;
      }
      case kUint64: {
        DoProcess(type, builder, &reader, uint64_t(0));
        break;
      }
      case kFloat: {
        DoProcess(type, builder, &reader, 0.f);
        break;
      }
      case kDouble: {
        DoProcess(type, builder, &reader, 0.0);
        break;
      }
      case kHashValue: {
        DoProcess<HashValue>(type, builder, &reader, 0);
        break;
      }
      case kSymbol: {
        DoProcess(type, builder, &reader, Symbol());
        break;
      }
      case kString: {
        DoProcess(type, builder, &reader, string_view());
        break;
      }
      case kPush: {
        builder->Process(type, nullptr, "");
        break;
      }
      case kPop: {
        builder->Process(type, nullptr, "");
        break;
      }
      case kPushArray: {
        builder->Process(type, nullptr, "");
        break;
      }
      case kPopArray: {
        builder->Process(type, nullptr, "");
        break;
      }
      case kPushMap: {
        builder->Process(type, nullptr, "");
        break;
      }
      case kPopMap: {
        builder->Process(type, nullptr, "");
        break;
      }
      case kEof: {
        builder->Process(type, nullptr, "");
        done = true;
        break;
      }
    }
  }
}

void ScriptCompiler::Error(string_view token, string_view message) {
  LOG(WARNING) << "Error parsing " << token.to_string() << ": " <<
      message.to_string();
  code_->clear();
  error_ = true;
}


bool ScriptCompiler::IsByteCode(Span<uint8_t> bytes) {
  return !bytes.empty() && bytes[0] == kByteCodeMarker;
}

}  // namespace lull
