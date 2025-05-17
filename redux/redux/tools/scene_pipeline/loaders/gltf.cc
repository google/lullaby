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

#include "redux/tools/scene_pipeline/loaders/gltf.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "redux/tools/scene_pipeline/buffer.h"
#include "redux/tools/scene_pipeline/drawable.h"
#include "redux/tools/scene_pipeline/image.h"
#include "redux/tools/scene_pipeline/index_buffer.h"
#include "redux/tools/scene_pipeline/loaders/gltf_utils.h"
#include "redux/tools/scene_pipeline/loaders/import_options.h"
#include "redux/tools/scene_pipeline/loaders/import_utils.h"
#include "redux/tools/scene_pipeline/material.h"
#include "redux/tools/scene_pipeline/model.h"
#include "redux/tools/scene_pipeline/sampler.h"
#include "redux/tools/scene_pipeline/scene.h"
#include "redux/tools/scene_pipeline/type_id.h"
#include "redux/tools/scene_pipeline/types.h"
#include "redux/tools/scene_pipeline/vertex_buffer.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/types/span.h"
#include "cgltf.h"

namespace redux::tool {

class GltfLoader {
 public:
  explicit GltfLoader(const ImportOptions& opts) : opts_(opts) {}

  std::unique_ptr<Scene> ImportScene(std::string_view path) {
    scene_ = std::make_unique<Scene>();

    Buffer file = opts_.file_loader(path);

    cgltf_options opts;
    std::memset(&opts, 0, sizeof(opts));
    cgltf_data* gltf = nullptr;
    cgltf_result result = cgltf_parse(&opts, file.data(), file.size(), &gltf);
    if (result != cgltf_result_success) {
      return nullptr;
    }

    // Register the GLTF file itself as the first buffer. This is useful for
    // GLBs.
    int first_buffer = 0;
    if (gltf->buffers_count > 0 && gltf->bin) {
      if (file.contains(reinterpret_cast<const std::byte*>(gltf->bin))) {
        // The "bin" data is within the entire GLB binary itself.  However,
        // all the buffers in the cgltf file refer to the "bin" data and not
        // the entire GLB binary.  So, to make our life easier, we retarget
        // our buffer to refer to the "bin" data, but the deleter still needs
        // to delete the GLB itself.
        int offset = reinterpret_cast<const char*>(gltf->bin) -
                     reinterpret_cast<const char*>(file.data());
        int length = file.size() - offset;
        std::byte* ptr = file.release();
        auto retargetted = Buffer::Own(ptr + offset, length,
                                       [ptr](std::byte*) { delete[] ptr; });

        scene_->buffers.emplace_back(std::move(retargetted));
        buffer_map_[&gltf->buffers[0]] = BufferIndex(0);
        ++first_buffer;
      }
    }

    for (int i = first_buffer; i < gltf->buffers_count; ++i) {
      RegisterBuffer(&gltf->buffers[i]);
    }
    for (int i = 0; i < gltf->images_count; ++i) {
      RegisterImage(&gltf->images[i]);
    }
    for (int i = 0; i < gltf->materials_count; ++i) {
      RegisterMaterial(&gltf->materials[i]);
    }

    for (int i = 0; i < gltf->scenes_count; ++i) {
      const cgltf_scene* gltf_scene = &gltf->scenes[i];
      for (int j = 0; j < gltf_scene->nodes_count; ++j) {
        ProcessNode(gltf_scene->nodes[j]);
      }
    }

    cgltf_free(gltf);
    return std::move(scene_);
  }

 private:
  const ImportOptions& opts_;
  std::unique_ptr<Scene> scene_;
  Model* curr_model_ = nullptr;
  Model::Node* curr_node_ = nullptr;
  absl::flat_hash_map<const cgltf_image*, ImageIndex> image_map_;
  absl::flat_hash_map<const cgltf_buffer*, BufferIndex> buffer_map_;
  absl::flat_hash_map<const cgltf_material*, MaterialIndex> material_map_;

  BufferIndex GetBufferIndex(const cgltf_buffer* buffer) const {
    auto it = buffer_map_.find(buffer);
    CHECK(it != buffer_map_.end()) << "Cannot find buffer.";
    return it->second;
  }

  MaterialIndex GetMaterialIndex(const cgltf_material* gltf_material) const {
    auto it = material_map_.find(gltf_material);
    CHECK(it != material_map_.end()) << "Cannot find material.";
    return it->second;
  }

  ImageIndex GetImageIndex(const cgltf_image* gltf_image) const {
    auto it = image_map_.find(gltf_image);
    CHECK(it != image_map_.end()) << "Cannot find image.";
    return it->second;
  }

