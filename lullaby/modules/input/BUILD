
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
        "//lullaby/util:color",
        "//lullaby/util:common_types",
        "//lullaby/util:entity",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "//lullaby/util:optional",
        "//lullaby/util:registry",
        "//lullaby/util:time",
        "//lullaby/util:type_util",
        "//lullaby/util:typeid",
        "//lullaby/util:variant",
        "@mathfu//:mathfu",
    ],
)
