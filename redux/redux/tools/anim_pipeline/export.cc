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

#include "redux/tools/anim_pipeline/export.h"

#include <limits>

#include "redux/data/asset_defs/anim_asset_def_generated.h"
#include "redux/engines/animation/spline/compact_spline.h"
#include "redux/modules/flatbuffers/common.h"
#include "redux/modules/flatbuffers/math.h"
#include "redux/modules/flatbuffers/var.h"
#include "redux/tools/common/flatbuffer_utils.h"
#include "redux/tools/common/log_utils.h"

namespace redux::tool {

static std::unique_ptr<fbs::HashStringT> MakeName(std::string_view name) {
  return std::make_unique<fbs::HashStringT>(CreateHashStringT(name));
}

static CompactSplinePtr CreateCompactSpline(AnimCurve::CurveSegment nodes) {
  Interval y_range(Interval::Empty());
  for (const auto& n : nodes) {
    y_range = y_range.Included(n.value);
  }

  const float x_granularity = CompactSpline::RecommendXGranularity(
      static_cast<float>(nodes.back().time_ms));

  CompactSplinePtr spline = CompactSpline::Create(nodes.size());
  spline->Init(y_range, x_granularity);

  float last_time = -std::numeric_limits<float>::max();
  for (const auto& node : nodes) {
    const float n_time = static_cast<float>(std::max(0.0f, node.time_ms));
    // Exclude any decreasing time values, as these may produce invalid spans at
    // evaluation time and lead to errors.
    if (n_time >= last_time) {
      spline->AddNode(n_time, node.value, node.derivative,
                      kAddWithoutModification);
      last_time = n_time;
    }
  }
  return spline;
}


static std::unique_ptr<AnimChannelSplineAssetDefT> ExportSpline(
    const AnimCurve& curve) {
  // We generate the same compact spline as the runtime in order to extract
  // good values for export.
  CompactSplinePtr spline = CreateCompactSpline(curve.Nodes());

  auto res = std::make_unique<AnimChannelSplineAssetDefT>();
  res->y_range_start = spline->y_range().min;
  res->y_range_end = spline->y_range().max;
  res->x_granularity = spline->x_granularity();

  const int num_nodes = spline->num_nodes();
  res->nodes.reserve(num_nodes);
  for (int i = 0; i < num_nodes; ++i) {
    const auto& node = spline->nodes()[i];
    const auto x = node.x();
    const auto y = node.y();
    const auto angle = node.angle();
    static_assert(sizeof(x) == sizeof(uint16_t));
    static_assert(sizeof(y) == sizeof(uint16_t));
    static_assert(sizeof(angle) == sizeof(uint16_t));
    res->nodes.emplace_back(x, y, angle);
  }
  return res;
}

static std::unique_ptr<AnimChannelConstValueAssetDefT> ExportConstValue(
    const AnimCurve& curve) {
  auto res = std::make_unique<AnimChannelConstValueAssetDefT>();
  res->value = curve.Nodes().front().value;
  return res;
}

static std::unique_ptr<BoneAnimAssetDefT> ExportBoneAnim(
    const AnimBone& bone_anim) {
  auto bone_anim_def = std::make_unique<BoneAnimAssetDefT>();
  for (const AnimCurve& curve : bone_anim.curves) {
    auto op = std::make_unique<AnimChannelAssetDefT>();
    op->type = curve.Type();

    if (curve.Nodes().empty()) {
      LOG(ERROR) << "Skipping empty channel for bone " << bone_anim.name;
      continue;
    } else if (curve.Nodes().size() == 1) {
      auto val = ExportConstValue(curve);
      op->data.type = AnimChannelDataAssetDef::AnimChannelConstValueAssetDef;
      op->data.value = val.release();
    } else {
      auto spline = ExportSpline(curve);
      op->data.type = AnimChannelDataAssetDef::AnimChannelSplineAssetDef;
      op->data.value = spline.release();
    }
    bone_anim_def->ops.emplace_back(std::move(op));
  }
  return bone_anim_def;
}

void LogResults(const AnimAssetDefT& out, Logger& log) {
  log("version: ", out.version);

  const size_t num_bones = out.bone_names.size();
  log("bones: ", num_bones);
  for (size_t i = 0; i < num_bones; ++i) {
    const std::string& name = out.bone_names[i]->name;
    const int parent = out.bone_parents[i];
    log("  ", name, " (", parent, ")");
  }

  log("anims: ", num_bones);
  for (size_t i = 0; i < num_bones; ++i) {
    const std::string& name = out.bone_names[i]->name;
    const size_t num_ops = out.bone_anims[i]->ops.size();
    log("  ", name, " (", num_ops, ")");
    for (const auto& channel : out.bone_anims[i]->ops) {
      const char* type_name = EnumNameAnimChannelType(channel->type);
      const auto* const_data = channel->data.AsAnimChannelConstValueAssetDef();
      const auto* spline_data = channel->data.AsAnimChannelSplineAssetDef();

      if (const_data) {
        const float value = const_data->value;
        log("    ", type_name, " const ", value);
      } else if (spline_data) {
        const size_t num = spline_data->nodes.size();
        log("    ", type_name, " spline ", num);
        for (const auto& n : spline_data->nodes) {
          const float time =
              static_cast<float>(n.x()) * spline_data->x_granularity;
          log("      ", time, " ", n.y(), " ", n.angle());
        }
      }
    }
  }
}

DataContainer ExportAnimation(AnimationPtr anim, Logger& log) {
  AnimAssetDefT anim_def;
  anim_def.version = 1;
  anim_def.repeat = anim->Repeat();
  anim_def.length_in_seconds =
      (anim->MaxAnimatedTimeMs() - anim->MinAnimatedTimeMs()) * 1000.0f;

  const size_t num_bones = anim->NumBones();
  anim_def.bone_names.reserve(num_bones);
  anim_def.bone_parents.reserve(num_bones);
  anim_def.bone_anims.reserve(num_bones);

  for (size_t i = 0; i < num_bones; ++i) {
    const AnimBone& bone_anim = anim->GetBone(i);
    anim_def.bone_names.emplace_back(MakeName(bone_anim.name));
    anim_def.bone_parents.emplace_back(bone_anim.parent_bone_index);
    anim_def.bone_anims.emplace_back(ExportBoneAnim(bone_anim));
  }

  LogResults(anim_def, log);

  return BuildFlatbuffer(anim_def);
}

}  // namespace redux::tool