  ByteSpan GetBufferViewSpan(const cgltf_buffer_view* view) const {
    const BufferIndex idx = GetBufferIndex(view->buffer);
    auto base_span = scene_->buffers[idx].span();
    CHECK(view->offset + view->size <= base_span.size())
        << "Buffer view out of bounds.";
    return ByteSpan(&base_span[view->offset], view->size);
  }

  VertexBuffer ProcessVertexBuffer(const cgltf_attribute* attribs,
                                   cgltf_size num_attribs) {
    VertexBuffer vertex_buffer;
    vertex_buffer.num_vertices = attribs[0].data->count;

    for (int i = 0; i < num_attribs; ++i) {
      const cgltf_accessor* accessor = attribs[i].data;
      CHECK_EQ(accessor->buffer_view->stride, 0)
          << "We currently only support packed vertex attributes.";
      CHECK_EQ(vertex_buffer.num_vertices, accessor->count)
          << "Inconsistent vertex counts for accessor: " << attribs[i].type;

      auto& attrib = vertex_buffer.attributes.emplace_back();
      attrib.name = AttributeName(attribs[i].type);
      CHECK(!attrib.name.empty()) << "Unknown attrib: " << attribs[i].type;

      attrib.type = AttributeType(accessor->type);
      CHECK(attrib.type != kInvalidTypeId)
          << "Unknown attrib type: " << accessor->type;

      attrib.index = attribs[i].index;
      attrib.stride = accessor->stride;

      attrib.buffer_view.buffer_index =
          GetBufferIndex(accessor->buffer_view->buffer);
      attrib.buffer_view.offset =
          accessor->offset + accessor->buffer_view->offset;
      attrib.buffer_view.length = accessor->count * TypeSize(attrib.type);
      CHECK_EQ(attrib.buffer_view.length, accessor->buffer_view->size)
          << "Inconsistent buffer view size for accessor: " << attribs[i].type;
    }

    return vertex_buffer;
  }

  IndexBuffer ProcessIndexBuffer(const cgltf_accessor* accessor) {
    IndexBuffer index_buffer;

    // ignore: accessor->name
    CHECK(!accessor->is_sparse) << "Sparse accessors not supported.";

    index_buffer.type = ComponentType(accessor->component_type);
    CHECK_EQ(accessor->buffer_view->stride, 0) << "Indices should be packed.";
    CHECK_EQ(accessor->stride, TypeSize(index_buffer.type))
        << "Indices should be packed.";
    CHECK(index_buffer.type == GetTypeId<std::uint16_t>() ||
          index_buffer.type == GetTypeId<std::uint32_t>())
        << "Unsupported index type: " << accessor->component_type;

    index_buffer.num_indices = accessor->count;
    index_buffer.buffer_view.buffer_index =
        GetBufferIndex(accessor->buffer_view->buffer);
    index_buffer.buffer_view.offset =
        accessor->offset + accessor->buffer_view->offset;
    index_buffer.buffer_view.length =
        accessor->count * TypeSize(index_buffer.type);
    CHECK_EQ(index_buffer.buffer_view.length, accessor->buffer_view->size)
        << "Inconsistent buffer view size for indices.";
    return index_buffer;
  }

  void ProcessPrimitive(const cgltf_node* gltf_node,
                        const cgltf_primitive* gltf_primitive) {
    Drawable drawable;
    drawable.primitive_type = PrimitiveType(gltf_primitive->type);
    CHECK(drawable.primitive_type != Drawable::PrimitiveType::kUnspecified)
        << "Unknown primitive type: " << gltf_primitive->type;

    drawable.material_index = GetMaterialIndex(gltf_primitive->material);
    drawable.vertex_buffer = ProcessVertexBuffer(
        gltf_primitive->attributes, gltf_primitive->attributes_count);
    drawable.index_buffer = ProcessIndexBuffer(gltf_primitive->indices);
    drawable.offset = gltf_primitive->indices->offset;
    drawable.count = gltf_primitive->indices->count;
    if (gltf_primitive->has_draco_mesh_compression) {
      LOG(FATAL) << "Draco compression not supported.";
    }

    cgltf_node_transform_local(gltf_node, &curr_node_->transform.data[0][0]);
    curr_node_->drawable_indexes.push_back(
        DrawableIndex(scene_->drawables.size()));
    scene_->drawables.emplace_back(std::move(drawable));
  }

