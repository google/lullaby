
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "light",
    srcs = ["light_system.cc"],
    hdrs = [
        "light_system.h",
    ],
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/ecs",
        "//lullaby/modules/render:render_view",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/render",
        "//lullaby/systems/transform",
        "//lullaby/util:math",
        "//lullaby/util:optional",
        "@mathfu//:mathfu",
    ],
)
