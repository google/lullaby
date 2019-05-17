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
        "//lullaby/events",
        "//lullaby/modules/camera",
        "//lullaby/modules/config",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/file",
        "//lullaby/modules/input",
        "//lullaby/modules/input_processor",
        "//lullaby/modules/render",
        "//lullaby/modules/script",
        "//lullaby/util:clock",
        "//lullaby/util:registry",
        "//lullaby/util:span",
        "@mathfu//:mathfu",
    ],
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
        "//lullaby/modules/camera",
        "//lullaby/modules/camera:mutable_camera",
        "//lullaby/modules/input",
        "//lullaby/util:device_util",
        "//lullaby/util:registry",
        "@mathfu//:mathfu",
        "//lullaby/util:math",
    ] + select({
        "//conditions:default": [
            "@//third_party/gl",
        ],
    }),
)