  void ProcessNode(const cgltf_node* gltf_node) {
    bool is_model_root = false;

    if (curr_node_ && gltf_node->name) {
      curr_node_->name = gltf_node->name;
    }

    if (gltf_node->mesh) {
      if (curr_model_ == nullptr) {
        // We assume any mesh that doesn't have a mesh ancestor is the root
        // of a new Model.
        scene_->models.emplace_back();
        curr_model_ = &scene_->models.back();
        curr_node_ = &curr_model_->root_node;
        cgltf_node_transform_world(gltf_node,
                                   &curr_model_->transform.data[0][0]);
        is_model_root = true;
      }

      cgltf_mesh* gltf_mesh = gltf_node->mesh;
      for (int i = 0; i < gltf_mesh->primitives_count; ++i) {
        ProcessPrimitive(gltf_node, &gltf_mesh->primitives[i]);
      }
    }

    for (int i = 0; i < gltf_node->children_count; ++i) {
      Model::Node* parent = curr_node_;
      curr_node_ = &curr_node_->children.emplace_back();
      ProcessNode(gltf_node->children[i]);
      curr_node_ = parent;
    }

    if (gltf_node->mesh && is_model_root) {
      curr_model_ = nullptr;
    }
  }

  void RegisterBuffer(const cgltf_buffer* gltf_buffer) {
    // ignore: gltf_buffer->name

    // This buffer points to the GLTF file itself (i.e. a bin file).
    if (!gltf_buffer->data && !gltf_buffer->uri) {
      CHECK(gltf_buffer->data == scene_->buffers.front().data())
          << "Expected first buffer to be the GLTF file itself.";
      buffer_map_[gltf_buffer] = BufferIndex(0);
    } else {
      CHECK(gltf_buffer->uri != nullptr) << "External buffers not supported.";
      Buffer buffer = opts_.file_loader(gltf_buffer->uri);
      buffer_map_[gltf_buffer] = BufferIndex(scene_->buffers.size());
      scene_->buffers.emplace_back(std::move(buffer));
    }
  }

  void RegisterImage(const cgltf_image* gltf_image) {
    // ignore: gltf_image->name
    Image image;

    CHECK(gltf_image->buffer_view || gltf_image->uri)
        << "Images must have a buffer view or a URI.";

    ImageIndex image_index;
    if (gltf_image->buffer_view) {
      CHECK(gltf_image->buffer_view->buffer) << "Buffer view has no buffer.";
      auto encoded_image = GetBufferViewSpan(gltf_image->buffer_view);
      image_index = DecodeImageIntoScene(scene_.get(), opts_, encoded_image);
    } else if (strncmp(gltf_image->uri, "data:", 5) == 0) {
      CHECK(false) << "Data URIs not currently supported.";
    } else {
      image_index = LoadImageIntoScene(scene_.get(), opts_, gltf_image->uri);
    }

    image_map_[gltf_image] = image_index;
  }

  Sampler MakeSampler(const cgltf_texture_view& view, const float4& mask) {
    Sampler sampler;

    const cgltf_texture* gltf_texture = view.texture;
    CHECK(gltf_texture != nullptr) << "Can only sample from textures.";

    sampler.image_index = GetImageIndex(gltf_texture->image);
    sampler.texcoord = view.texcoord;
    sampler.channel_mask = mask;
    if (const auto* gltf_sampler = gltf_texture->sampler) {
      // ignore: sampler->name
      sampler.min_filter = TextureMinFilter(gltf_sampler->min_filter);
      sampler.mag_filter = TextureMagFilter(gltf_sampler->mag_filter);
      sampler.wrap_s = TextureWrapMode(gltf_sampler->wrap_s);
      sampler.wrap_t = TextureWrapMode(gltf_sampler->wrap_t);
    }

    return sampler;
  }

