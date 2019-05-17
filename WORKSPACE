load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("//dev:custom_rules.bzl", "local_repository_env")
load("//third_party/libfbx:build_defs.bzl", "FBX_BUILD_FILE_CONTENTS")

http_archive(
  name = "absl",
  urls = ["https://github.com/abseil/abseil-cpp/archive/7aacab8ae05d.tar.gz"],
  build_file = "BUILD.absl",
  strip_prefix = "abseil-cpp-7aacab8ae05d7115049923da9cfbf584dc1f8338",
)

http_archive(
  name = "assimp",
  urls = ["https://github.com/assimp/assimp/archive/80799bdbf90c.tar.gz"],
  build_file = "BUILD.assimp",
  strip_prefix = "assimp-80799bdbf90ce626475635815ee18537718a05b1",
)

http_archive(
  name = "astc_encoder",
  urls = ["https://github.com/ARM-software/astc-encoder/archive/a47b80f081f1.tar.gz"],
  build_file = "BUILD.astc_encoder",
  strip_prefix = "astc-encoder-a47b80f081f10c43d96bd10bcb713c71708041b9",
)

http_archive(
  name = "astc_codec",
  urls = ["https://github.com/google/astc-codec/archive/9b150919fd51.tar.gz"],
  build_file = "BUILD.astc_codec",
  strip_prefix = "astc-codec-9b150919fd518f0755d6624ddc36163467ced431",
)

http_archive(
  name = "bullet",
  urls = ["https://github.com/bulletphysics/bullet3/archive/0e893d4c56ba.tar.gz"],
  build_file = "BUILD.bullet",
  strip_prefix = "bullet3-0e893d4c56ba16039a103764bd7c815a8417572e",
)

http_archive(
  name = "flatbuffers",
  urls = ["https://github.com/google/flatbuffers/archive/b56d60f058cb.tar.gz"],
  build_file = "BUILD.flatbuffers",
  strip_prefix = "flatbuffers-b56d60f058cbe0c9b41d884857e977f38e9b0205",
)

http_archive(
  name = "flatui",
  urls = ["https://github.com/google/flatui/archive/2e363fa18228.tar.gz"],
  build_file = "BUILD.flatui",
  strip_prefix = "flatui-2e363fa18228664a06d8279ab07e514f3a3f51af",
)

http_archive(
  name = "fplbase",
  urls = ["https://github.com/google/fplbase/archive/57c832966594.tar.gz"],
  build_file = "BUILD.fplbase",
  strip_prefix = "fplbase-57c83296659469cf51db142baec3039edd347476",
)

http_archive(
  name = "fplutil",
  urls = ["https://github.com/google/fplutil/archive/45ca7eabda42.tar.gz"],
  build_file = "BUILD.fplutil",
  strip_prefix = "fplutil-45ca7eabda429ca209e5d3eb71bf5c81a42be568",
)

http_archive(
  name = "freetype2",
  url = "https://downloads.sourceforge.net/project/freetype/freetype2/2.8.1/freetype-2.8.1.tar.gz",
  build_file = "BUILD.freetype2",
)

http_archive(
   name = "gtest",
   urls = ["https://github.com/google/googletest/archive/master.zip"],
   strip_prefix = "googletest-master",
)

http_archive(
  name = "gumbo",
  urls = ["https://github.com/google/gumbo-parser/archive/aa91b27b02c0.tar.gz"],
  build_file = "BUILD.gumbo",
  strip_prefix = "gumbo-parser-aa91b27b02c0c80c482e24348a457ed7c3c088e0",
)

http_archive(
  name = "gvr",
  urls = ["https://github.com/googlevr/gvr-android-sdk/archive/3bd22d73.tar.gz"],
  build_file = "BUILD.gvr",
  strip_prefix = "gvr-android-sdk-3bd22d73c3d68b760b99957bb557c8ab6b1755b3",
)

http_archive(
  name = "harfbuzz",
  urls = ["https://github.com/behdad/harfbuzz/archive/60e2586f7652.tar.gz"],
  build_file = "BUILD.harfbuzz",
  strip_prefix = "harfbuzz-60e2586f7652aaa0ee908eb8f54b1498e2ad299e",
)

