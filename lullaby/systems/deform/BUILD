
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "deform",
    srcs = ["deform_system.cc"],
    hdrs = ["deform_system.h"],
    deps = [
        "//lullaby/generated",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/render",
        "//lullaby/systems/render",
        "//lullaby/systems/transform",
        "//lullaby/util:math",
        "//lullaby/util:optional",
        "//lullaby/util:string_view",
    ],
)