  void RegisterMaterial(const cgltf_material* gltf_material) {
    Material material;

    if (gltf_material->has_clearcoat) {
      material.shading_model = Material::kClearCoat;
    } else if (gltf_material->has_pbr_specular_glossiness) {
      material.shading_model = Material::kSpecularGlossiness;
    } else if (gltf_material->has_pbr_metallic_roughness) {
      material.shading_model = Material::kMetallicRoughness;
    } else if (gltf_material->unlit) {
      material.shading_model = Material::kUnlit;
    } else {
      LOG(FATAL) << "Unknown GLTF shading model.";
    }

    auto& props = material.properties;

    // Basic properties.
    props[Material::kDoubleSided].Set<bool>(gltf_material->double_sided);
    props[Material::kAlphaCutoff].Set<float>(gltf_material->alpha_cutoff);
    props[Material::kAlphaMode].Set<int>(AlphaMode(gltf_material->alpha_mode));

    if (gltf_material->emissive_texture.texture) {
      const cgltf_texture_view& view = gltf_material->emissive_texture;
      props[Material::kEmissiveTexture].Set(
          MakeSampler(view, Sampler::kRgbMask));
    }
    if (gltf_material->has_emissive_strength) {
      auto* sub = &gltf_material->emissive_strength;
      float3 emissive = float3(gltf_material->emissive_factor);
      emissive.x *= sub->emissive_strength;
      emissive.y *= sub->emissive_strength;
      emissive.z *= sub->emissive_strength;
      props[Material::kEmissive].Set(float3(emissive));
    } else {
      props[Material::kEmissive].Set(float3(gltf_material->emissive_factor));
    }

    if (gltf_material->normal_texture.texture) {
      const cgltf_texture_view& view = gltf_material->normal_texture;
      props[Material::kNormalScale].Set<float>(view.scale);
      Sampler sampler = MakeSampler(view, Sampler::kRgbMask);
      // Normal maps are expected to be scaled and biased.
      sampler.bias = float4(-1, -1, -1, -1);
      sampler.scale = float4(2, 2, 2, 2);
      props[Material::kNormalTexture].Set(sampler);
    }

    // Occlusion texture properties.
    if (gltf_material->occlusion_texture.texture) {
      const cgltf_texture_view& view = gltf_material->occlusion_texture;
      // Occlusion strength is stored in the scale field.
      props[Material::kOcclusionStrength].Set<float>(view.scale);
      props[Material::kOcclusionTexture].Set(
          MakeSampler(view, Sampler::kRedMask));
    }

    if (gltf_material->has_pbr_metallic_roughness) {
      auto* sub = &gltf_material->pbr_metallic_roughness;

      // Base color properties.
      props[Material::kBaseColor].Set(float4(sub->base_color_factor));
      if (sub->base_color_texture.texture) {
        const cgltf_texture_view& view = sub->base_color_texture;
        props[Material::kBaseColorTexture].Set(
            MakeSampler(view, Sampler::kRgbaMask));
      }

      // Metallic roughness properties.
      props[Material::kMetallic].Set<float>(sub->metallic_factor);
      props[Material::kRoughness].Set<float>(sub->roughness_factor);
      if (sub->metallic_roughness_texture.texture) {
        const cgltf_texture_view& view = sub->metallic_roughness_texture;
        props[Material::kRoughnessTexture].Set(
            MakeSampler(view, Sampler::kGreenMask));
        props[Material::kMetallicTexture].Set(
            MakeSampler(view, Sampler::kBlueMask));
      }
    }

    if (gltf_material->has_pbr_specular_glossiness) {
      auto* sub = &gltf_material->pbr_specular_glossiness;

      // Diffuse properties (mapped onto BaseColor).
      props[Material::kBaseColor].Set(float4(sub->diffuse_factor));
      if (sub->diffuse_texture.texture) {
        const cgltf_texture_view& view = sub->diffuse_texture;
        props[Material::kBaseColorTexture].Set(
            MakeSampler(view, Sampler::kRgbaMask));
      }

      // Specular glossiness properties.
      props[Material::kGlossiness].Set<float>(sub->glossiness_factor);
      props[Material::kSpecular].Set(float3(sub->specular_factor));
      if (sub->specular_glossiness_texture.texture) {
        const cgltf_texture_view& view = sub->specular_glossiness_texture;
        props[Material::kSpecularTexture].Set(
            MakeSampler(view, Sampler::kRgbMask));
        props[Material::kGlossinessTexture].Set(
            MakeSampler(view, Sampler::kAlphaMask));
      }
    }

    if (gltf_material->has_clearcoat) {
      auto* sub = &gltf_material->clearcoat;

      // Clearcoat properties.
      props[Material::kClearCoat].Set<float>(sub->clearcoat_factor);
      if (sub->clearcoat_texture.texture) {
        const cgltf_texture_view& view = sub->clearcoat_texture;
        props[Material::kClearCoatTexture].Set(
            MakeSampler(view, Sampler::kRedMask));
      }

      // Clearcoat roughness properties.
      props[Material::kClearCoatRoughness].Set<float>(
          sub->clearcoat_roughness_factor);
      if (sub->clearcoat_roughness_texture.texture) {
        const cgltf_texture_view& view = sub->clearcoat_roughness_texture;
        props[Material::kClearCoatRoughnessTexture].Set(
            MakeSampler(view, Sampler::kGreenMask));
      }
      // Clearcoat normal properties.
      if (sub->clearcoat_normal_texture.texture) {
        const cgltf_texture_view& view = sub->clearcoat_normal_texture;
        props[Material::kClearCoatNormalTexture].Set(
            MakeSampler(view, Sampler::kRgbMask));
      }
    }

    material_map_[gltf_material] = MaterialIndex(scene_->materials.size());
    scene_->materials.emplace_back(std::move(material));
  }
};

std::unique_ptr<Scene> LoadGltf(std::string_view path,
                                const ImportOptions& opts) {
  GltfLoader importer(opts);
  return importer.ImportScene(path);
}

}  // namespace redux::tool
