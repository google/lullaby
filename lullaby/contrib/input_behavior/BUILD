
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "input_behavior",
    srcs = [
        "input_behavior_system.cc",
    ],
    hdrs = [
        "input_behavior_system.h",
    ],
    deps = [
        "//:fbs",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/input",
        "//lullaby/modules/input_processor",
        "//lullaby/systems/collision",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/transform",
        "//lullaby/util:clock",
        "//lullaby/util:math",
        "@mathfu//:mathfu",
    ],
)
