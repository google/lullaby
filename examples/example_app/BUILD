# Description:
#  Build rules for Lullaby's examples framework.
package(
    default_visibility = ["//visibility:public"],
)
licenses(["notice"])  # Apache 2.0
cc_library(
    name = "example_app",
    srcs = [
        "example_app.cc",
    ],
    hdrs = [
        "example_app.h",
    ],
    linkstatic = 1,
    deps = [
        "@fplbase//:fplbase",
        "@mathfu//:mathfu",
        "//lullaby/modules/ecs",
        "//lullaby/modules/render",
        "//lullaby/modules/script",
        "//lullaby/util:clock",
        "//lullaby/util:math",
        "//lullaby/util:span",
    ],
    alwayslink = 1,
)
cc_library(
    name = "sdl2_main",
    srcs = [
        "port/sdl2/main.cc",
        "port/sdl2/software_controller.cc",
        "port/sdl2/software_controller.h",
        "port/sdl2/software_hmd.cc",
        "port/sdl2/software_hmd.h",
    ],
    deps = [
        ":example_app",
        "@sdl2//:sdl2",
        "@fplbase//:fplbase",
        "@mathfu//:mathfu",
        "//lullaby/util:math",
    ],
    linkstatic = 1,
)