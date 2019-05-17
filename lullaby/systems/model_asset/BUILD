
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "model_asset",
    srcs = [
        "model_asset.cc",
        "model_asset_system.cc",
    ],
    hdrs = [
        "model_asset.h",
        "model_asset_system.h",
    ],
    deps = [
        "//:fbs",
        "//lullaby/modules/ecs",
        "//lullaby/modules/file",
        "//lullaby/modules/render:image_data",
        "//lullaby/modules/render:image_decode",
        "//lullaby/modules/render:material_info",
        "//lullaby/modules/render:mesh",
        "//lullaby/modules/render:texture_params",
        "//lullaby/modules/render:vertex",
        "//lullaby/systems/blend_shape",
        "//lullaby/systems/collision",
        "//lullaby/systems/render",
        "//lullaby/systems/render:render_helpers",
        "//lullaby/systems/rig",
        "//lullaby/util:entity",
        "//lullaby/util:filename",
        "//lullaby/util:flatbuffer_reader",
        "//lullaby/util:make_unique",
        "//lullaby/util:registry",
        "//lullaby/util:resource_manager",
        "//lullaby/util:span",
        "//lullaby/util:string_view",
    ],
)
