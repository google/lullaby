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

#ifndef LULLABY_MODULES_JAVASCRIPT_CONVERT_H_
#define LULLABY_MODULES_JAVASCRIPT_CONVERT_H_

#include <map>
#include <unordered_map>
#include <type_traits>
#include <utility>

#include "lullaby/modules/dispatcher/event_wrapper.h"
#include "lullaby/modules/function/call_native_function.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/entity.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/math.h"
#include "lullaby/util/optional.h"
#include "lullaby/util/typeid.h"
#include "mathfu/glsl_mappings.h"
#include "v8.h"

namespace lull {
namespace script {
namespace javascript {

// Class used to lock the isolate when accessing V8 (for single thread access).
class IsolateLocker {
 public:
  explicit IsolateLocker(v8::Isolate* isolate)
      : locker_(isolate), scope_(isolate), handle_scope_(isolate) {}

 private:
  const v8::Locker locker_;
  const v8::Isolate::Scope scope_;
  const v8::HandleScope handle_scope_;
};

// Also acquires a local scoped Context from the provided global Context.
class IsolateContextLocker : public IsolateLocker {
 public:
  explicit IsolateContextLocker(v8::Isolate* isolate,
                               const v8::Global<v8::Context>& context)
      : IsolateLocker(isolate),
        context_(context.Get(isolate)),
        context_scope_(context_) {}

  const v8::Local<v8::Context>& GetLocalContext() { return context_; }

 private:
  const v8::Local<v8::Context> context_;
  const v8::Context::Scope context_scope_;
};

enum IsolateDataSlot : uint32_t {
  kArrayBufferAllocator = 0,
};

// Context used to perform conversion between v8 values and cpp types during
// a function call.
class JsContext {
 public:
  explicit JsContext(const v8::FunctionCallbackInfo<v8::Value>& args)
      : args_(args) {}

  template <typename T>
  bool ArgToCpp(const char* func_name, size_t arg_index, T* value);

  template <typename T>
  bool ReturnFromCpp(const char* func_name, const T& value);

  bool CheckNumArgs(const char* func_name, size_t expected_args) const;

 private:
  const v8::FunctionCallbackInfo<v8::Value>& args_;
};

template <typename T, bool IsEnum = std::is_enum<T>::value>
struct Convert;

template <typename T>
static v8::Local<v8::ArrayBuffer> CppVecToJsArrayBuffer(v8::Isolate* isolate,
                                                        std::vector<T> value) {
  const size_t count = value.size();
  const size_t byte_count = sizeof(T) * count;
  v8::ArrayBuffer::Allocator* allocator =
      reinterpret_cast<v8::ArrayBuffer::Allocator*>(
          isolate->GetData(kArrayBufferAllocator));
  DCHECK(allocator) << "ArrayBuffer::Allocator not set as isolate data.";
  T* bytes = reinterpret_cast<T*>(allocator->AllocateUninitialized(byte_count));
  for (size_t i = 0; i < count; i++) {
    bytes[i] = value[i];
  }

  return v8::ArrayBuffer::New(isolate, bytes, byte_count,
                              v8::ArrayBufferCreationMode::kInternalized);
}

template <typename T>
static v8::Local<v8::Array> CppToArray(v8::Isolate* isolate,
                                       const std::vector<T>& value) {
  const int count = static_cast<int>(value.size());
  v8::Local<v8::Array> array = v8::Array::New(isolate, count);
  for (int i = 0; i < count; i++) {
    array->Set(i, Convert<T>::CppToJs(isolate, value[i]));
  }
  return array;
}

template <typename JsType, typename CppType>
static v8::Local<v8::Value> CppToJsArrayHelper(
    v8::Isolate* isolate, const std::vector<CppType>& value) {
  return JsType::New(CppVecToJsArrayBuffer(isolate, value), 0, value.size());
}

template <>
struct Convert<bool, false> {
  static const char* GetJsTypeName() { return "boolean"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value, bool* value,
                      bool hash = false) {
    if (!js_value->IsBoolean()) {
      return false;
    }
    *value = js_value.As<v8::Boolean>()->Value();
    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate, bool value) {
    return v8::Boolean::New(isolate, value);
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return js_value->IsInt8Array() || js_value->IsUint8Array();
  }

  static v8::Local<v8::Value> CppToJsArray(v8::Isolate* isolate,
                                           const std::vector<bool>& value) {
    return CppToJsArrayHelper<v8::Int8Array>(isolate, value);
  }
};

template <>
struct Convert<int8_t, false> {
  static const char* GetJsTypeName() { return "number"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value, int8_t* value,
                      bool hash = false) {
    if (!js_value->IsNumber()) {
      return false;
    }
    auto context = isolate->GetCurrentContext();
    *value = static_cast<int8_t>(js_value->Int32Value(context).ToChecked());
    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate, int8_t value) {
    return v8::Int32::New(isolate, value);
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return js_value->IsInt8Array();
  }

  static v8::Local<v8::Value> CppToJsArray(v8::Isolate* isolate,
                                           const std::vector<int8_t>& value) {
    return CppToJsArrayHelper<v8::Int8Array>(isolate, value);
  }
};

template <>
struct Convert<int16_t, false> {
  static const char* GetJsTypeName() { return "number"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value, int16_t* value,
                      bool hash = false) {
    if (!js_value->IsNumber()) {
      return false;
    }
    auto context = isolate->GetCurrentContext();
    *value = static_cast<int16_t>(js_value->Int32Value(context).ToChecked());
    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate, int16_t value) {
    return v8::Int32::New(isolate, value);
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return js_value->IsInt16Array();
  }

