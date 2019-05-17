
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "input",
    srcs = [
        "device_profile.cc",
        "input_manager.cc",
        "input_manager_util.cc",
    ],
    hdrs = [
        "device_profile.h",
        "input_focus.h",
        "input_manager.h",
        "input_manager_util.h",
    ],
    deps = [
        "//lullaby/util:bits",
        "//lullaby/util:clock",
        "//lullaby/util:entity",
        "//lullaby/util:hash",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "//lullaby/util:optional",
        "//lullaby/util:registry",
        "//lullaby/util:time",
        "//lullaby/util:typeid",
        "//lullaby/util:variant",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "input_binder",
    srcs = [
        "input_manager_binder.cc",
    ],
    hdrs = [
        "input_manager_binder.h",
    ],
    deps = [
        ":input",
        "//lullaby/modules/script",
        "//lullaby/util:device_util",
        "//lullaby/util:logging",
        "//lullaby/util:registry",
        "//lullaby/util:typeid",
    ],
)

cc_library(
    name = "input_jni",
    srcs = [
        "input_manager_jni.cc",
    ],
    deps = [
        ":input",
        ":input_binder",
        "//lullaby/modules/jni:registry_jni",
    ],
)
