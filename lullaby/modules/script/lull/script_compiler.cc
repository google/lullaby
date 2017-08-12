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

namespace lull {

static const uint8_t kByteCodeMarker = 0;

ScriptCompiler::ScriptCompiler(ScriptByteCode* code)
    : code_(code), writer_(code) {}

void ScriptCompiler::Process(CodeType type, const void* ptr,
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
    case kInt:
      writer_(reinterpret_cast<const int*>(ptr), 0);
      break;
    case kFloat:
      writer_(reinterpret_cast<const float*>(ptr), 0);
      break;
    case kSymbol:
      writer_(reinterpret_cast<const HashValue*>(ptr), 0);
      break;
    case kString:
      writer_(reinterpret_cast<const string_view*>(ptr), 0);
      break;
    case kEof:
    case kNil:
    case kPush:
    case kPop:
      break;
  }
}

template <typename Value>
static void DoProcess(ParserCallbacks::CodeType type, ParserCallbacks* builder,
                      LoadFromBuffer* reader, Value value) {
  (*reader)(&value, 0);
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

    const CodeType type = static_cast<CodeType>(code);
    switch (type) {
      case kBool: {
        DoProcess(type, builder, &reader, false);
        break;
      }
      case kInt: {
        DoProcess(type, builder, &reader, 0);
        break;
      }
      case kFloat: {
        DoProcess(type, builder, &reader, 0.f);
        break;
      }
      case kSymbol: {
        DoProcess<HashValue>(type, builder, &reader, 0);
        break;
      }
      case kString: {
        DoProcess(type, builder, &reader, string_view());
        break;
      }
      case kNil: {
        builder->Process(type, nullptr, "");
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
