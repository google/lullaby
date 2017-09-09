
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "stategraph",
    srcs = [
        "stategraph_asset.cc",
        "stategraph_system.cc",
    ],
    hdrs = [
        "stategraph_asset.h",
        "stategraph_system.h",
    ],
    deps = [
        "//lullaby/generated",
        "//lullaby/events",
        "//lullaby/modules/ecs",
        "//lullaby/modules/ecs:entity",
        "//lullaby/modules/file",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/script",
        "//lullaby/modules/script/lull",
        "//lullaby/modules/stategraph",
        "//lullaby/systems/animation",
        "//lullaby/util:clock",
        "//lullaby/util:make_unique",
        "//lullaby/util:random_number_generator",
        "//lullaby/util:registry",
        "//lullaby/util:resource_manager",
        "//lullaby/util:selector",
    ],
)
