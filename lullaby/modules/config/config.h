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

#ifndef LULLABY_MODULES_CONFIG_CONFIG_H_
#define LULLABY_MODULES_CONFIG_CONFIG_H_

#include <mutex>
#include <unordered_map>
#include "lullaby/generated/config_def_generated.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/string_view.h"
#include "lullaby/util/typeid.h"
#include "lullaby/util/variant.h"

namespace lull {

// A thread-safe store for key-value pairs.
//
// Generally, a single instance of this class will be made available in the
// registry to allow for app-wide configuration settings.
class Config {
 public:
  // Associates the |value| with the |key|.
  template <typename T>
  void Set(HashValue key, const T& value);

  // Associates the |value| with the |key|.
  void Set(HashValue key, Variant value);

  // Returns the value associated with the |key| if it is of type |T|.  If no
  // such value exists, returns the specified |default_value| instead.
  template <typename T>
  T Get(HashValue key, const T& default_value) const;

  // Removes the value associated with the |key|.
  void Remove(HashValue key);

 private:
  using Database = std::unordered_map<HashValue, Variant>;
  mutable std::mutex mutex_;
  Database values_;
};

template <typename T>
void Config::Set(HashValue key, const T& value) {
  Set(key, Variant(value));
}

inline void Config::Set(HashValue key, Variant value) {
  std::unique_lock<std::mutex> lock(mutex_);
  values_[key] = std::move(value);
}

inline void Config::Remove(HashValue key) {
  std::unique_lock<std::mutex> lock(mutex_);
  values_.erase(key);
}

template <typename T>
T Config::Get(HashValue key, const T& default_value) const {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = values_.find(key);
  if (iter == values_.end()) {
    return default_value;
  }
  const T* ptr = iter->second.Get<T>();
  return ptr ? *ptr : default_value;
}

void LoadConfigFromFile(Registry* registry, Config* config,
                        string_view filename);
void SetConfigFromFlatbuffer(Config* config, const ConfigDef* config_def);
void SetConfigFromVariantMap(Config* config, const VariantMap* variant_map);

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::Config);

#endif  // LULLABY_MODULES_CONFIG_CONFIG_H_
