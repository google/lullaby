
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
        "//lullaby/generated",
        "//lullaby/modules/ecs",
        "//lullaby/systems/render",
        "//lullaby/systems/transform",
        "//lullaby/util:math",
    ],
)