  static v8::Local<v8::Value> CppToJsArray(v8::Isolate* isolate,
                                           const std::vector<int16_t>& value) {
    return CppToJsArrayHelper<v8::Int16Array>(isolate, value);
  }
};

template <>
struct Convert<int32_t, false> {
  static const char* GetJsTypeName() { return "number"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value, int32_t* value,
                      bool hash = false) {
    if (!js_value->IsNumber()) {
      return false;
    }
    auto context = isolate->GetCurrentContext();
    *value = js_value->Int32Value(context).ToChecked();
    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate, int32_t value) {
    return v8::Int32::New(isolate, value);
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return js_value->IsInt32Array();
  }

  static v8::Local<v8::Value> CppToJsArray(v8::Isolate* isolate,
                                           const std::vector<int32_t>& value) {
    return CppToJsArrayHelper<v8::Int32Array>(isolate, value);
  }
};

template <typename T>
struct Convert<T, true> {
  static const char* GetJsTypeName() { return "number"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value, T* value,
                      bool hash = false) {
    if (!js_value->IsNumber()) {
      return false;
    }
    auto context = isolate->GetCurrentContext();
    *value = static_cast<T>(js_value->Int32Value(context).ToChecked());
    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate, T value) {
    return v8::Int32::New(isolate, value);
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return js_value->IsInt32Array();
  }

  static v8::Local<v8::Value> CppToJsArray(v8::Isolate* isolate,
                                           const std::vector<T>& value) {
    return CppToJsArrayHelper<v8::Int32Array>(isolate, value);
  }
};

template <>
struct Convert<int64_t, false> {
  static const char* GetJsTypeName() { return "number"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value, int64_t* value,
                      bool hash = false) {
    if (!js_value->IsNumber()) {
      return false;
    }
    auto context = isolate->GetCurrentContext();
    *value = js_value->IntegerValue(context).ToChecked();
    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate, int64_t value) {
    return v8::Number::New(isolate, static_cast<double>(value));
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return js_value->IsFloat64Array();
  }

  static v8::Local<v8::Value> CppToJsArray(v8::Isolate* isolate,
                                           const std::vector<int64_t>& value) {
    return CppToArray(isolate, value);
  }
};

template <>
struct Convert<uint8_t, false> {
  static const char* GetJsTypeName() { return "number"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value, uint8_t* value,
                      bool hash = false) {
    if (!js_value->IsNumber()) {
      return false;
    }
    auto context = isolate->GetCurrentContext();
    *value = static_cast<uint8_t>(js_value->Uint32Value(context).ToChecked());
    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate, uint8_t value) {
    return v8::Uint32::New(isolate, value);
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return js_value->IsUint8Array();
  }

  static v8::Local<v8::Value> CppToJsArray(v8::Isolate* isolate,
                                           const std::vector<uint8_t>& value) {
    return CppToJsArrayHelper<v8::Uint8Array>(isolate, value);
  }
};

template <>
struct Convert<uint16_t, false> {
  static const char* GetJsTypeName() { return "number"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value, uint16_t* value,
                      bool hash = false) {
    if (!js_value->IsNumber()) {
      return false;
    }
    auto context = isolate->GetCurrentContext();
    *value = static_cast<uint16_t>(js_value->Uint32Value(context).ToChecked());
    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate, uint16_t value) {
    return v8::Uint32::New(isolate, value);
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return js_value->IsUint16Array();
  }

  static v8::Local<v8::Value> CppToJsArray(v8::Isolate* isolate,
                                           const std::vector<uint16_t>& value) {
    return CppToJsArrayHelper<v8::Uint16Array>(isolate, value);
  }
};

template <>
struct Convert<uint32_t, false> {
  static const char* GetJsTypeName() { return "number"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value, uint32_t* value,
                      bool hash = false) {
    auto context = isolate->GetCurrentContext();
    // Hash if we receive a string js value which is not already a hash.
    if (hash && js_value->IsString()) {
      std::string s = *v8::String::Utf8Value(isolate, js_value);
      bool is_numeric =
          (s.find_first_not_of("-0123456789") == std::string::npos);
      if (!is_numeric) {
        *value = Hash(s.c_str());
      } else {
        *value = js_value->Int32Value(context).ToChecked();
      }
      return true;
    }
    if (!js_value->IsNumber()) {
      return false;
    }
    *value = js_value->Uint32Value(context).ToChecked();
    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate, uint32_t value) {
    return v8::Uint32::New(isolate, value);
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return js_value->IsUint32Array();
  }

  static v8::Local<v8::Value> CppToJsArray(v8::Isolate* isolate,
                                           const std::vector<uint32_t>& value) {
    return CppToJsArrayHelper<v8::Uint32Array>(isolate, value);
  }
};

template <>
struct Convert<uint64_t, false> {
  static const char* GetJsTypeName() { return "number"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value, uint64_t* value,
                      bool hash = false) {
    if (!js_value->IsNumber()) {
      return false;
    }
    // This cast to v8::Number is safe because it is guarded by the IsNumber
    // check above.
    *value = static_cast<uint64_t>(js_value.As<v8::Number>()->Value());
    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate, uint64_t value) {
    return v8::Number::New(isolate, static_cast<double>(value));
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return false;
  }

  static v8::Local<v8::Value> CppToJsArray(v8::Isolate* isolate,
                                           const std::vector<uint64_t>& value) {
    return CppToArray(isolate, value);
  }
};

template <>
struct Convert<float, false> {
  static const char* GetJsTypeName() { return "number"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value, float* value,
                      bool hash = false) {
    if (!js_value->IsNumber()) {
      return false;
    }
    // This cast to v8::Number is safe because it is guarded by the IsNumber
    // check above.
    *value = static_cast<float>(js_value.As<v8::Number>()->Value());
    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate, float value) {
    return v8::Number::New(isolate, static_cast<double>(value));
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return js_value->IsFloat32Array();
  }

