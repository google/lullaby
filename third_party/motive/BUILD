package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "motive_fbs",
    hdrs = [
        "anim_generated.h",
        "anim_list_generated.h",
        "anim_table_generated.h",
        "compact_spline_generated.h",
        "motive_generated.h",
        "spline_anim_generated.h",
    ],
    includes = [
        ".",
        "..",
    ],
    deps = [
        "@flatbuffers//:flatbuffers",
    ],
)
