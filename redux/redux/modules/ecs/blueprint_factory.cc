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

#include "redux/modules/ecs/blueprint_factory.h"

#include <string>
#include <utility>
#include <vector>

#include "redux/engines/script/redux/script_ast_builder.h"
#include "redux/engines/script/redux/script_parser.h"
#include "redux/modules/base/asset_loader.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/datafile/datafile_parser.h"
#include "redux/modules/var/var_array.h"
#include "redux/modules/var/var_table.h"

namespace redux {

static constexpr HashValue kUriHash = ConstHash("uri");
static constexpr HashValue kTypeHash = ConstHash("$type");
static constexpr HashValue kImportHash = ConstHash("#import");

class BlueprintParser : public DatafileParserCallbacks {
 public:
  BlueprintParser() = default;

  void Key(std::string_view value) override {
    CHECK_EQ(key_.get(), 0);
    key_ = Hash(value);
  }

  void BeginObject() override {
    if (!started_) {
      started_ = true;
      return;
    }
    Add(VarTable());
  }

  void EndObject() override {
    if (!stack_.empty()) {
      stack_.pop_back();
    }
  }

  void BeginArray() override {
    if (!started_) {
      ParseError("", "Document must be an object, not an array.");
      started_ = true;
      return;
    }
    Add(VarArray());
  }

  void EndArray() override {
    if (!stack_.empty()) {
      stack_.pop_back();
    }
  }

  void Null() override { Add(Var()); }

  void Boolean(bool value) override { Add(value); }

  void Number(double value) override { Add(value); }

  void String(std::string_view value) override { Add(std::string(value)); }

  void Expression(std::string_view value) override {
    ScriptAstBuilder builder;
    ParseScript(value, &builder);
    if (const AstNode* root = builder.GetRoot()) {
      Add(ScriptValue(*root));
    } else {
      LOG(ERROR) << "There were errors parsing the expression: " << value;
    }
  }

  void ParseError(std::string_view context, std::string_view message) override {
    LOG(ERROR) << message;
    error_ = message;
  }

  VarArray Release() { return std::move(root_); }

  bool Ok() { return root_.Count() > 0 && stack_.empty(); }

 private:
  template <typename T>
  void Add(T value) {
    Add(Var(value));
  }

  void Add(Var var) {
    if (!started_) {
      ParseError("", "Document must be an object.");
      return;
    } else if (!error_.empty()) {
      return;
    }

    if (stack_.empty()) {
      CHECK_NE(key_.get(), 0);
      if (var.Is<VarTable>()) {
        var[kTypeHash] = key_;
        root_.PushBack(std::move(var));
      } else if (var.Is<std::string>()) {
        CHECK(key_ == kImportHash);
        VarTable import;
        import.Insert(kTypeHash, key_);
        import.Insert(kUriHash, std::move(var));
        root_.PushBack(Var(std::move(import)));
      } else {
        CHECK(false);
      }
      Var& back = root_.At(root_.Count() - 1);
      stack_.emplace_back(&back);
    } else {
      const bool is_table = var.Is<VarTable>();
      const bool is_array = var.Is<VarArray>();

      Var& top = *stack_.back();
      if (top.Is<VarArray>()) {
        CHECK_EQ(key_.get(), 0);
        top.Get<VarArray>()->PushBack(std::move(var));

        if (is_array || is_table) {
          Var& back = top[top.Count() - 1];
          stack_.emplace_back(&back);
        }
      } else {
        CHECK_NE(key_.get(), 0);
        top.Get<VarTable>()->Insert(key_, std::move(var));

        if (is_array || is_table) {
          Var& back = top[key_];
          stack_.emplace_back(&back);
        }
      }
    }
    key_ = HashValue(0);
  }

  VarArray root_;
  HashValue key_;
  std::vector<Var*> stack_;
  bool started_ = false;
  std::string_view error_;
};

static std::string_view GetStringContents(
    const AssetLoader::StatusOrData& asset) {
  CHECK(asset.ok());
  const char* str = reinterpret_cast<const char*>(asset->GetBytes());
  const size_t len = asset->GetNumBytes();
  return std::string_view(str, len);
}

BlueprintFactory::BlueprintFactory(Registry* registry) : registry_(registry) {}

BlueprintPtr BlueprintFactory::LoadBlueprint(std::string_view uri) {
  const HashValue key = Hash(uri);
  return blueprints_.Create(key, [=]() {
    auto asset_loader = registry_->Get<AssetLoader>();
    CHECK(asset_loader);
    AssetLoader::StatusOrData asset = asset_loader->LoadNow(uri);
    return ParseBlueprintDatafile(std::string(uri), GetStringContents(asset));
  });
}

BlueprintPtr BlueprintFactory::ReadBlueprint(std::string_view text) {
  return ParseBlueprintDatafile("", text);
}

static void ImportAt(VarArray* dst, const BlueprintPtr& src, size_t index) {
  dst->Erase(index);
  for (size_t i = 0; i < src->GetNumComponents(); ++i) {
    dst->Insert(index++, src->GetComponent(i));
  }
}

BlueprintPtr BlueprintFactory::ParseBlueprintDatafile(std::string name,
                                                      std::string_view text) {
  BlueprintParser parser;
  ParseDatafile(text, &parser);
  if (!parser.Ok()) {
    LOG(ERROR) << text;
    return nullptr;
  }

  VarArray components = parser.Release();

  // Look for any "imported" blueprints and merge them into the final blueprint.
  for (size_t i = 0; i < components.Count();) {
    const Var& component = components.At(i);

    if (component[kTypeHash].ValueOr(HashValue()) == kImportHash) {
      const std::string uri = component[kUriHash].ValueOr(std::string(""));
      const BlueprintPtr& imported_blueprint = LoadBlueprint(uri);
      CHECK(imported_blueprint);
      ImportAt(&components, imported_blueprint, i);
    } else {
      ++i;
    }
  }

  return std::make_shared<Blueprint>(name, std::move(components));
}

}  // namespace redux
