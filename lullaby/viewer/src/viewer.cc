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

#include "lullaby/viewer/src/viewer.h"

#include "ion/base/logging.h"
#include "dear_imgui/imgui.h"
#include "fplbase/utilities.h"
#include "lullaby/modules/animation_channels/render_channels.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/modules/script/script_engine.h"
#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/audio/audio_system.h"
#include "lullaby/systems/collision/collision_system.h"
#include "lullaby/systems/datastore/datastore_system.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/light/light_system.h"
#include "lullaby/systems/model_asset/model_asset_system.h"
#include "lullaby/systems/name/name_system.h"
#include "lullaby/systems/physics/physics_system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/rig/rig_system.h"
#include "lullaby/systems/script/script_system.h"
#include "lullaby/systems/stategraph/stategraph_system.h"
#include "lullaby/systems/text/text_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/filename.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/viewer/entity_generated.h"
#include "lullaby/viewer/src/builders/build_blueprint.h"
#include "lullaby/viewer/src/file_manager.h"
#include "lullaby/viewer/src/widgets/build_blueprint_popup.h"
#include "lullaby/viewer/src/widgets/build_model_popup.h"
#include "lullaby/viewer/src/widgets/build_shader_popup.h"
#include "lullaby/viewer/src/widgets/console.h"
#include "lullaby/viewer/src/widgets/entity_editor.h"
#include "lullaby/viewer/src/widgets/file_dialog.h"
#include "lullaby/viewer/src/widgets/preview_window.h"

