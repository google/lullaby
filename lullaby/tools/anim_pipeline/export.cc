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

#include "lullaby/tools/anim_pipeline/export.h"
#include "flatbuffers/flatbuffers.h"
#include "motive/anim_generated.h"
#include "motive/anim_list_generated.h"

namespace lull {
namespace tool {

motive::Range SplineYRange(const AnimChannel& ch) {
  // Find extreme values for nodes.
  motive::Range y_range(motive::Range::Empty());
  for (auto n = ch.nodes.begin(); n != ch.nodes.end(); ++n) {
    y_range = y_range.Include(n->val);
  }
  return y_range;
}

flatbuffers::Offset<motive::CompactSplineFb> CreateSplineFlatBuffer(
    flatbuffers::FlatBufferBuilder& fbb, const AnimChannel& ch) {
  const auto& nodes = ch.nodes;
  assert(nodes.size() > 1);

  // Maximize the bits we get for x by making the last time the maximum
  // x-value.
  const float x_granularity = motive::CompactSpline::RecommendXGranularity(
      static_cast<float>(nodes.back().time));
  const motive::Range y_range = SplineYRange(ch);

  // Construct the Spline from the node data directly.
  motive::CompactSpline* s = motive::CompactSpline::Create(
      static_cast<motive::CompactSplineIndex>(nodes.size()));
  s->Init(y_range, x_granularity);
  float last_time = -std::numeric_limits<float>::max();
  for (auto n = nodes.begin(); n != nodes.end(); ++n) {
    const float n_time = static_cast<float>(std::max(0, n->time));
    // Exclude any decreasing time values, as these may produce invalid spans at
    // evaluation time and lead to errors.
    if (n_time >= last_time) {
      s->AddNode(n_time, n->val, n->derivative,
                 motive::kAddWithoutModification);
      last_time = n_time;
    }
  }

  auto nodes_fb = fbb.CreateVectorOfStructs(
      reinterpret_cast<const motive::CompactSplineNodeFb*>(s->nodes()),
      s->num_nodes());

  auto spline_fb = motive::CreateCompactSplineFb(fbb, s->y_range().start(),
                                                 s->y_range().end(),
                                                 s->x_granularity(), nodes_fb);
  motive::CompactSpline::Destroy(s);
  return spline_fb;
}

flatbuffers::Offset<motive::RigAnimFb> CreateRigAnimFb(
    flatbuffers::FlatBufferBuilder& fbb, const Animation& anim,
    RepeatPreference repeat_preference) {
  std::vector<flatbuffers::Offset<motive::MatrixAnimFb>> matrix_anims;
  std::vector<flatbuffers::Offset<flatbuffers::String>> bone_names;
  std::vector<motive::BoneIndex> bone_parents;

  const motive::BoneIndex num_bones =
      static_cast<motive::BoneIndex>(anim.NumBones());
  matrix_anims.reserve(num_bones);
  bone_names.reserve(num_bones);
  bone_parents.reserve(num_bones);

  for (motive::BoneIndex bone_idx = 0; bone_idx < num_bones; ++bone_idx) {
    const AnimBone& bone = anim.GetBone(bone_idx);
    const auto& channels = bone.channels;

    // Output each channel as a MatrixOp, and gather in the `ops` vector.
    std::vector<flatbuffers::Offset<motive::MatrixOpFb>> ops;
    for (auto c = channels.begin(); c != channels.end(); ++c) {
      const auto& n = c->nodes;

      flatbuffers::Offset<void> value;
      motive::MatrixOpValueFb value_type;

      if (n.empty()) {
        LOG(ERROR) << "Skipping empty channel for bone " << bone.name;
        continue;
      } else if (n.size() == 1) {
        // Output constant value MatrixOp.
        value = motive::CreateConstantOpFb(fbb, n[0].val).Union();
        value_type = motive::MatrixOpValueFb_ConstantOpFb;
      } else {
        // We clamp negative times to 0, but it's going to look strange for
        // non-constant channels.
        if (n[0].time < 0) {
          LOG(WARNING) << bone.name << " (" << MatrixOpName(c->op)
                       << ") starts at negative time: " << n[0].time;
        }

        // Output spline MatrixOp.
        value = CreateSplineFlatBuffer(fbb, *c).Union();
        value_type = motive::MatrixOpValueFb_CompactSplineFb;
      }

      ops.push_back(motive::CreateMatrixOpFb(
          fbb, c->id, static_cast<motive::MatrixOperationTypeFb>(c->op),
          value_type, value));
    }

    // Convert vector into a FlatBuffers vector, and create the
    // MatrixAnimation.
    auto ops_fb = fbb.CreateVector(ops);
    auto matrix_anim_fb = CreateMatrixAnimFb(fbb, ops_fb, anim.IsSqtAnim());
    matrix_anims.push_back(matrix_anim_fb);
    bone_names.push_back(fbb.CreateString(bone.name));
    bone_parents.push_back(anim.BoneParent(bone_idx));
  }

  // Finish off the FlatBuffer by creating the root RigAnimFb table.
  auto bone_names_fb = fbb.CreateVector(bone_names);
  auto bone_parents_fb = fbb.CreateVector(bone_parents);
  auto matrix_anims_fb = fbb.CreateVector(matrix_anims);
  const bool repeat = anim.Repeat(repeat_preference);
  auto anim_name_fb = fbb.CreateString(anim.GetName());
  auto rig_anim_fb = CreateRigAnimFb(fbb, matrix_anims_fb, bone_parents_fb,
                                     bone_names_fb, repeat, anim_name_fb);
  return rig_anim_fb;
}

ByteArray ExportAnimation(const Animation& animation) {
  if (animation.NumBones() > motive::kMaxNumBones) {
    LOG(ERROR) << "Too many bones in animation: " << animation.NumBones();
    return {};
  }

  const RepeatPreference repeat_preference = kNeverRepeat;

  flatbuffers::FlatBufferBuilder fbb;
  const flatbuffers::Offset<motive::RigAnimFb> rig_anim_offset =
      CreateRigAnimFb(fbb, animation, repeat_preference);
  motive::FinishRigAnimFbBuffer(fbb, rig_anim_offset);

  return {fbb.GetBufferPointer(), fbb.GetBufferPointer() + fbb.GetSize()};
}

}  // namespace tool
}  // namespace lull
