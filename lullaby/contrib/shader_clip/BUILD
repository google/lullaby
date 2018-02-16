
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "shader_clip",
    srcs = ["shader_clip_system.cc"],
    hdrs = ["shader_clip_system.h"],
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/systems/render",
        "//lullaby/systems/transform",
        "//lullaby/util:hash",
        "//lullaby/util:logging",
        "//lullaby/util:math",
    ],
)
