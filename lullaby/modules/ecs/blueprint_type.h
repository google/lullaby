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

#ifndef LULLABY_MODULES_ECS_BLUEPRINT_TYPE_H_
#define LULLABY_MODULES_ECS_BLUEPRINT_TYPE_H_

#include "lullaby/util/hash.h"
#include "lullaby/util/typeid.h"

namespace lull {

// The type of data stored in a blueprint.
//
// Ideally, we would just use a TypeId to provide information about the type
// of data stored in a blueprint.  However, legacy Systems use the hash of the
// string returned by the GetFullyQualifiedName function generated for a
// flatbuffer schema for identifying blueprints.
//
// As such, this class stores both the TypeId of the blueprint (if available)
// and the hash of the name of the schema.  Once all Systems have been updated
// to use Blueprints (instead of flatbuffers::Table* + HashValue combo), this
// class can be entirely replaced by using TypeIds directly.
class BlueprintType {
 public:
  BlueprintType() {}

  // Creates a BlueprintType for a given class generated from a schema.
  template <typename T>
  static BlueprintType Create();

  template <typename T>
  bool Is() const;

  // Creates BlueprintType using only legacy schema name information.
  static BlueprintType CreateFromSchemaNameHash(HashValue type);

  // Returns the legacy schema name hash.
  HashValue GetSchemaNameHash() const;

  // Compares the BlueprintType with another BlueprintType.
  bool operator==(const BlueprintType& rhs) const;
  bool operator!=(const BlueprintType& rhs) const;

 private:
  BlueprintType(TypeId type, HashValue name) : type_(type), name_(name) {}

  static HashValue GenerateSchemaNameHashFromTypeName(const char* name);

  TypeId type_ = 0;
  HashValue name_ = 0;
};

template <typename T>
BlueprintType BlueprintType::Create() {
  const TypeId type = GetTypeId<T>();
  const HashValue name = GenerateSchemaNameHashFromTypeName(GetTypeName<T>());
  return BlueprintType(type, name);
}

template <typename T>
bool BlueprintType::Is() const {
  if (type_ == 0) {
    return name_ == GenerateSchemaNameHashFromTypeName(GetTypeName<T>());
  } else {
    return type_ == GetTypeId<T>();
  }
}

}  // namespace lull

#endif  // LULLABY_MODULES_ECS_BLUEPRINT_TYPE_H_

