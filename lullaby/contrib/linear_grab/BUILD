
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "linear_grab",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.h"]),
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/input",
        "//lullaby/modules/reticle",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/input_behavior",
        "//lullaby/systems/reticle",
        "//lullaby/systems/transform",
        "//lullaby/util:clock",
        "//lullaby/util:entity",
        "//lullaby/util:math",
        "//lullaby/util:optional",
        "//lullaby/util:typeid",
        "@mathfu//:mathfu",
    ],
)
