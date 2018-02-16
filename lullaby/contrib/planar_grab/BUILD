
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "planar_grab",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/input",
        "//lullaby/modules/input_processor",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/transform",
        "//lullaby/util:clock",
        "//lullaby/util:entity",
        "//lullaby/util:math",
        "//lullaby/util:optional",
        "//lullaby/util:time",
        "//lullaby/util:typeid",
        "@mathfu//:mathfu",
    ],
)