  static v8::Local<v8::Value> CppToJsArray(v8::Isolate* isolate,
                                           const std::vector<float>& value) {
    return CppToJsArrayHelper<v8::Float32Array>(isolate, value);
  }
};

template <>
struct Convert<double, false> {
  static const char* GetJsTypeName() { return "number"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value, double* value,
                      bool hash = false) {
    if (!js_value->IsNumber()) {
      return false;
    }
    // This cast to v8::Number is safe because it is guarded by the IsNumber
    // check above.
    *value = js_value.As<v8::Number>()->Value();
    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate, double value) {
    return v8::Number::New(isolate, value);
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return js_value->IsFloat64Array();
  }

  static v8::Local<v8::Value> CppToJsArray(v8::Isolate* isolate,
                                           const std::vector<double>& value) {
    return CppToJsArrayHelper<v8::Float64Array>(isolate, value);
  }
};

template <>
struct Convert<Clock::duration, false> {
  static const char* GetJsTypeName() { return "number"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value,
                      Clock::duration* value, bool hash = false) {
    if (!js_value->IsNumber()) {
      return false;
    }
    auto context = isolate->GetCurrentContext();
    *value = std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<int64_t, std::nano>(
            js_value->IntegerValue(context).ToChecked()));
    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate,
                                      const Clock::duration& value) {
    return v8::Number::New(isolate, static_cast<double>(value.count()));
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return js_value->IsFloat64Array();
  }

  static v8::Local<v8::Value> CppToJsArray(
      v8::Isolate* isolate, const std::vector<Clock::duration>& value) {
    return CppToArray(isolate, value);
  }
};

template <>
struct Convert<Entity, false> {
  static const char* GetJsTypeName() { return "number"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value, Entity* value,
                      bool hash = false) {
    if (!js_value->IsNumber()) {
      return false;
    }
    auto context = isolate->GetCurrentContext();
    *value = js_value->Uint32Value(context).ToChecked();
    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate,
                                      const Entity& value) {
    return v8::Uint32::New(isolate, value.AsUint32());
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return js_value->IsUint32Array();
  }

  static v8::Local<v8::Value> CppToJsArray(v8::Isolate* isolate,
                                           const std::vector<Entity>& value) {
    return CppToJsArrayHelper<v8::Uint32Array>(isolate, value);
  }
};

template <>
struct Convert<std::string, false> {
  static const char* GetJsTypeName() { return "string"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value, std::string* value,
                      bool hash = false) {
    if (!js_value->IsString()) {
      return false;
    }
    *value = *v8::String::Utf8Value(isolate, js_value);
    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate,
                                      const std::string& value) {
    return v8::String::NewFromUtf8(isolate, value.c_str());
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return false;
  }

  static v8::Local<v8::Value> CppToJsArray(
      v8::Isolate* isolate, const std::vector<std::string>& value) {
    return CppToArray(isolate, value);
  }
};

// The |js_value| parameter must be a v8::Object.
template <typename T>
static size_t ExtractMathArgs(v8::Isolate* isolate,
                              const v8::Local<v8::Value>& js_value,
                              T* x = nullptr, T* y = nullptr, T* z = nullptr,
                              T* w = nullptr, T* s = nullptr) {
  auto context = isolate->GetCurrentContext();
  v8::Local<v8::Object> obj = js_value->ToObject(context).ToLocalChecked();

  auto get_value = [=](const char* name, T* value) {
    v8::Local<v8::String> key = v8::String::NewFromUtf8(isolate, name);
    if (!obj->Has(key)) {
      return false;
    }
    v8::Local<v8::Value> js_value = obj->Get(key);
    if (!Convert<T>::JsToCpp(isolate, js_value, value)) {
      return false;
    }
    return true;
  };

  size_t count = 0;

  if (x && get_value("x", x)) count++;
  if (y && get_value("y", y)) count++;
  if (z && get_value("z", z)) count++;
  if (w && get_value("w", w)) count++;
  if (s && get_value("s", s)) count++;

  return count;
}

template <typename T>
static v8::Local<v8::Value> SetMathArgs(
    v8::Isolate* isolate, const T* x = nullptr, const T* y = nullptr,
    const T* z = nullptr, const T* w = nullptr, const T* s = nullptr) {
  v8::Local<v8::Object> obj = v8::Object::New(isolate);

  auto set_value = [=](const char* name, T value) {
    v8::Local<v8::String> key = v8::String::NewFromUtf8(isolate, name);
    v8::Local<v8::Value> js_value = Convert<T>::CppToJs(isolate, value);
    obj->Set(key, js_value);
  };

  if (x) set_value("x", *x);
  if (y) set_value("y", *y);
  if (z) set_value("z", *z);
  if (w) set_value("w", *w);
  if (s) set_value("s", *s);

  return obj;
}

template <typename T>
struct Convert<lull::Optional<T>, false> {
  static const char* GetJsTypeName() { return "value or nil"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value,
                      lull::Optional<T>* value, bool hash = false) {
    T t;
    if (js_value->IsNull()) {
      value->reset();
      return true;
    } else if (!Convert<T>::JsToCpp(isolate, js_value, &t)) {
      return false;
    }
    *value = t;
    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate,
                                      const lull::Optional<T>& value) {
    if (value) {
      return Convert<T>::CppToJs(isolate, *value);
    } else {
      return v8::Null(isolate);
    }
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return false;
  }

  static v8::Local<v8::Value> CppToJsArray(
      v8::Isolate* isolate, const std::vector<lull::Optional<T>>& value) {
    return CppToArray(isolate, value);
  }
};

template <typename T>
struct Convert<mathfu::Vector<T, 2>, false> {
  static const char* GetJsTypeName() { return "array of number"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value,
                      mathfu::Vector<T, 2>* value, bool hash = false) {
    if (!js_value->IsObject()) {
      return false;
    }

    T x, y;
    size_t num = ExtractMathArgs(isolate, js_value, &x, &y);
    if (num != 2) {
      return false;
    }
    value->x = x;
    value->y = y;

    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate,
                                      const mathfu::Vector<T, 2>& value) {
    return SetMathArgs(isolate, &value.x, &value.y);
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return false;
  }

