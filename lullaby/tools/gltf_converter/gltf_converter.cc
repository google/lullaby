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

#include "lullaby/tools/gltf_converter/gltf_converter.h"

#include "lullaby/util/filename.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace lull {
namespace tool {
namespace {

// Some magic constants used by GLB files.
static const uint32_t kGlbMagic = 0x46546C67;
static const uint32_t kGlbVersion = 0x00000002;
static const uint32_t kJsonType = 0x4E4F534A;
static const uint32_t kBinaryType = 0x004E4942;

// Determines the number of bytes needed in order to align the data to a 4-byte
// boundary as per the GLB specification.
size_t PaddingNeeded(size_t size) {
  const int remainder = static_cast<int>(size) % 4;
  return (remainder == 0) ? 0 : (4 - remainder);
}

// Writes a value (int or float) to the output array.
template <typename T>
void Write(ByteArray* arr, const T& value) {
  union {
    uint8_t bytes[sizeof(T)];
    T value;
  } tmp;

  tmp.value = value;
  for (size_t i = 0; i < sizeof(T); ++i) {
    arr->push_back(tmp.bytes[i]);
  }
}

// Writes the data from a container to the output array.
template <typename T>
void WriteBytes(ByteArray* arr, const T& data) {
  arr->insert(arr->end(), data.begin(), data.end());
  const size_t padding = PaddingNeeded(data.size());
  for (size_t i = 0; i < padding; ++i) {
    arr->push_back(0);
  }
}

// Calculates the size of chunk (including padding) that is needed to store the
// specified data object into a GLB file
template <typename T>
uint32_t CalculateChunkSize(const T& data) {
  if (data.empty()) {
    return 0;
  }
  // A chunk has three parts:
  // - chunkLength: uint32, length of chunkData, in bytes.
  // - chunkType: uint32, indicates the type of chunk.
  // - chunkData: byte array, the binary payload of chunk.
  const size_t size =
      (sizeof(uint32_t) * 2) + data.size() + PaddingNeeded(data.size());
  return static_cast<uint32_t>(size);
}

class GltfToGlbConverter {
 public:
  using LoadFn = std::function<ByteArray(string_view)>;

  GltfToGlbConverter(Span<uint8_t> gltf, LoadFn load_fn);

  ByteArray ToGlb();

 private:
  rapidjson::Document json_;
  ByteArray bin_;
  LoadFn load_fn_;
};

GltfToGlbConverter::GltfToGlbConverter(Span<uint8_t> gltf, LoadFn load_fn)
    : load_fn_(std::move(load_fn)) {
  json_.Parse(reinterpret_cast<const char*>(gltf.data()));
  auto& alloc = json_.GetAllocator();

  // Keywords for gltf json data.
  constexpr const char kBufferView[] = "bufferView";
  constexpr const char kBufferViews[] = "bufferViews";
  constexpr const char kBuffer[] = "buffer";
  constexpr const char kBuffers[] = "buffers";
  constexpr const char kByteLength[] = "byteLength";
  constexpr const char kByteOffset[] = "byteOffset";
  constexpr const char kImages[] = "images";
  constexpr const char kMimeType[] = "mimeType";
  constexpr const char kMimeTypeJpg[] = "image/jpeg";
  constexpr const char kMimeTypePng[] = "image/png";
  constexpr const char kUri[] = "uri";

  // Load the bin file if one is specified.
  if (json_.HasMember(kBuffers)) {
    auto& buffers = json_[kBuffers];
    assert(buffers.GetArray().Size() == 1);

    for (auto& buffer : buffers.GetArray()) {
      if (buffer.HasMember(kUri)) {
        const int size = buffer[kByteLength].GetInt();
        bin_ = load_fn_(buffer[kUri].GetString());
        assert(size == static_cast<int>(bin_.size()));
      }
    }
  }

  // Add textures to bin file
  auto& buffer_views = json_[kBufferViews];
  auto& images = json_[kImages];
  for (auto& image : images.GetArray()) {
    if (image.HasMember(kUri)) {
      const std::string uri = image[kUri].GetString();
      ByteArray bytes = load_fn_(uri);
      assert(!bytes.empty());

      const int buffer = 0;
      const int offset = static_cast<int>(bin_.size());
      const int length = static_cast<int>(bytes.size());
      const int index = static_cast<int>(buffer_views.GetArray().Size());

      image.EraseMember(kUri);
      image.AddMember(kBufferView, index, alloc);
      const std::string ext = GetExtensionFromFilename(uri);
      if (ext == ".jpg" || ext == ".jpeg") {
        image.AddMember(kMimeType, kMimeTypeJpg, alloc);
      } else if (ext == ".png") {
        image.AddMember(kMimeType, kMimeTypePng, alloc);
      } else {
        assert(false);
      }
      rapidjson::Value value;
      value.SetObject();
      value.AddMember(kBuffer, buffer, alloc);
      value.AddMember(kByteOffset, offset, alloc);
      value.AddMember(kByteLength, length, alloc);
      buffer_views.GetArray().PushBack(std::move(value), alloc);

      WriteBytes(&bin_, bytes);
    }
  }

  // Reset buffers to just refer to the single bin data.
  if (!json_.HasMember(kBuffers)) {
    json_.AddMember(kBuffers, rapidjson::Value(), alloc);
    json_[kBuffers].SetArray();
  }
  json_[kBuffers].GetArray().Clear();
  json_[kBuffers].GetArray().PushBack(rapidjson::Value(), alloc);
  auto& buffer = json_[kBuffers].GetArray()[0];
  buffer.SetObject();
  buffer.AddMember(kByteLength, bin_.size(), alloc);
}

ByteArray GltfToGlbConverter::ToGlb() {
  // Get the JSON string contents.
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  json_.Accept(writer);
  const std::string txt = buffer.GetString();

  // Calculate the final size of the glb.
  const uint32_t header_size = static_cast<uint32_t>(sizeof(uint32_t) * 3);
  const uint32_t total =
      header_size + CalculateChunkSize(txt) + CalculateChunkSize(bin_);

  ByteArray glb;

  // The GLB header.
  Write(&glb, kGlbMagic);
  Write(&glb, kGlbVersion);
  Write(&glb, total);

  // Chunk 0 contains the json data.
  Write(&glb, uint32_t(txt.size()));
  Write(&glb, kJsonType);
  WriteBytes(&glb, txt);

  // Chunk 1 contains the binary data.
  if (!bin_.empty()) {
    Write(&glb, uint32_t(bin_.size()));
    Write(&glb, kBinaryType);
    WriteBytes(&glb, bin_);
  }
  return glb;
}

}  // namespace

ByteArray GltfToGlb(Span<uint8_t> gltf,
                    const std::function<ByteArray(string_view)>& load_fn) {
  GltfToGlbConverter converter(gltf, load_fn);
  return converter.ToGlb();
}

}  // namespace tool
}  // namespace lull
