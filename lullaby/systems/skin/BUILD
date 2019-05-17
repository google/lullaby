
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "skin",
    srcs = ["skin_system.cc"],
    hdrs = ["skin_system.h"],
    deps = [
        "//lullaby/modules/ecs",
        "//lullaby/modules/script",
        "//lullaby/systems/render",
        "//lullaby/systems/transform",
        "//lullaby/util:math",
        "//lullaby/util:span",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "skin_jni",
    srcs = [
        "skin_system_jni.cc",
    ],
    deps = [
        ":skin",
        "//lullaby/modules/ecs:ecs_jni",
    ],
)