  static v8::Local<v8::Value> CppToJsArray(
      v8::Isolate* isolate, const std::vector<mathfu::Vector<T, 2>>& value) {
    return CppToArray(isolate, value);
  }
};

template <typename T>
struct Convert<mathfu::Vector<T, 3>, false> {
  static const char* GetJsTypeName() { return "array of number"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value,
                      mathfu::Vector<T, 3>* value, bool hash = false) {
    if (!js_value->IsObject()) {
      return false;
    }

    T x, y, z;
    size_t num = ExtractMathArgs(isolate, js_value, &x, &y, &z);
    if (num != 3) {
      return false;
    }
    value->x = x;
    value->y = y;
    value->z = z;

    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate,
                                      const mathfu::Vector<T, 3>& value) {
    return SetMathArgs(isolate, &value.x, &value.y, &value.z);
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return false;
  }

  static v8::Local<v8::Value> CppToJsArray(
      v8::Isolate* isolate, const std::vector<mathfu::Vector<T, 3>>& value) {
    return CppToArray(isolate, value);
  }
};

template <typename T>
struct Convert<mathfu::Vector<T, 4>, false> {
  static const char* GetJsTypeName() { return "array of number"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value,
                      mathfu::Vector<T, 4>* value, bool hash = false) {
    if (!js_value->IsObject()) {
      return false;
    }

    T x, y, z, w;
    size_t num = ExtractMathArgs(isolate, js_value, &x, &y, &z, &w);
    if (num != 4) {
      return false;
    }
    value->x = x;
    value->y = y;
    value->z = z;
    value->w = w;

    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate,
                                      const mathfu::Vector<T, 4>& value) {
    return SetMathArgs(isolate, &value.x, &value.y, &value.z, &value.w);
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return false;
  }

  static v8::Local<v8::Value> CppToJsArray(
      v8::Isolate* isolate, const std::vector<mathfu::Vector<T, 4>>& value) {
    return CppToArray(isolate, value);
  }
};

template <>
struct Convert<mathfu::quat, false> {
  static const char* GetJsTypeName() { return "array of number"; }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value, mathfu::quat* value,
                      bool hash = false) {
    if (!js_value->IsObject()) {
      return false;
    }

    float x, y, z, s;
    size_t num =
        ExtractMathArgs<float>(isolate, js_value, &x, &y, &z, nullptr, &s);
    if (num != 4) {
      return false;
    }
    value->set_vector(mathfu::vec3(x, y, z));
    value->set_scalar(s);

    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate,
                                      const mathfu::quat& value) {
    return SetMathArgs<float>(isolate, &value.vector().x, &value.vector().y,
                              &value.vector().z, nullptr, &value.scalar());
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return false;
  }

  static v8::Local<v8::Value> CppToJsArray(
      v8::Isolate* isolate, const std::vector<mathfu::quat>& value) {
    return CppToArray(isolate, value);
  }
};

template <typename T>
struct Convert<mathfu::Rect<T>, false> {
  static const char* GetJsTypeName() {
    static std::string type = std::string("map like {pos:") +
                              Convert<mathfu::Vector<T, 2>>::GetJsTypeName() +
                              std::string(", size:") +
                              Convert<mathfu::Vector<T, 2>>::GetJsTypeName() +
                              std::string("}");
    return type.c_str();
  }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value,
                      mathfu::Rect<T>* value, bool hash = false) {
    if (!js_value->IsObject()) {
      return false;
    }

    v8::Local<v8::String> pos_key = v8::String::NewFromUtf8(isolate, "pos");
    v8::Local<v8::String> size_key = v8::String::NewFromUtf8(isolate, "size");
    v8::Local<v8::Object> js_obj = js_value->ToObject(isolate);
    if (!js_obj->Has(pos_key) || !js_obj->Has(size_key)) {
      return false;
    }
    if (!Convert<mathfu::Vector<T, 2>>::JsToCpp(isolate, js_obj->Get(pos_key),
                                                &value->pos) ||
        !Convert<mathfu::Vector<T, 2>>::JsToCpp(isolate, js_obj->Get(size_key),
                                                &value->size)) {
      return false;
    }

    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate,
                                      const mathfu::Rect<T>& value) {
    v8::Local<v8::String> pos_key = v8::String::NewFromUtf8(isolate, "pos");
    v8::Local<v8::String> size_key = v8::String::NewFromUtf8(isolate, "size");
    v8::Local<v8::Object> obj = v8::Object::New(isolate);
    obj->Set(pos_key,
             Convert<mathfu::Vector<T, 2>>::CppToJs(isolate, value.pos));
    obj->Set(size_key,
             Convert<mathfu::Vector<T, 2>>::CppToJs(isolate, value.size));
    return obj;
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return false;
  }

  static v8::Local<v8::Value> CppToJsArray(
      v8::Isolate* isolate, const std::vector<mathfu::Rect<T>>& value) {
    return CppToArray(isolate, value);
  }
};

template <>
struct Convert<lull::Aabb, false> {
  static const char* GetJsTypeName() {
    return "map like {min:vec3, max:vec3}";
  }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value, lull::Aabb* value,
                      bool hash = false) {
    if (!js_value->IsObject()) {
      return false;
    }

    v8::Local<v8::Object> js_obj = js_value->ToObject(isolate);
    v8::Local<v8::String> min = v8::String::NewFromUtf8(isolate, "min");
    v8::Local<v8::String> max = v8::String::NewFromUtf8(isolate, "max");
    if (!js_obj->Has(min) || !js_obj->Has(max)) {
      return false;
    }
    if (!Convert<mathfu::vec3>::JsToCpp(isolate, js_obj->Get(min),
                                        &value->min) ||
        !Convert<mathfu::vec3>::JsToCpp(isolate, js_obj->Get(max),
                                        &value->max)) {
      return false;
    }

    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate,
                                      const lull::Aabb& value) {
    v8::Local<v8::Object> obj = v8::Object::New(isolate);
    v8::Local<v8::String> min = v8::String::NewFromUtf8(isolate, "min");
    obj->Set(min, Convert<mathfu::vec3>::CppToJs(isolate, value.min));
    v8::Local<v8::String> max = v8::String::NewFromUtf8(isolate, "max");
    obj->Set(max, Convert<mathfu::vec3>::CppToJs(isolate, value.max));
    return obj;
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return false;
  }

  static v8::Local<v8::Value> CppToJsArray(
      v8::Isolate* isolate, const std::vector<lull::Aabb>& value) {
    return CppToArray(isolate, value);
  }
};

