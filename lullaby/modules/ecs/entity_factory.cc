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

#include "lullaby/modules/ecs/entity_factory.h"

#include <memory>
#include <utility>

#include "lullaby/modules/ecs/blueprint_builder.h"
#include "lullaby/modules/ecs/blueprint_reader.h"
#include "lullaby/modules/ecs/blueprint_writer.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/util/filename.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/make_unique.h"

namespace lull {

const char* const EntityFactory::kLegacyFileIdentifier = "ENTS";

EntityFactory::EntityFactory(Registry* registry)
    : registry_(registry), entity_generator_(0) {
  FunctionBinder* binder = registry->Get<FunctionBinder>();
  if (binder) {
    void (EntityFactory::*initialize)() =
        &lull::EntityFactory::Initialize;
    Entity (EntityFactory::*create)() =
        &lull::EntityFactory::Create;
    Entity (EntityFactory::*create_from_name)(const std::string&) =
        &lull::EntityFactory::Create;
    binder->RegisterMethod("lull.EntityFactory.InitializeSystems",
                           initialize);
    binder->RegisterMethod("lull.EntityFactory.CreateEntity", create);
    binder->RegisterMethod("lull.EntityFactory.CreateEntityFromName",
                           create_from_name);
    binder->RegisterMethod("lull.EntityFactory.DestroyEntity",
                           &lull::EntityFactory::Destroy);
    binder->RegisterMethod("lull.EntityFactory.QueueForDestruction",
                           &lull::EntityFactory::QueueForDestruction);
    binder->RegisterMethod("lull.EntityFactory.DestroyQueuedEntities",
                           &lull::EntityFactory::DestroyQueuedEntities);
  }
}

EntityFactory::~EntityFactory() {
  FunctionBinder* binder = registry_->Get<FunctionBinder>();
  if (binder) {
    binder->UnregisterFunction("lull.EntityFactory.InitializeSystems");
    binder->UnregisterFunction("lull.EntityFactory.CreateEntity");
    binder->UnregisterFunction("lull.EntityFactory.CreateEntityFromName");
    binder->UnregisterFunction("lull.EntityFactory.DestroyEntity");
    binder->UnregisterFunction("lull.EntityFactory.QueueForDestruction");
    binder->UnregisterFunction("lull.EntityFactory.DestroyQueuedEntities");
  }
}

EntityFactory* EntityFactory::Create(Registry* registry) {
  return registry->Create<lull::EntityFactory>(registry);
}

void EntityFactory::Initialize() {
  if (systems_.empty()) {
    LOG(DFATAL) << "Call Initialize after creating Systems.";
  }

  InitializeSystems();
  registry_->CheckAllDependencies();
  InitializeBlueprintConverter();
}

void EntityFactory::InitializeBlueprintConverter() {
  FlatbufferConverter* converter = CreateFlatbufferConverter(
      detail::BlueprintBuilder::kBlueprintFileIdentifier);
  converter->load = [this](const void* data,
                           size_t len) -> Optional<BlueprintTree> {
    return BlueprintReader(&component_handlers_)
        .ReadFlatbuffer({reinterpret_cast<const uint8_t*>(data), len});
  };
  converter->finalize = [](FlatbufferWriter* writer,
                           Blueprint* blueprint) -> size_t {
    LOG(DFATAL) << "Not implemented.";
    return 0;
  };
}

void EntityFactory::RegisterDef(TypeId system_type,
                                Blueprint::DefType def_type) {
  type_map_[def_type] = system_type;
}

void EntityFactory::InitializeSystems() {
  for (auto& iter : systems_) {
    iter.second->Initialize();
  }
}

void EntityFactory::AddSystem(TypeId system_type, System* system) {
  if (!system) {
    return;
  }
  auto iter = systems_.find(system_type);
  if (iter == systems_.end()) {
    systems_.emplace(system_type, system);
  }
}

void EntityFactory::CreateTypeList(const char* const* names,
                                   FlatbufferConverter* converter) {
  converter->types.reserve(type_map_.size());
  while (*names) {
    const Blueprint::DefType type = Hash(*names);
    converter->types.emplace_back(type);
    ++names;
  }
}

size_t EntityFactory::PerformReverseTypeLookup(
    Blueprint::DefType name, const FlatbufferConverter* converter) const {
  for (size_t i = 0; i < converter->types.size(); ++i) {
    if (converter->types[i] == name) {
      return i;
    }
  }
  return 0;
}

Entity EntityFactory::Create() {
  Lock lock(mutex_);
  const Entity entity(++entity_generator_);
  CHECK_NE(entity, kNullEntity) << "Overflow on Entity generation.";
  entity_to_blueprint_map_[entity] = "";
  return entity;
}

Entity EntityFactory::Create(const std::string& name) {
  auto asset = GetBlueprintAsset(name);
  if (asset == nullptr) {
    LOG(ERROR) << "No such blueprint: " << name;
    return kNullEntity;
  }
  return CreateFromBlueprint(asset->GetData(), asset->GetSize(), name);
}

Entity EntityFactory::Create(Blueprint* blueprint) {
  Entity entity = Create();
  if (!CreateImpl(entity, blueprint)) {
    return kNullEntity;
  }

  entity_to_blueprint_map_[entity] = "";
  return entity;
}

Entity EntityFactory::Create(BlueprintTree* blueprint) {
  Entity entity = Create();
  return Create(entity, blueprint);
}

Entity EntityFactory::Create(Entity entity, const std::string& name) {
  auto asset = GetBlueprintAsset(name);
  if (!asset) {
    LOG(ERROR) << "No such blueprint: " << name;
    return kNullEntity;
  }

  if (!CreateImpl(entity, name, asset->GetData(), asset->GetSize())) {
    LOG(ERROR) << "Could not create from blueprint: " << name;
    return kNullEntity;
  }

  return entity;
}

Entity EntityFactory::Create(Entity entity, BlueprintTree* blueprint) {
  entity_to_blueprint_map_[entity] = "";
  if (!CreateImpl(entity, blueprint)) {
    return kNullEntity;
  }
  return entity;
}

Span<uint8_t> EntityFactory::Finalize(Blueprint* blueprint,
                                      string_view identifier) {
  Span<uint8_t> data;
  // This should properly return the schema if only one has been set, but fail
  // when multiple or no schemas have been set.
  FlatbufferConverter* converter = GetFlatbufferConverter(identifier);
  if (converter) {
    data = blueprint->Finalize(converter->finalize);
  } else {
    LOG(DFATAL) << "Unknown file identifier for finalizing blueprint: "
                << identifier;
  }
  return data;
}

Entity EntityFactory::CreateFromBlueprint(const void* blueprint, size_t len,
                                          const std::string& name) {
  const Entity entity = Create();
  if (!CreateImpl(entity, name, blueprint, len)) {
    return kNullEntity;
  }
  return entity;
}

bool EntityFactory::CreateFromBlueprint(Entity entity, const void* blueprint,
                                        size_t len, const std::string& name) {
  return CreateImpl(entity, name, blueprint, len);
}

bool EntityFactory::CreateImpl(Entity entity, const std::string& name,
                               const void* data, size_t len) {
  if (entity == kNullEntity) {
    LOG(DFATAL) << "Cannot create null entity: " << name;
    return false;
  }
  if (data == nullptr) {
    LOG(DFATAL) << "Cannot create entity from null data: " << name;
    return false;
  }

  auto blueprint = CreateBlueprintFromData(name, data, len);
  if (!blueprint) {
    return false;
  }

  entity_to_blueprint_map_[entity] = name;

  return CreateImpl(entity, blueprint.get());
}

bool EntityFactory::CreateImpl(Entity entity, BlueprintTree* blueprint) {
  return CreateImpl(entity, blueprint, blueprint->Children());
}

bool EntityFactory::CreateImpl(Entity entity, Blueprint* blueprint,
                               std::list<BlueprintTree>* children) {
  if (entity == kNullEntity) {
    LOG(DFATAL) << "Cannot create null entity";
    return false;
  }
  if (blueprint == nullptr) {
    LOG(DFATAL) << "Cannot create entity from null blueprint";
    return false;
  }

  blueprint->ForEachComponent([this, entity](const Blueprint& blueprint) {
    System* system = GetSystem(blueprint.GetLegacyDefType());
    if (system) {
      system->CreateComponent(entity, blueprint);
    } else {
      LOG(DFATAL) << "Unknown system " << blueprint.GetLegacyDefType()
                  << " when creating entity " << entity
                  << " from blueprint: " << entity_to_blueprint_map_[entity];
    }
  });
  // Construct children after parent creation, but before parent post-creation.
  // This allows the parent to discover/manipulate children during
  // PostCreateComponent.
  if (children) {
    for (auto& child_blueprint : *children) {
      create_child_fn_(entity, &child_blueprint);
    }
  }
  // Now invoke PostCreateComponent on parent.
  blueprint->ForEachComponent([this, entity](const Blueprint& blueprint) {
    System* system = GetSystem(blueprint.GetLegacyDefType());
    if (system) {
      system->PostCreateComponent(entity, blueprint);
    }
  });

  return true;
}

std::shared_ptr<SimpleAsset> EntityFactory::GetBlueprintAsset(
    const std::string& name) {
  std::string filename = name;
  if (!EndsWith(filename, ".json")) {
    filename += ".bin";
  }

  const HashValue key = Hash(filename.c_str());

  auto asset = blueprints_.Create(key, [&]() {
    AssetLoader* asset_loader = registry_->Get<AssetLoader>();
    return asset_loader->LoadNow<SimpleAsset>(filename);
  });

  if (asset->GetSize() == 0) {
    LOG(ERROR) << "Could not load entity blueprint: " << name;
    return nullptr;
  }
  return asset;
}

Optional<BlueprintTree> EntityFactory::CreateBlueprint(
    const std::string& name) {
  auto asset = GetBlueprintAsset(name);
  return CreateBlueprintFromAsset(name, asset.get());
}

Optional<BlueprintTree> EntityFactory::CreateBlueprintFromAsset(
    const std::string& name, const SimpleAsset* asset) {
  if (asset == nullptr) {
    LOG(ERROR) << "No such blueprint: " << name;
    return NullOpt;
  }
  return CreateBlueprintFromData(name, asset->GetData(), asset->GetSize());
}

Optional<BlueprintTree> EntityFactory::CreateBlueprintFromData(
    const std::string& name, const void* data, size_t size) {
  if (data == nullptr) {
    LOG(DFATAL) << "Cannot create entity from null data: " << name;
    return NullOpt;
  }

  const string_view identifier(
      flatbuffers::GetBufferIdentifier(data),
      flatbuffers::FlatBufferBuilder::kFileIdentifierLength);
  FlatbufferConverter* const converter = GetFlatbufferConverter(identifier);
  if (!converter) {
    if (converters_.empty()) {
      // Creating an entity before entity_factory was initialized.
      LOG(ERROR) << "Unable to convert raw data to blueprint.  Call "
                    "::Initialize with arguments to specify how to perform "
                    "this conversion.";
    } else {
      // Created an entity after initialization, but with a flatbuffer bin using
      // an unregistered schema.
      LOG(DFATAL) << "Unknown file identifier for entity: " << name
                  << ".  Identifier was: " << identifier;
    }
    return NullOpt;
  }
  Optional<BlueprintTree> result = converter->load(data, size);
  if (!result) {
      LOG(WARNING) << "Entity Blueprint conversion failed: " << name;
  }
  return result;
}

flatbuffers::DetachedBuffer EntityFactory::Finalize(
    BlueprintTree* blueprint_tree) {
  BlueprintWriter writer(&component_handlers_);
  return writer.WriteBlueprintTree(blueprint_tree);
}

void EntityFactory::ForgetCachedBlueprint(const std::string& name) {
  std::string filename = name;
  if (!EndsWith(filename, ".json")) {
    filename += ".bin";
  }
  blueprints_.Erase(Hash(filename));
}

void EntityFactory::Destroy(Entity entity) {
  if (entity == kNullEntity) {
    return;
  }

  entity_to_blueprint_map_.erase(entity);
  for (auto& iter : systems_) {
    iter.second->Destroy(entity);
  }
}

void EntityFactory::QueueForDestruction(Entity entity) {
  if (entity == kNullEntity) {
    return;
  }
  Lock lock(mutex_);
  pending_destroy_.push(entity);
}

void EntityFactory::DestroyQueuedEntities() {
  // Swap the queue of entities to be destroyed so that it can safely be
  // modified in QueueForDestruction from another thread.
  std::queue<Entity> pending_destroy;
  {
    Lock lock(mutex_);
    using std::swap;
    std::swap(pending_destroy_, pending_destroy);
  }

  while (!pending_destroy.empty()) {
    Destroy(pending_destroy.front());
    pending_destroy.pop();
  }
}

size_t EntityFactory::GetFlatbufferConverterCount() {
  return converters_.size();
}

EntityFactory::FlatbufferConverter* EntityFactory::CreateFlatbufferConverter(
    string_view identifier) {
  // This needs to live in the cc file, since including make_unique in the
  // header file causes ambiguous function errors with gtl::MakeUnique in some
  // Lullaby apps.
  converters_.emplace_back(MakeUnique<FlatbufferConverter>(identifier));
  return converters_.back().get();
}

EntityFactory::FlatbufferConverter* EntityFactory::GetFlatbufferConverter(
    string_view identifier) {
  for (auto& schema : converters_) {
    if (identifier == schema->identifier) {
      return schema.get();
    }
  }
  LOG(DFATAL) << "Unknown file identifier: " << identifier
              << ".  Please make sure you've called "
                 "EntityFactory::RegisterFlatbufferConverter with every "
                 "schema you are using.";
  return nullptr;
}

System* EntityFactory::GetSystem(const Blueprint::DefType def_type) {
  // Don't pollute the type and systems maps with null values.
  const auto type_id = type_map_.find(def_type);
  if (type_id == type_map_.end()) {
    return nullptr;
  }
  const auto system = systems_.find(type_id->second);
  if (system == systems_.end()) {
    return nullptr;
  }
  return system->second;
}

const EntityFactory::BlueprintMap& EntityFactory::GetEntityToBlueprintMap()
    const {
  return entity_to_blueprint_map_;
}

}  // namespace lull
