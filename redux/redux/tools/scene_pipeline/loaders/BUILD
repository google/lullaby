# Tool for building redux scene files.

licenses(["notice"])

package(
    
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "import_options",
    hdrs = ["import_options.h"],
    deps = [
        "//redux/tools/scene_pipeline:buffer",
        "//redux/tools/scene_pipeline:image",
    ],
)

cc_library(
    name = "import_utils",
    srcs = ["import_utils.cc"],
    hdrs = ["import_utils.h"],
    deps = [
        ":import_options",
        "@absl//absl/log:check",
        "//redux/tools/scene_pipeline:buffer",
        "//redux/tools/scene_pipeline:image",
        "//redux/tools/scene_pipeline:scene",
    ],
)

cc_library(
    name = "gltf",
    srcs = [
        "gltf.cc",
        "gltf_utils.cc",
    ],
    hdrs = [
        "gltf.h",
        "gltf_utils.h",
    ],
    deps = [
        ":import_options",
        ":import_utils",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/log",
        "@absl//absl/log:check",
        "@absl//absl/types:span",
        "//third_party/cgltf",
        "//redux/tools/scene_pipeline:buffer",
        "//redux/tools/scene_pipeline:drawable",
        "//redux/tools/scene_pipeline:image",
        "//redux/tools/scene_pipeline:index_buffer",
        "//redux/tools/scene_pipeline:material",
        "//redux/tools/scene_pipeline:model",
        "//redux/tools/scene_pipeline:sampler",
        "//redux/tools/scene_pipeline:scene",
        "//redux/tools/scene_pipeline:type_id",
        "//redux/tools/scene_pipeline:types",
        "//redux/tools/scene_pipeline:vertex_buffer",
    ],
)

cc_library(
    name = "stb_decode_image",
    srcs = ["stb_decode_image.cc"],
    hdrs = ["stb_decode_image.h"],
    deps = [
        ":import_options",
        "@absl//absl/log:check",
        "//redux/tools/scene_pipeline:buffer",
        "//redux/tools/scene_pipeline:buffer_view",
        "@stblib//:stblib",
    ],
)

cc_library(
    name = "std_load_file",
    srcs = ["std_load_file.cc"],
    hdrs = ["std_load_file.h"],
    deps = [
        "@absl//absl/log:check",
        "//redux/tools/scene_pipeline:buffer",
    ],
)

cc_library(
    name = "usd",
    srcs = [
        "usd.cc",
        "usd_utils.cc",
    ],
    hdrs = [
        "usd.h",
        "usd_utils.h",
    ],
    deps = [
        ":import_options",
        ":import_utils",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/log",
        "@absl//absl/log:check",
        "//redux/tools/scene_pipeline:buffer",
        "//redux/tools/scene_pipeline:buffer_view",
        "//redux/tools/scene_pipeline:collider",
        "//redux/tools/scene_pipeline:drawable",
        "//redux/tools/scene_pipeline:image",
        "//redux/tools/scene_pipeline:index_buffer",
        "//redux/tools/scene_pipeline:material",
        "//redux/tools/scene_pipeline:model",
        "//redux/tools/scene_pipeline:sampler",
        "//redux/tools/scene_pipeline:scene",
        "//redux/tools/scene_pipeline:type_id",
        "//redux/tools/scene_pipeline:types",
        "//redux/tools/scene_pipeline:vertex_buffer",
        "@usd//:ar",
        "@usd//:gf",
        "@usd//:sdf",
        "@usd//:tf",
        "@usd//:usd",
        "@usd//:usdGeom",
        "@usd//:usdPhysics",
        "@usd//:usdShade",
        "@usd//:vt",
    ],
)