template <>
struct Convert<mathfu::mat4, false> {
  static const char* GetJsTypeName() {
    return "map like {c0:vec4, c1:vec4, c2:vec4, c3:vec4}";
  }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value, mathfu::mat4* value,
                      bool hash = false) {
    if (!js_value->IsObject()) {
      return false;
    }

    v8::Local<v8::Object> js_obj = js_value->ToObject(isolate);
    for (int i = 0; i < 4; i++) {
      std::stringstream stream;
      stream << "c" << i;
      v8::Local<v8::String> col_key =
          v8::String::NewFromUtf8(isolate, stream.str().c_str());
      if (!js_obj->Has(col_key)) {
        return false;
      }
      if (!Convert<mathfu::vec4>::JsToCpp(isolate, js_obj->Get(col_key),
                                          &value->GetColumn(i))) {
        return false;
      }
    }

    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate,
                                      const mathfu::mat4& value) {
    v8::Local<v8::Object> obj = v8::Object::New(isolate);
    for (int i = 0; i < 4; i++) {
      std::stringstream stream;
      stream << "c" << i;
      v8::Local<v8::String> col_key =
          v8::String::NewFromUtf8(isolate, stream.str().c_str());
      obj->Set(col_key,
               Convert<mathfu::vec4>::CppToJs(isolate, value.GetColumn(i)));
    }
    return obj;
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return false;
  }

  static v8::Local<v8::Value> CppToJsArray(
      v8::Isolate* isolate, const std::vector<mathfu::mat4>& value) {
    return CppToArray(isolate, value);
  }
};

template <typename T>
struct Convert<std::vector<T>, false> {
  static const char* GetJsTypeName() {
    static std::string type =
        std::string("array of ") + Convert<T>::GetJsTypeName();
    return type.c_str();
  }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value,
                      std::vector<T>* value, bool hash = false) {
    if (!js_value->IsTypedArray() && !js_value->IsArray()) {
      return false;
    }

    if (Convert<T>::IsTypedArray(js_value)) {
      v8::Local<v8::ArrayBufferView> buffer_view =
          v8::Local<v8::ArrayBufferView>::Cast(js_value);
      v8::Local<v8::ArrayBuffer> buffer = buffer_view->Buffer();
      v8::ArrayBuffer::Contents contents = buffer->GetContents();
      const size_t count = contents.ByteLength() / sizeof(T);
      T* data = reinterpret_cast<T*>(contents.Data());
      for (size_t i = 0; i < count; i++) {
        value->push_back(data[i]);
      }
    } else {
      v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(js_value);
      const int count = array->Length();
      for (int i = 0; i < count; i++) {
        v8::Local<v8::Value> item = array->Get(i);
        T cpp_value;
        if (!Convert<T>::JsToCpp(isolate, item, &cpp_value)) {
          return false;
        }
        value->push_back(cpp_value);
      }
    }
    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate,
                                      const std::vector<T>& value) {
    return Convert<T>::CppToJsArray(isolate, value);
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return false;
  }

  static v8::Local<v8::Value> CppToJsArray(
      v8::Isolate* isolate, const std::vector<std::vector<T>>& value) {
    return CppToArray(isolate, value);
  }
};

namespace detail {

struct JsArgv {
  // Convert zero variadic argument.
  static void ToJsArgv(v8::Isolate* isolate,
                       std::vector<v8::Local<v8::Value>>* argv) {}

  // Convert one variadic argument and add to argv.
  template <typename Arg, typename... Args>
  static void ToJsArgv(v8::Isolate* isolate,
                       std::vector<v8::Local<v8::Value>>* argv, Arg&& arg,
                       Args&&... args) {
    using ArgType = typename std::decay<Arg>::type;
    v8::Local<v8::Value> js_value = Convert<ArgType>::CppToJs(isolate, arg);
    argv->push_back(js_value);
    ToJsArgv(isolate, argv, std::forward<Args>(args)...);
  }
};

// Invoke Js Callback function given variadic Cpp arguments.
template <typename... Args>
v8::Local<v8::Value> InvokeJsCallback(
    v8::Isolate* isolate,
    const v8::Persistent<v8::Value, v8::CopyablePersistentTraits<v8::Value>>&
        persistent_js_value,
    Args&&... args) {
  v8::Local<v8::Value> js_value = persistent_js_value.Get(isolate);

  // Convert and collect the cpp variadic arguments in a v8::Value argv vector.
  std::vector<v8::Local<v8::Value>> argv;
  detail::JsArgv::ToJsArgv(isolate, &argv, std::forward<Args>(args)...);

  // Call the function with the argv.
  v8::Local<v8::Function> js_fn = v8::Local<v8::Function>::Cast(js_value);

  // Use the bound function as the receiver of the call.
  v8::Local<v8::Value> js_recv = js_fn->GetBoundFunction();
  if (js_recv.IsEmpty() || js_recv->IsUndefined()) {
    js_recv = js_fn;
  }

  v8::Local<v8::Value> js_ret =
      js_fn->Call(js_recv, static_cast<int>(argv.size()), argv.data());
  return js_ret;
}

class JsFunctionCallerBase {
 public:
  JsFunctionCallerBase(v8::Isolate* isolate, v8::Local<v8::Value> js_value)
      : isolate_(isolate), js_value_(isolate, js_value) {
    v8::Local<v8::Context> current = isolate_->GetCurrentContext();
    context.reset(new v8::Global<v8::Context>(isolate, current));
  }

