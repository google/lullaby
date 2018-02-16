
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "slider",
    srcs = ["slider_system.cc"],
    hdrs = ["slider_system.h"],
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/animation_channels:transform_channels",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/input",
        "//lullaby/modules/input_processor",
        "//lullaby/systems/collision",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/render",
        "//lullaby/systems/transform",
        "//lullaby/util:clock",
        "//lullaby/util:math",
        "//lullaby/util:time",
        "//lullaby/util:trace",
    ],
)
