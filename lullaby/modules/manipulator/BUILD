
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "manipulator",
    srcs = [
        "manipulator_manager.cc",
        "rotation_manipulator.cc",
        "scaling_manipulator.cc",
        "translation_manipulator.cc",
    ],
    hdrs = [
        "manipulator.h",
        "manipulator_manager.h",
        "rotation_manipulator.h",
        "scaling_manipulator.h",
        "translation_manipulator.h",
    ],
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/debug",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/input",
        "//lullaby/modules/input_processor",
        "//lullaby/modules/render:mesh",
        "//lullaby/modules/render:mesh_util",
        "//lullaby/modules/render:render_view",
        "//lullaby/modules/reticle",
        "//lullaby/systems/collision",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/render",
        "//lullaby/systems/transform",
        "//lullaby/util:entity",
        "//lullaby/util:make_unique",
        "//lullaby/util:math",
        "//lullaby/util:registry",
        "@mathfu//:mathfu",
    ],
)
