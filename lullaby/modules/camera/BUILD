
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "camera",
    srcs = [
        "camera.cc",
        "camera_manager.cc",
    ],
    hdrs = [
        "camera.h",
        "camera_manager.h",
    ],
    deps = [
        "//lullaby/modules/render:render_view",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "//lullaby/util:registry",
        "//lullaby/util:typeid",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "camera_binder",
    srcs = [
        "camera_manager_binder.cc",
    ],
    hdrs = [
        "camera_manager_binder.h",
    ],
    deps = [
        ":camera",
        ":mutable_camera",
        "//lullaby/modules/render:render_view",
        "//lullaby/modules/script",
        "//lullaby/systems/render",
        "//lullaby/util:hash",
        "//lullaby/util:logging",
        "//lullaby/util:registry",
        "//lullaby/util:typeid",
    ],
)

cc_library(
    name = "camera_jni",
    srcs = [
        "camera_manager_jni.cc",
    ],
    deps = [
        ":camera",
        ":camera_binder",
        "//lullaby/modules/jni:registry_jni",
    ],
)

cc_library(
    name = "mutable_camera",
    srcs = [
        "mutable_camera.cc",
    ],
    hdrs = [
        "mutable_camera.h",
    ],
    deps = [
        "//lullaby/modules/camera",
    ],
)
