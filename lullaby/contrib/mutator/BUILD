
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "mutator",
    srcs = [
        "mutator_system.cc",
    ],
    hdrs = [
        "mutator_system.h",
    ],
    deps = [
        "//lullaby/modules/ecs",
        "//lullaby/util:math",
    ],
)

cc_library(
    name = "stay_in_box_mutator",
    srcs = [
        "stay_in_box_mutator_system.cc",
    ],
    hdrs = [
        "stay_in_box_mutator_system.h",
    ],
    deps = [
        ":mutator",
        "//:fbs",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/systems/transform",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "face_point_mutator",
    srcs = [
        "face_point_mutator_system.cc",
    ],
    hdrs = [
        "face_point_mutator_system.h",
    ],
    deps = [
        ":mutator",
        "//:fbs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/input",
        "//lullaby/systems/transform",
        "@mathfu//:mathfu",
    ],
)
