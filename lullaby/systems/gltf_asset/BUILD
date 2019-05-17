
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "gltf_asset",
    srcs = [
        "gltf_asset.cc",
        "gltf_asset_system.cc",
    ],
    hdrs = [
        "gltf_asset.h",
        "gltf_asset_system.h",
    ],
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/animation_channels:skeleton_channel",
        "//lullaby/modules/ecs",
        "//lullaby/modules/file",
        "//lullaby/modules/render:image_data",
        "//lullaby/modules/render:material_info",
        "//lullaby/modules/render:mesh",
        "//lullaby/modules/render:tangent_generation",
        "//lullaby/modules/render:texture_params",
        "//lullaby/modules/render:vertex",
        "//lullaby/modules/tinygltf",
        "//lullaby/systems/animation",
        "//lullaby/systems/blend_shape",
        "//lullaby/systems/name",
        "//lullaby/systems/render",
        "//lullaby/systems/render:render_helpers",
        "//lullaby/systems/skin",
        "//lullaby/systems/transform",
        "//lullaby/util:entity",
        "//lullaby/util:filename",
        "//lullaby/util:flatbuffer_reader",
        "//lullaby/util:make_unique",
        "//lullaby/util:math",
        "//lullaby/util:optional",
        "//lullaby/util:registry",
        "//lullaby/util:resource_manager",
        "//lullaby/util:span",
        "//lullaby/util:string_view",
        "@mathfu//:mathfu",
        "@motive//:motive",
        "@tinygltf//:tinygltf",
    ],
)

cc_library(
    name = "gltf_asset_jni",
    srcs = [
        "gltf_asset_system_jni.cc",
    ],
    deps = [
        ":gltf_asset",
        "//lullaby/modules/ecs:ecs_jni",
    ],
)
