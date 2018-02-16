
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
        "//lullaby/modules/render",
        "//lullaby/systems/render",
        "//lullaby/systems/rig",
        "//lullaby/util:entity",
        "//lullaby/util:filename",
        "//lullaby/util:make_unique",
        "//lullaby/util:registry",
        "//lullaby/util:resource_manager",
        "//lullaby/util:string_view",
    ],
)
