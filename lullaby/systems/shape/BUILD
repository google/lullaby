
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "shape",
    srcs = ["shape_system.cc"],
    hdrs = ["shape_system.h"],
    deps = [
        "//:fbs",
        "//lullaby/modules/ecs",
        "//lullaby/modules/render:mesh",
        "//lullaby/modules/render:mesh_util",
        "//lullaby/systems/render",
    ],
)

cc_library(
    name = "shape_jni",
    srcs = ["shape_system_jni.cc"],
    deps = [
        ":shape",
        "//lullaby/modules/ecs:ecs_jni",
    ],
)
