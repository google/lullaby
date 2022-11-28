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

#include "redux/tools/common/assimp_utils.h"

#include <iostream>
#include <sstream>

#include "assimp/DefaultIOSystem.h"
#include "assimp/DefaultLogger.hpp"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "redux/modules/base/logging.h"

namespace redux::tool {

// The default logger always spews everything. We only care when something goes
// wrong. BatchLogger collects all the spew so that it can be reported if
// something goes wrong.
class BatchLogger : public Assimp::Logger {
 public:
  BatchLogger() : Assimp::Logger() {}
  ~BatchLogger() override {}

  bool attachStream(Assimp::LogStream* pStream,
                    unsigned int severity) override {
    return false;
  }

  bool detachStream(Assimp::LogStream* pStream,
                    unsigned int severity) override {
    return false;
  }

  void ReportWarningsAndErrors() {
    const auto warning_count = LogCountWithSeverity(Assimp::Logger::Warn);
    const auto error_count = LogCountWithSeverity(Assimp::Logger::Err);

    std::ostringstream report_prefix;
    report_prefix << "Import failed";

    if (warning_count && error_count) {
      report_prefix << " with " << warning_count << " warnings and "
                    << error_count << " errors:";
    } else if (error_count) {
      report_prefix << " with " << error_count << " errors:";
    } else if (warning_count) {
      report_prefix << " with " << warning_count << " warnings:";
    } else {
      report_prefix << ".";
    }

    LOG(ERROR) << report_prefix.str();
    for (auto& entry : entries_) {
      if (entry.first >= Assimp::Logger::Warn) {
        DumpLogItem(entry.first, entry.second);
      }
    }
  }

 protected:
  explicit BatchLogger(LogSeverity severity) : Assimp::Logger(severity) {}

  void OnDebug(const char* message) override {
    entries_.emplace_back(Logger::Debugging, message);
  }
  void OnInfo(const char* message) override {
    entries_.emplace_back(Logger::Info, message);
  }
  void OnWarn(const char* message) override {
    entries_.emplace_back(Logger::Warn, message);
  }
  void OnError(const char* message) override {
    entries_.emplace_back(Logger::Err, message);
  }
  void OnVerboseDebug(const char* message) override {
    entries_.emplace_back(Logger::Debugging, message);
  }

  size_t LogCountWithSeverity(Assimp::Logger::ErrorSeverity severity) {
    return std::count_if(
        entries_.begin(), entries_.end(),
        [severity](const LogEntry& entry) { return entry.first == severity; });
  }

  void DumpLogItem(Assimp::Logger::ErrorSeverity severity,
                   const std::string& error) {
    switch (severity) {
      case Assimp::Logger::Debugging: {
        break;
      }
      case Assimp::Logger::Info: {
        break;
      }
      case Assimp::Logger::Warn: {
        LOG(WARNING) << error;
        break;
      }
      case Assimp::Logger::Err: {
        LOG(ERROR) << error;
        break;
      }
    }
  }

  using LogEntry = std::pair<Assimp::Logger::ErrorSeverity, std::string>;
  std::vector<LogEntry> entries_;
};

// An IO System that keeps track of all the files that have been requested.
class TrackedIOSystem : public Assimp::DefaultIOSystem {
 public:
  TrackedIOSystem() : Assimp::DefaultIOSystem() {}
  ::Assimp::IOStream* Open(const char* filename, const char* mode) override {
    if (!std::any_of(opened_files_.begin(), opened_files_.end(),
                     [filename, this](const std::string& opened_file) {
                       return ComparePaths(filename, opened_file.c_str());
                     })) {
      opened_files_.emplace_back(filename);
    }
    return Assimp::DefaultIOSystem::Open(filename, mode);
  }

  const std::vector<std::string>& GetOpenedFiles() { return opened_files_; }

