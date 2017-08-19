package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "fplbase_fbs",
    hdrs = [
        "common_generated.h",
        "materials_generated.h",
        "mesh_generated.h",
        "shader_generated.h",
        "texture_atlas_generated.h",
    ],
    includes = [
        ".",
        "..",
    ],
    deps = [
        "@flatbuffers//:flatbuffers",
    ],
)
