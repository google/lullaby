
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "blend_shape",
    srcs = [
        "blend_shape_system.cc",
    ],
    hdrs = [
        "blend_shape_system.h",
    ],
    deps = [
        "//:fbs",
        "//lullaby/modules/ecs",
        "//lullaby/modules/file",
        "//lullaby/systems/render",
        "//lullaby/util:filename",
        "//lullaby/util:resource_manager",
        "//lullaby/util:string_view",
        "@mathfu//:mathfu",
    ],
)