 private:
  std::vector<std::string> opened_files_;
};

bool AssimpBaseImporter::LoadScene(const std::string& filename,
                                   const Options& options) {
  importer_.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE,
                             options.smoothing_angle);
  importer_.SetPropertyFloat(AI_CONFIG_PP_LBW_MAX_WEIGHTS,
                             options.max_bone_weights);
  importer_.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY,
                             options.scale_multiplier);

  int flags = 0;
  flags |= aiProcess_CalcTangentSpace;
  flags |= aiProcess_JoinIdenticalVertices;
  flags |= aiProcess_Triangulate;
  flags |= aiProcess_GenSmoothNormals;
  flags |= aiProcess_ImproveCacheLocality;
  flags |= aiProcess_RemoveRedundantMaterials;
  flags |= aiProcess_LimitBoneWeights;
  if (options.optimize_mesh) {
    flags |= aiProcess_OptimizeMeshes;
    flags |= aiProcess_OptimizeGraph;
  }
  if (options.flip_texture_coordinates) {
    flags |= aiProcess_FlipUVs;
  }
  if (options.flatten_hierarchy_and_transform_vertices_to_root_space) {
    flags |= aiProcess_PreTransformVertices;
  }
  if (options.fix_infacing_normals) {
    flags |= aiProcess_FixInfacingNormals;
  }

  // Assimp expects a pointer to a C++-allocated logger (which it then owns).
  if (!options.require_thread_safe) {
    Assimp::DefaultLogger::set(new BatchLogger());
  }
  TrackedIOSystem io_system;
  importer_.SetIOHandler(&io_system);
  scene_ = importer_.ReadFile(filename.c_str(), flags);
  imported_files_ = io_system.GetOpenedFiles();
  importer_.SetIOHandler(nullptr);

  if (scene_ == nullptr) {
    if (options.require_thread_safe) {
      LOG(ERROR) << "Unable to load scene: " << filename;
      LOG(ERROR) << importer_.GetErrorString();
    } else {
      BatchLogger* logger =
          static_cast<BatchLogger*>(Assimp::DefaultLogger::get());
      logger->error("Unable to load scene: " + filename);
      logger->error(importer_.GetErrorString());
      logger->ReportWarningsAndErrors();
    }
    return false;
  }

  PopulateHierarchyRecursive(scene_->mRootNode);
  return true;
}

void AssimpBaseImporter::AddNodeToHierarchy(const aiNode* node) {
  while (node != nullptr && node != scene_->mRootNode) {
    // Nodes with $ symbols seem to be generated as part of the assimp importer
    // itself and are not part of the original asset.
    if (!strstr(node->mName.C_Str(), "$")) {
      valid_nodes_.emplace(node);
    }
    node = node->mParent;
  }
}

void AssimpBaseImporter::PopulateHierarchyRecursive(const aiNode* node) {
  for (int i = 0; i < node->mNumMeshes; ++i) {
    AddNodeToHierarchy(node);

    const int mesh_index = node->mMeshes[i];
    const aiMesh* mesh = scene_->mMeshes[mesh_index];
    for (int j = 0; j < mesh->mNumBones; ++j) {
      const aiBone* bone = mesh->mBones[j];
      const aiNode* bone_node = scene_->mRootNode->FindNode(bone->mName);
      AddNodeToHierarchy(bone_node);
    }
  }
  for (int i = 0; i < node->mNumChildren; ++i) {
    PopulateHierarchyRecursive(node->mChildren[i]);
  }
}

void AssimpBaseImporter::ReadSkeletonRecursive(
    const BoneFn& fn, const aiNode* node, const aiNode* parent,
    const aiMatrix4x4& base_transform) {
  const aiMatrix4x4 transform = base_transform * node->mTransformation;
  auto iter = valid_nodes_.find(node);
  const bool is_bone_node = iter != valid_nodes_.end();
  if (is_bone_node) {
    fn(node, parent, transform);
  }
  for (int i = 0; i < node->mNumChildren; ++i) {
    // assimp may insert nodes between the original nodes in the data. To ensure
    // that bone parents are correct, only use |node| if it was, in fact, a
    // bone, otherwise it might result in many orphaned bones.
    ReadSkeletonRecursive(fn, node->mChildren[i], is_bone_node ? node : parent,
                          transform);
  }
}

void AssimpBaseImporter::ReadMeshRecursive(const MeshFn& fn,
                                           const aiNode* node) {
  for (int i = 0; i < node->mNumMeshes; ++i) {
    const int mesh_index = node->mMeshes[i];
    const aiMesh* mesh = scene_->mMeshes[mesh_index];

    if (mesh->mNumAnimMeshes != 0 || mesh->mAnimMeshes != nullptr) {
      LOG(ERROR) << "Animated meshes are unsupported.";
      continue;
    } else if (!mesh->HasPositions()) {
      LOG(ERROR) << "Mesh does not have positions.";
      continue;
    }

    const aiMaterial* material = scene_->mMaterials[mesh->mMaterialIndex];
    fn(mesh, node, material);
  }
  for (int i = 0; i < node->mNumChildren; ++i) {
    ReadMeshRecursive(fn, node->mChildren[i]);
  }
}

void AssimpBaseImporter::ForEachBone(const BoneFn& fn) {
  if (scene_) {
    ReadSkeletonRecursive(fn, scene_->mRootNode, nullptr, aiMatrix4x4());
  }
}

void AssimpBaseImporter::ForEachMaterial(const MaterialFn& fn) {
  if (scene_) {
    for (int i = 0; i < scene_->mNumMaterials; ++i) {
      fn(scene_->mMaterials[i]);
    }
  }
}

void AssimpBaseImporter::ForEachMesh(const MeshFn& fn) {
  if (scene_) {
    ReadMeshRecursive(fn, scene_->mRootNode);
  }
}

void AssimpBaseImporter::ForEachOpenedFile(const FileOpenedFn& fn) {
  for (const std::string& file : imported_files_) {
    fn(file);
  }
}

const aiScene* AssimpBaseImporter::GetScene() const { return scene_; }

}  // namespace redux::tool