http_archive(
  name = "jsonnet",
  urls = ["https://github.com/google/jsonnet/archive/a3b9ec19d3cb.tar.gz"],
  strip_prefix = "jsonnet-a3b9ec19d3cba2a83d7aa4774a7ae63a6398d89f",
)

http_archive(
  name = "ktx",
  urls = ["https://github.com/KhronosGroup/KTX-Software/archive/0e6b5c7c2181.tar.gz"],
  build_file = "BUILD.ktx",
  strip_prefix = "KTX-Software-0e6b5c7c21818044da33da5f39534ea268fed181",
)

local_repository_env(
  name = "libfbx",
  env = "FBX_SDK_ROOT",
  build_file = FBX_BUILD_FILE_CONTENTS,
)

http_archive(
   name = "libjpeg_turbo",
   urls = ["https://github.com/libjpeg-turbo/libjpeg-turbo/archive/3041cf67ffdc.tar.gz"],
   build_file = "BUILD.libjpeg_turbo",
   strip_prefix = "libjpeg-turbo-3041cf67ffdc7230e377802cba0e5c325d6d01c6",
)

http_archive(
   name = "libpng",
   urls = ["https://github.com/glennrp/libpng/archive/4ddac468.tar.gz"],
   build_file = "BUILD.libpng",
   strip_prefix = "libpng-4ddac468c41ab68091e0efa353ec8cabf27beb89",
)

http_archive(
  name = "libunibreak",
  urls = ["https://github.com/adah1972/libunibreak/archive/0e8e32d.tar.gz"],
  build_file = "BUILD.libunibreak",
  strip_prefix = "libunibreak-0e8e32dcd0d8f9d2744026bef591b16af1ddd0ff",
)

http_archive(
  name = "libwebp",
  urls = ["https://github.com/webmproject/libwebp/archive/fb3daad.tar.gz"],
  build_file = "BUILD.libwebp",
  strip_prefix = "libwebp-fb3daad604ffbd807bf33dec93df9dbc728d1cbd",
)

http_archive(
  name = "mathfu",
  urls = ["https://github.com/google/mathfu/archive/9d8a26d1df90.tar.gz"],
  build_file = "BUILD.mathfu",
  strip_prefix = "mathfu-9d8a26d1df9083253605b0b64438ee7b2a4d00ab",
)

http_archive(
  name = "motive",
  urls = ["https://github.com/google/motive/archive/8e6e850de1f6.tar.gz"],
  build_file = "BUILD.motive",
  strip_prefix = "motive-8e6e850de1f645a690920eaaaa6e6e57ca934cc4",
)

http_archive(
  name = "rapidjson",
  urls = ["https://github.com/Tencent/rapidjson/archive/af223d44f4e8.tar.gz"],
  build_file = "BUILD.rapidjson",
  strip_prefix = "rapidjson-af223d44f4e8d3772cb1ac0ce8bc2a132b51717f",
)

http_archive(
  name = "sdl2",
  urls = ["https://www.libsdl.org/release/SDL2-2.0.6.zip"],
  build_file = "BUILD.sdl2",
  strip_prefix = "SDL2-2.0.6",
)

http_archive(
  name = "stb",
  urls = ["https://github.com/nothings/stb/archive/9d9f75e.tar.gz"],
  build_file = "BUILD.stb",
  strip_prefix = "stb-9d9f75eb682dd98b34de08bb5c489c6c561c9fa6",
)

http_archive(
  name = "tinygltf",
  urls = ["https://github.com/syoyo/tinygltf/archive/edf8d5c.tar.gz"],
  build_file = "@//external:BUILD.tinygltf",
  strip_prefix = "tinygltf-edf8d5cae1eb69376832da69b1e06e5f4daf3740",
)

http_archive(
  name = "vectorial",
  urls = ["https://github.com/scoopr/vectorial/archive/ae7dc88.tar.gz"],
  build_file = "BUILD.vectorial",
  strip_prefix = "vectorial-ae7dc88215e14c812786dc7a1f214610ae8540f5",
)

http_archive(
  name = "zlib",
  urls = ["https://github.com/madler/zlib/archive/cacf7f1.tar.gz"],
  build_file = "BUILD.zlib",
  strip_prefix = "zlib-cacf7f1d4e3d44d871b605da3b647f07d718623f",
)
