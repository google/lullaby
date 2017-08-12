
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "input",
    srcs = [
        "input_manager.cc",
        "input_manager_util.cc",
        "input_processor.cc",
    ],
    hdrs = [
        "input_manager.h",
        "input_manager_util.h",
        "input_processor.h",
    ],
    deps = [
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/util:bits",
        "//lullaby/util:clock",
        "//lullaby/util:dependency_checker",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "//lullaby/util:registry",
        "//lullaby/util:time",
        "//lullaby/util:typeid",
        "@mathfu//:mathfu",
    ],
)