 protected:
  v8::Isolate* isolate_;
  // v8::Global are not copyable, but we need to pass this object through
  // various function calls in call_native_function.h, so we hold it indirectly.
  std::shared_ptr<v8::Global<v8::Context>> context;
  const v8::Persistent<v8::Value, v8::CopyablePersistentTraits<v8::Value>>
      js_value_;
};

template <typename Sig>
class JsFunctionCaller;

template <typename Return, typename... Args>
class JsFunctionCaller<Return(Args...)> : public JsFunctionCallerBase {
 public:
  JsFunctionCaller(v8::Isolate* isolate, v8::Local<v8::Value> js_value)
      : JsFunctionCallerBase(isolate, js_value) {}

  Return operator()(Args... args) {
    // Js functions can be invoked from outside a running script, so we need
    // to reclaim the context.
    IsolateContextLocker lock(isolate_, *context);

    v8::Local<v8::Value> js_ret =
        InvokeJsCallback(isolate_, js_value_, std::forward<Args>(args)...);
    Return ret;
    Convert<Return>::JsToCpp(isolate_, js_ret, &ret);
    return ret;
  }
};

template <typename... Args>
class JsFunctionCaller<void(Args...)> : public JsFunctionCallerBase {
 public:
  JsFunctionCaller(v8::Isolate* isolate, v8::Local<v8::Value> js_value)
      : JsFunctionCallerBase(isolate, js_value) {}

  void operator()(Args... args) {
    // Js functions can be invoked from outside a running script, so we need
    // to reclaim the context.
    IsolateContextLocker lock(isolate_, *context);

    InvokeJsCallback(isolate_, js_value_, std::forward<Args>(args)...);
  }
};

template <typename FnSig>
struct FnHolder {
  std::function<FnSig> fn;
  v8::Persistent<v8::Value> persistent;

  FnHolder(const std::function<FnSig>& fn) : fn(fn) {}
};

}  // namespace detail.

template <typename FnSig>
struct Convert<std::function<FnSig>, false> {
  static const char* GetJsTypeName() {
    return "callback function";
  }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value,
                      std::function<FnSig>* value, bool hash = false) {
    if (!js_value->IsFunction()) {
      return false;
    }
    *value = detail::JsFunctionCaller<FnSig>(isolate, js_value);
    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate,
                                      const std::function<FnSig>& value) {
    // Create a pointer to the C++ callback function: value, that is released
    // when the corresponding JS function object is released.
    detail::FnHolder<FnSig>* holder = new detail::FnHolder<FnSig>(value);

    v8::Local<v8::Value> js_value = v8::Function::New(
        isolate,
        [](const v8::FunctionCallbackInfo<v8::Value>& args) {
          v8::Local<v8::External> external =
              v8::Local<v8::External>::Cast(args.Data());
          const detail::FnHolder<FnSig>* holder =
              reinterpret_cast<const detail::FnHolder<FnSig>*>(
                  external->Value());
          if (holder == nullptr) {
            return;
          }

          JsContext context(args);
          CallNativeFunction(&context, "anonymous function", holder->fn);
        },
        v8::External::New(isolate, reinterpret_cast<void*>(holder)));

    // Add a GC callback when the JS function object is garbage collected. We
    // release the pointer to the C++ callback function in it.
    IsolateLocker lock(isolate);
    holder->persistent.Reset(isolate, js_value);
    holder->persistent.template SetWeak<detail::FnHolder<FnSig>>(
        holder,
        [](const v8::WeakCallbackInfo<detail::FnHolder<FnSig>>& data) {
          detail::FnHolder<FnSig>* holder = data.GetParameter();
          holder->persistent.Reset();
          delete holder;
        },
        v8::WeakCallbackType::kParameter);

    return js_value;
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return false;
  }

  static v8::Local<v8::Value> CppToJsArray(
      v8::Isolate* isolate, const std::vector<std::function<FnSig>>& value) {
    return CppToArray(isolate, value);
  }
};

static void DumpObject(v8::Isolate* isolate, v8::Local<v8::Value> js_value) {
  auto context = isolate->GetCurrentContext();
  if (js_value->IsObject()) {
    v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(js_value);
    v8::Local<v8::Array> array = obj->GetPropertyNames();
    int count = array->Length();
    for (int i = 0; i < count; i++) {
      v8::Local<v8::Value> property = array->Get(i);
      v8::Local<v8::Value> value = obj->Get(property);
      if (property->IsString()) {
        DLOG(INFO) << "Property: " << *v8::String::Utf8Value(isolate, property);
      } else if (property->IsNumber()) {
        DLOG(INFO) << "Property: "
                   << property->Uint32Value(context).ToChecked();
      } else {
        DLOG(INFO) << "Property: unknown";
      }
      DumpObject(isolate, value);
    }
  } else if (js_value->IsArray()) {
  } else if (js_value.IsEmpty()) {
    DLOG(INFO) << "Empty";
  } else if (js_value->IsString()) {
    DLOG(INFO) << "Value: " << *v8::String::Utf8Value(isolate, js_value);
  } else if (js_value->IsNumber()) {
    DLOG(INFO) << "Value: " << js_value->Uint32Value(context).ToChecked();
  } else {
    DLOG(INFO) << "Value: unknown";
  }
}
// Returns a v8::Object that uses an interceptor to get/set values with hashed
// keys.
static v8::Local<v8::Object> NewSerializableObject(v8::Isolate* isolate) {
  v8::Local<v8::ObjectTemplate> object_template =
      v8::ObjectTemplate::New(isolate);
  object_template->SetHandler(v8::NamedPropertyHandlerConfiguration(
      [](v8::Local<v8::Name> property,
         const v8::PropertyCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        v8::Local<v8::String> name = property->ToString(isolate);

        // Lookup using the given name.
        v8::Local<v8::Value> ret_value;
        if (info.This()
                ->GetRealNamedProperty(context, name)
                .ToLocal(&ret_value)) {
          info.GetReturnValue().Set(ret_value);
          return;
        }
        // Otherwise, lookup using the hash of the name.
        HashValue hash = Hash(*v8::String::Utf8Value(info.GetIsolate(), name));
        v8::Local<v8::Value> hash_value =
            Convert<HashValue>::CppToJs(isolate, hash);
        v8::Local<v8::String> hash_name =
            hash_value->ToString(context).ToLocalChecked();
        if (info.This()
                ->GetRealNamedProperty(context, hash_name)
                .ToLocal(&ret_value)) {
          info.GetReturnValue().Set(ret_value);
          return;
        }
      },
      0, 0, 0, 0, v8::Local<v8::Value>(),
      static_cast<v8::PropertyHandlerFlags>(
          static_cast<int>(v8::PropertyHandlerFlags::kOnlyInterceptStrings) |
          static_cast<int>(v8::PropertyHandlerFlags::kNonMasking))));
  return object_template->NewInstance();
}

