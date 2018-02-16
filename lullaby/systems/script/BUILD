
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "script",
    srcs = ["script_system.cc"],
    hdrs = ["script_system.h"],
    deps = [
        "//:fbs",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/file",
        "//lullaby/modules/script",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/transform",
        "//lullaby/util:clock",
        "//lullaby/util:entity",
        "//lullaby/util:hash",
    ],
)
