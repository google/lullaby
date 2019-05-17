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

#ifndef LULLABY_MODULES_CONFIG_CONFIG_H_
#define LULLABY_MODULES_CONFIG_CONFIG_H_

#include <mutex>
#include <unordered_map>
#include "lullaby/generated/config_def_generated.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/util/flatbuffer_reader.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/string_view.h"
#include "lullaby/util/typeid.h"
#include "lullaby/util/variant.h"

namespace lull {

// A thread-safe store for configuration data.
//
// Configuration data can be key-value pairs or data objects.
//
// Generally, a single instance of this class will be made available in the
// registry to allow for app-wide configuration settings.
class Config {
 public:
  // Loads data from the ConfigDef specified by the |filename|.  Uses the
  // |registry| to perform the file load operation via the AssetLoader.
  void LoadConfig(Registry* registry, string_view filename);

  // Loads an object of type T from a flatbuffer specified by the |filename|.
  // Uses the |registry| to perform the file load operation via the AssetLoader.
  template <typename T>
  void LoadObject(Registry* registry, string_view filename);

  // Associates the |value| with the |key|.
  template <typename T>
  void Set(HashValue key, const T& value);

  // Associates the |value| with the |key|.
  void Set(HashValue key, Variant value);

  // Copies all the key-value pairs in |values| into this Config.
  void Set(const VariantMap& values);

  // Returns the value associated with the |key| if it is of type |T|.  If no
  // such value exists, returns the specified |default_value| instead.
  template <typename T>
  T Get(HashValue key, const T& default_value) const;

  // Removes the value associated with the |key|.
  void Remove(HashValue key);

  // Associates an object that cannot be stored in a Variant.
  template <typename T>
  void SetObject(const T& obj);

  // Returns the value associated with the |key| if it is of type |T|.  If no
  // such value exists, returns a default-constructed instance of that type.
  template <typename T>
  const T& GetObject() const;

  // Removes the object of type T that is stored internally.
  template <typename T>
  void RemoveObject();

 private:
  std::string LoadFile(Registry* registry, string_view filename);

  using ObjectMap = std::unordered_map<TypeId, std::shared_ptr<void>>;

  mutable std::mutex mutex_;
  mutable ObjectMap objects_;
  VariantMap values_;
};

template <typename T>
inline void Config::LoadObject(Registry* registry, string_view filename) {
  const std::string data = LoadFile(registry, filename);
  if (data.empty()) {
    return;
  }

  T obj;
  if (ReadRootFlatbuffer(&obj, reinterpret_cast<const uint8_t*>(data.data()),
                         data.size())) {
    SetObject(std::move(obj));
  }
}

template <typename T>
inline void Config::Set(HashValue key, const T& value) {
  Set(key, Variant(value));
}

inline void Config::Set(HashValue key, Variant value) {
  std::unique_lock<std::mutex> lock(mutex_);
  values_[key] = std::move(value);
}

inline void Config::Set(const VariantMap& values) {
  for (const auto& iter : values) {
    Set(iter.first, iter.second);
  }
}

inline void Config::Remove(HashValue key) {
  std::unique_lock<std::mutex> lock(mutex_);
  values_.erase(key);
}

template <typename T>
inline T Config::Get(HashValue key, const T& default_value) const {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = values_.find(key);
  if (iter == values_.end()) {
    return default_value;
  }
  const T* ptr = iter->second.Get<T>();
  return ptr ? *ptr : default_value;
}

template <typename T>
inline void Config::SetObject(const T& obj) {
  std::unique_lock<std::mutex> lock(mutex_);
  objects_[GetTypeId<T>()] = std::make_shared<T>(obj);
}

template <typename T>
inline const T& Config::GetObject() const {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = objects_.find(GetTypeId<T>());
  if (iter == objects_.end()) {
    auto ptr = std::make_shared<T>();
    objects_[GetTypeId<T>()] = ptr;
    return *ptr;
  } else {
    return *static_cast<const T*>(iter->second.get());
  }
}

template <typename T>
inline void Config::RemoveObject() {
  std::unique_lock<std::mutex> lock(mutex_);
  objects_.erase(GetTypeId<T>());
}

inline void LoadConfigFromFile(Registry* registry, Config* config,
                               string_view filename) {
  if (config) {
    config->LoadConfig(registry, filename);
  }
}

inline void SetConfigFromVariantMap(Config* config, const VariantMap* values) {
  if (config && values) {
    config->Set(*values);
  }
}

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::Config);

#endif  // LULLABY_MODULES_CONFIG_CONFIG_H_
