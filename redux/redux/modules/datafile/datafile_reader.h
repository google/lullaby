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

#ifndef REDUX_MODULES_DATAFILE_DATAFILE_H_
#define REDUX_MODULES_DATAFILE_DATAFILE_H_

#include <string_view>
#include <type_traits>

#include "redux/engines/script/redux/script_env.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/base/serialize.h"
#include "redux/modules/base/serialize_traits.h"
#include "redux/modules/datafile/datafile_parser.h"
#include "redux/modules/var/var.h"

namespace redux {

// Parses the datafile text and evaluates it directly into the provided runtime
// object of type T. The provided ScriptEnv is used to evaluate any expressions
// encoded in the datafile.
//
// T must provide a Serialize function (see base/archive.h for details).
template <typename T>
T ReadDatafile(std::string_view text, ScriptEnv* env);

namespace detail {

// Internal class that deals with the parsing and evaluation of the data.
class DatafileReader : public DatafileParserCallbacks {
 public:
  explicit DatafileReader(ScriptEnv* env) : env_(env) {}

  template <typename T>
  void Read(std::string_view text, T& obj) {
    // Push 'obj' as the root object for the traversal stack.
    root_.reset(new ElementT<T>(&obj));
    ParseDatafile(text, this);
    CHECK(stack_.empty());
    root_.reset();
  }

  // DatafileParserCallbacks.
  void Key(std::string_view value) override;
  void BeginObject() override;
  void EndObject() override;
  void BeginArray() override;
  void EndArray() override;
  void Null() override;
  void Boolean(bool value) override;
  void Number(double value) override;
  void String(std::string_view value) override;
  void Expression(std::string_view value) override;
  void ParseError(std::string_view context, std::string_view message) override;

 private:
  // When it comes to serializing datafiles, there's basically three types of
  // elements we're dealing with:
  //   Values: The "primitive" types such as ints, floats, bools, strings,
  //           expressions, etc.
  //   Objects: Maps of key/value pairs, where the key is a HashValue and a
  //            value is another element. As a text file, these are enclosed
  //            in braces (i.e. { and }), and in C++, these are structs or
  //            classes.
  //   Arrays: A sequence of elements. As a text file, these are enclosed in
  //           brackets (i.e. [ and ]), and in C++, these are vectors. You can
  //           have arrays of Values, Objects, or Arrays. Arrays have uniform
  //           type; you can't have an Array of both Values and Objects.

  struct ObjectDetector {
    template <typename T>
    void operator()(T& value, HashValue key) {}
  };

  template <typename U>
  static constexpr bool IsObject = IsSerializableT<U, ObjectDetector>::value;

  template <typename U>
  static constexpr bool IsArray = IsVectorT<U>::value;

  template <typename U>
  static constexpr bool IsValue = !IsObject<U> && !IsArray<U>;

  class Element;
  using ElementPtr = std::unique_ptr<Element>;

  // Responsible for Serializing data into an Element. In this case, Elements
  // are limited to either Objects or Arrays.
  class Element {
   public:
    virtual ~Element() = default;

    // Marks the start of a new Object or Array within the current element.
    // Returns the pointer to the new "Element" object.
    virtual ElementPtr Begin(HashValue key) = 0;

    // Attempts to set the value of the given field within the current element.
    virtual void SetValue(HashValue key, const Var& var) = 0;
  };

  template <typename T>
  class ElementT : public Element {
   public:
    explicit ElementT(T* obj) : obj_(obj) {}

    ElementPtr Begin(HashValue key) override {
      if constexpr (IsArray<T>) {
        // We can only call Begin in an array if its an array of objects.
        if constexpr (!IsValue<typename T::value_type>) {
          return BeginArrayElement(key);
        }
      } else if constexpr (IsObject<T>) {
        return BeginObjectElement(key);
      }
      return nullptr;
    }

    // Assigns a value to the element.
    void SetValue(HashValue key, const Var& var) override {
      if constexpr (IsArray<T>) {
        // We can only call SetValue in an array if its an array of values.
        if constexpr (IsValue<typename T::value_type>) {
          SetArrayValue(key, var);
        }
      } else if constexpr (IsObject<T>) {
        SetObjectValue(key, var);
      }
    }

   private:
    template <typename U>
    ElementPtr MakeElementPtr(U* obj) {
      return ElementPtr(new ElementT<U>(obj));
    }

    ElementPtr BeginArrayElement(HashValue key) {
      obj_->emplace_back();
      return MakeElementPtr(&obj_->back());
    }

    void SetArrayValue(HashValue key, const Var& var) {
      obj_->emplace_back();
      FromVar(var, &obj_->back());
    }

    ElementPtr BeginObjectElement(HashValue key) {
      ElementPtr result;
      obj_->Serialize([&](auto& value, HashValue match) {
        if (key == match) {
          result = MakeElementPtr(&value);
        }
      });
      return result;
    }

    void SetObjectValue(HashValue key, const Var& var) {
      obj_->Serialize([&](auto& value, HashValue match) {
        using U = std::decay_t<decltype(value)>;
        if constexpr (IsValue<U>) {
          if (key == match) {
            FromVar(var, &value);
          }
        }
      });
    }

    T* obj_ = nullptr;
  };

  // Basically calls SetValue on the back of the stack with the current key.
  void SetValue(const Var& var);

  HashValue key_;
  ElementPtr root_;
  std::vector<ElementPtr> stack_;
  ScriptEnv* env_ = nullptr;
};

}  // namespace detail

template <typename T>
T ReadDatafile(std::string_view text, ScriptEnv* env) {
  detail::DatafileReader reader(env);

  T obj;
  reader.Read(text, obj);
  return obj;
}

}  // namespace redux

#endif  // REDUX_MODULES_DATAFILE_DATAFILE_H_
