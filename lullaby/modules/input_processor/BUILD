
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "input_processor",
    srcs = [
        "gesture.cc",
        "input_processor.cc",
    ],
    hdrs = [
        "gesture.h",
        "input_processor.h",
    ],
    deps = [
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/input",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/transform",
        "//lullaby/util:bits",
        "//lullaby/util:clock",
        "//lullaby/util:entity",
        "//lullaby/util:enum_hash",
        "//lullaby/util:math",
        "//lullaby/util:registry",
        "//lullaby/util:span",
        "//lullaby/util:string_view",
        "//lullaby/util:variant",
    ],
)

cc_library(
    name = "input_processor_jni",
    srcs = [
        "input_processor_jni.cc",
    ],
    deps = [
        ":input_processor",
        "//lullaby/modules/jni:registry_jni",
    ],
)

cc_library(
    name = "touchscreen_gestures",
    srcs = [
        "touchscreen_gestures.cc",
    ],
    hdrs = [
        "touchscreen_gestures.h",
    ],
    deps = [
        ":input_processor",
        "//lullaby/events",
        "//lullaby/modules/camera",
        "//lullaby/modules/input",
        "//lullaby/util:bits",
        "//lullaby/util:clock",
        "//lullaby/util:entity",
        "//lullaby/util:math",
        "//lullaby/util:registry",
    ],
)
