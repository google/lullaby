
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "rig",
    srcs = ["rig_system.cc"],
    hdrs = ["rig_system.h"],
    deps = [
        "//lullaby/modules/ecs",
        "//lullaby/systems/render",
        "//lullaby/util:span",
        "@mathfu//:mathfu",
    ],
)