template <typename M>
static bool JsMapToCppMap(v8::Isolate* isolate,
                          const v8::Local<v8::Value>& js_value, M* value) {
  using K = typename M::key_type;
  using V = typename M::mapped_type;

  if (!js_value->IsObject()) {
    return false;
  }

  v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(js_value);
  v8::Local<v8::Array> array = obj->GetPropertyNames();
  int count = array->Length();
  for (int i = 0; i < count; i++) {
    v8::Local<v8::Value> property = array->Get(i);

    K name;
    if (!Convert<K>::JsToCpp(isolate, property, &name,
                             std::is_same<K, HashValue>::value)) {
      return false;
    }

    v8::MaybeLocal<v8::Value> prop_value = obj->Get(property);
    if (prop_value.IsEmpty()) {
      return false;
    }
    V cpp_value;
    if (!Convert<V>::JsToCpp(isolate, prop_value.ToLocalChecked(),
                             &cpp_value)) {
      return false;
    }
    value->emplace(name, cpp_value);
  }
  return true;
}

template <typename M>
static v8::Local<v8::Value> CppMapToJsMap(v8::Isolate* isolate,
                                          const M& value) {
  using K = typename M::key_type;
  using V = typename M::mapped_type;
  v8::Local<v8::Object> obj;
  // If the map is keyed by HashValues, use an object_template with a
  // NamedPropertyHandler that can use the hash of the key before performing a
  // lookup, so that on the JS side, we can look up a hash key by its
  // corresponding string key.
  if (std::is_same<K, HashValue>::value) {
    obj = NewSerializableObject(isolate);
  } else {
    obj = v8::Object::New(isolate);
  }
  for (const auto& kv : value) {
    auto js_key = Convert<K>::CppToJs(isolate, kv.first);
    auto js_value = Convert<V>::CppToJs(isolate, kv.second);
    obj->Set(js_key, js_value);
  }
  return obj;
}

template <typename K, typename V>
struct Convert<std::map<K, V>, false> {
  static const char* GetJsTypeName() {
    static std::string type = std::string("map of ") +
                              Convert<K>::GetJsTypeName() +
                              std::string(" to ") + Convert<V>::GetJsTypeName();
    return type.c_str();
  }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value,
                      std::map<K, V>* value, bool hash = false) {
    return JsMapToCppMap<std::map<K, V>>(isolate, js_value, value);
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate,
                                      const std::map<K, V>& value) {
    return CppMapToJsMap<std::map<K, V>>(isolate, value);
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return false;
  }

  static v8::Local<v8::Value> CppToJsArray(
      v8::Isolate* isolate, const std::vector<std::map<K, V>>& value) {
    return CppToArray(isolate, value);
  }
};

template <typename K, typename V>
struct Convert<std::unordered_map<K, V>, false> {
  static const char* GetJsTypeName() {
    static std::string type = std::string("map of ") +
                              Convert<K>::GetJsTypeName() +
                              std::string(" to ") + Convert<V>::GetJsTypeName();
    return type.c_str();
  }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value,
                      std::unordered_map<K, V>* value, bool hash = false) {
    return JsMapToCppMap<std::unordered_map<K, V>>(isolate, js_value, value);
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate,
                                      const std::unordered_map<K, V>& value) {
    return CppMapToJsMap<std::unordered_map<K, V>>(isolate, value);
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return false;
  }

  static v8::Local<v8::Value> CppToJsArray(
      v8::Isolate* isolate,
      const std::vector<std::unordered_map<K, V>>& value) {
    return CppToArray(isolate, value);
  }
};

template <>
struct Convert<EventWrapper, false> {
  static const char* GetJsTypeName() {
    static std::string type =
        std::string("map like {type:") + Convert<HashValue>::GetJsTypeName() +
        std::string(", data:") + Convert<VariantMap>::GetJsTypeName() +
        std::string("}");
    return type.c_str();
  }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value, EventWrapper* value,
                      bool hash = false) {
    if (!js_value->IsObject()) {
      return false;
    }

    v8::Local<v8::String> type_key = v8::String::NewFromUtf8(isolate, "type");
    v8::Local<v8::String> data_key = v8::String::NewFromUtf8(isolate, "data");
    v8::Local<v8::Object> js_obj = js_value->ToObject(isolate);
    if (!js_obj->Has(type_key) || !js_obj->Has(data_key)) {
      return false;
    }

    HashValue type;
    if (!Convert<HashValue>::JsToCpp(isolate, js_obj->Get(type_key), &type)) {
      return false;
    }

    VariantMap data;
    if (!Convert<VariantMap>::JsToCpp(isolate, js_obj->Get(data_key), &data)) {
      return false;
    }
    *value = EventWrapper(type);
    value->SetValues(data);
    return true;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate,
                                      const EventWrapper& value) {
    v8::Local<v8::String> type_key = v8::String::NewFromUtf8(isolate, "type");
    v8::Local<v8::String> data_key = v8::String::NewFromUtf8(isolate, "data");

    v8::Local<v8::Object> obj = v8::Object::New(isolate);
    obj->Set(type_key, Convert<HashValue>::CppToJs(isolate, value.GetTypeId()));
    obj->Set(data_key,
             Convert<VariantMap>::CppToJs(isolate, *value.GetValues()));
    return obj;
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return false;
  }

  static v8::Local<v8::Value> CppToJsArray(
      v8::Isolate* isolate, const std::vector<EventWrapper>& value) {
    return CppToArray(isolate, value);
  }
};