namespace lull {
namespace tool {

static const size_t kPreviewWidth = 1280 * 0.7;
static const size_t kPreviewHeight = 720 * 0.7;

void Viewer::CreateEntity(const std::string& path) {
  const std::string name = RemoveExtensionFromFilename(path);
  registry_->Get<EntityFactory>()->Create(name.c_str());
}

void Viewer::ImportDirectory(const std::string& path) {
  registry_->Get<FileManager>()->ImportDirectory(path);
}

void Viewer::OnInitialize() {
  //ion::base::SetBreakHandler(nullptr);

  registry_ = std::make_shared<Registry>();
  dispatcher_ = new QueuedDispatcher();
  DispatcherSystem::EnableQueuedDispatch();
  registry_->Create<FunctionBinder>(registry_.get());
  registry_->Create<ScriptEngine>(registry_.get());
  registry_->Register(std::unique_ptr<Dispatcher>(dispatcher_));
  registry_->Create<AssetLoader>(fplbase::LoadFile);
  registry_->Create<InputManager>();
  registry_->Create<EntityFactory>(registry_.get());
  registry_->Create<FileManager>(registry_.get());

  auto* entity_factory = registry_->Get<lull::EntityFactory>();
  entity_factory->CreateSystem<lull::AnimationSystem>();
  // entity_factory->CreateSystem<lull::AudioSystem>();
  entity_factory->CreateSystem<lull::CollisionSystem>();
  entity_factory->CreateSystem<lull::DatastoreSystem>();
  entity_factory->CreateSystem<lull::DispatcherSystem>();
  entity_factory->CreateSystem<lull::LightSystem>();
  entity_factory->CreateSystem<lull::ModelAssetSystem>();
  entity_factory->CreateSystem<lull::NameSystem>();
  entity_factory->CreateSystem<lull::PhysicsSystem>();
  entity_factory->CreateSystem<lull::RenderSystem>();
  entity_factory->CreateSystem<lull::RigSystem>();
  entity_factory->CreateSystem<lull::ScriptSystem>();
  entity_factory->CreateSystem<lull::StategraphSystem>();
  entity_factory->CreateSystem<lull::TextSystem>();
  entity_factory->CreateSystem<lull::TransformSystem>();

  entity_factory->Initialize<EntityDef, ComponentDef>(
      GetEntityDef, EnumNamesComponentDefType());

  registry_->Create<PreviewWindow>(registry_.get(), kPreviewWidth,
                                   kPreviewHeight);
  registry_->Create<BuildBlueprintPopup>(registry_.get());
  registry_->Create<BuildModelPopup>(registry_.get());
  registry_->Create<BuildShaderPopup>(registry_.get());
  registry_->Create<Console>(registry_.get());
  registry_->Create<EntityEditor>(registry_.get());

  auto* binder = registry_->Get<FunctionBinder>();
  binder->RegisterFunction("pause", [this]() {
    paused_ = !paused_;
  });
  binder->RegisterFunction("step", [this]() {
    single_step_ = true;
  });
  binder->RegisterFunction("delta-time", [this](float dt) {
    dt_override_ = dt;
  });
}

void Viewer::AdvanceFrame(double dt, int width, int height) {
  AdvanceLullabySystems(dt);
  UpdateViewerGui(width, height);
}

void Viewer::AdvanceLullabySystems(double dt) {
  registry_->Get<AssetLoader>()->Finalize(1);
  if (paused_ && !single_step_) {
    return;
  }
  single_step_ = false;

  auto delta_time = std::chrono::duration_cast<Clock::duration>(Secondsf(dt));
  if (dt_override_ > 0.f) {
    delta_time = DurationFromMilliseconds(dt_override_);
  }
  registry_->Get<InputManager>()->AdvanceFrame(delta_time);

  dispatcher_->Dispatch();
  registry_->Get<lull::DispatcherSystem>()->Dispatch();

  registry_->Get<lull::ScriptSystem>()->AdvanceFrame(delta_time);
  registry_->Get<lull::StategraphSystem>()->AdvanceFrame(delta_time);
  registry_->Get<lull::AnimationSystem>()->AdvanceFrame(delta_time);
  registry_->Get<lull::PhysicsSystem>()->AdvanceFrame(delta_time);
  registry_->Get<lull::LightSystem>()->AdvanceFrame();
  registry_->Get<lull::RenderSystem>()->ProcessTasks();
  registry_->Get<lull::RenderSystem>()->SubmitRenderData();
  // registry_->Get<lull::AudioSystem>()->Update();
}

void Viewer::UpdateViewerGui(int width, int height) {
  static bool show_test_window = false;
  if (show_test_window) {
    ImGui::ShowTestWindow();
  }
  static bool show_user_guide = false;
  if (show_user_guide) {
    ImGui::ShowUserGuide();
  }

  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Load Entity Blueprint...")) {
        const std::string filename =
            OpenFileDialog("Open File...");
        if (!filename.empty()) {
          CreateEntity(filename);
        }
      }
      if (ImGui::MenuItem("Import Folder...")) {
        const std::string filename =
            OpenDirectoryDialog("Select Directory...");
        if (!filename.empty()) {
          ImportDirectory(filename);
        }
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Exit", "Alt+F4")) {
        Exit("", 0);
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Scene")) {
      if (ImGui::MenuItem("Explorer")) {
      }
      if (ImGui::MenuItem("Preview Window")) {
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Reset")) {
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Compile")) {
      if (ImGui::MenuItem("Blueprint...")) {
        registry_->Get<BuildBlueprintPopup>()->Open();
      }
      if (ImGui::MenuItem("Shader...")) {
        registry_->Get<BuildShaderPopup>()->Open();
      }
      if (ImGui::MenuItem("Texture...")) {
      }
      if (ImGui::MenuItem("Model...")) {
        registry_->Get<BuildModelPopup>()->Open();
      }
      if (ImGui::MenuItem("Animation...")) {
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem("ShowUserGuide()")) {
        show_user_guide = !show_user_guide;
      }
      if (ImGui::MenuItem("ShowTestWindow()")) {
        show_test_window = !show_test_window;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  const int flags = ImGuiSetCond_FirstUseEver;
  const int menu_height = ImGui::GetItemsLineHeightWithSpacing();

  ImGui::SetNextWindowPos(ImVec2(0.f, menu_height), flags);
  ImGui::SetNextWindowSize(ImVec2(width * 0.7f, height * 0.7f), flags);
  registry_->Get<PreviewWindow>()->AdvanceFrame();

  ImGui::SetNextWindowPos(ImVec2(0.f, (height * 0.7f) + menu_height), flags);
  ImGui::SetNextWindowSize(ImVec2(width, (height * 0.3f) - menu_height), flags);
  registry_->Get<Console>()->AdvanceFrame();

  ImGui::SetNextWindowPos(ImVec2(width * 0.7f, menu_height), flags);
  ImGui::SetNextWindowSize(ImVec2(width * 0.3f, height * 0.7f), flags);
  registry_->Get<EntityEditor>()->AdvanceFrame();

  registry_->Get<BuildBlueprintPopup>()->AdvanceFrame();
  registry_->Get<BuildModelPopup>()->AdvanceFrame();
  registry_->Get<BuildShaderPopup>()->AdvanceFrame();
}

void Viewer::OnShutdown() { registry_.reset(); }

}  // namespace tool
}  // namespace lull