// Returns a deep copy of the object where strings have been replaced with
// their hash values.
static v8::Local<v8::Object> ToHashIndexedObject(v8::Isolate* isolate,
                                                 v8::Local<v8::Object> obj) {
  v8::Local<v8::Object> new_obj = NewSerializableObject(isolate);

  v8::Local<v8::Array> array = obj->GetPropertyNames();
  int count = array->Length();
  for (int i = 0; i < count; i++) {
    v8::Local<v8::Value> property = array->Get(i);
    v8::Local<v8::Value> value = obj->Get(property);
    if (property->IsString()) {
      std::string s = *v8::String::Utf8Value(isolate, property);
      bool is_numeric =
          (s.find_first_not_of("-0123456789") == std::string::npos);
      if (!is_numeric) {
        HashValue hash = Hash(*v8::String::Utf8Value(isolate, property));
        property = Convert<HashValue>::CppToJs(isolate, hash);
      }
    }
    if (value->IsObject()) {
      value = ToHashIndexedObject(isolate, value.As<v8::Object>());
    }
    new_obj->Set(property, value);
  }
  return new_obj;
}

namespace detail {

struct JsToCppSerializable {
  v8::Isolate* isolate;
  v8::Local<v8::Object> js_hash_obj;
  bool ret;
  explicit JsToCppSerializable(v8::Isolate* isolate,
                               const v8::Local<v8::Object>& js_obj)
      : isolate(isolate), ret(true) {
    // Deep copy the object, with string keys replaced by their hashes.
    js_hash_obj = ToHashIndexedObject(isolate, js_obj);
  }

  template <typename T>
  void operator()(T* ptr, HashValue key) {
    v8::Local<v8::Value> key_value = Convert<HashValue>::CppToJs(isolate, key);
    v8::Local<v8::Value> prop_value = js_hash_obj->Get(key_value);
    if (prop_value.IsEmpty()) {
      ret = false;
      return;
    }
    if (!Convert<T>::JsToCpp(isolate, prop_value, ptr)) {
      ret = false;
      return;
    }
  }

  bool IsDestructive() { return true; }
};

struct CppToJsSerializable {
  v8::Isolate* isolate;
  const v8::Local<v8::Object>& js_obj;
  CppToJsSerializable(v8::Isolate* isolate, const v8::Local<v8::Object>& js_obj)
      : isolate(isolate), js_obj(js_obj) {}

  template <typename T>
  void operator()(T* ptr, HashValue key) {
    auto js_key = Convert<HashValue>::CppToJs(isolate, key);
    auto js_value = Convert<T>::CppToJs(isolate, *ptr);
    js_obj->Set(js_key, js_value);
  }

  bool IsDestructive() { return false; }
};

}  // namespace detail

template <typename T>
struct Convert<T, false> {
  static const char* GetJsTypeName() {
    static std::string type = std::string("map like ") + GetTypeName<T>();
    return type.c_str();
  }

  static bool JsToCpp(v8::Isolate* isolate,
                      const v8::Local<v8::Value>& js_value, T* value,
                      bool hash = false) {
    if (!js_value->IsObject()) {
      return false;
    }

    v8::Local<v8::Object> js_obj = v8::Local<v8::Object>::Cast(js_value);
    detail::JsToCppSerializable serializer(isolate, js_obj);
    value->Serialize(serializer);
    return serializer.ret;
  }

  static v8::Local<v8::Value> CppToJs(v8::Isolate* isolate, const T& value) {
    v8::Local<v8::Object> obj = NewSerializableObject(isolate);

    detail::CppToJsSerializable serializer(isolate, obj);

    // CppToJsSerializable doesn't actually alter the value, but it still needs
    // a non-const reference.
    const_cast<T&>(value).Serialize(serializer);
    return obj;
  }

  static bool IsTypedArray(const v8::Local<v8::Value>& js_value) {
    return false;
  }

  static v8::Local<v8::Value> CppToJsArray(v8::Isolate* isolate,
                                           const std::vector<T>& value) {
    return CppToArray(isolate, value);
  }
};

template <typename T>
bool JsContext::ArgToCpp(const char* func_name, size_t arg, T* value) {
  v8::Isolate* isolate = args_.GetIsolate();
  v8::Local<v8::Value> js_value = args_[static_cast<int>(arg)];
  if (!Convert<T>::JsToCpp(isolate, js_value, value)) {
    LOG(ERROR) << func_name << " expects the type of arg " << arg << " to be "
               << Convert<T>::GetJsTypeName();
    return false;
  }
  return true;
}

template <typename T>
bool JsContext::ReturnFromCpp(const char* func_name, const T& value) {
  v8::Isolate* isolate = args_.GetIsolate();
  v8::Local<v8::Value> js_value = Convert<T>::CppToJs(isolate, value);
  args_.GetReturnValue().Set(js_value);
  return true;
}

bool inline JsContext::CheckNumArgs(const char* func_name,
                                    size_t expected_args) const {
  const size_t num_args = args_.Length();
  if (num_args != expected_args) {
    LOG(ERROR) << func_name << " expects " << expected_args << " args, but got "
               << num_args;
    return false;
  }
  return true;
}

}  // namespace javascript
}  // namespace script
}  // namespace lull

#endif  // LULLABY_MODULES_JAVASCRIPT_CONVERT_H_
