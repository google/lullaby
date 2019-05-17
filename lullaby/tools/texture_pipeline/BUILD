# Pipeline tool for building texture files.

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "encode_astc",
    srcs = [
        "encode_astc.cc",
    ],
    hdrs = [
        "encode_astc.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//lullaby/modules/render",
        "//lullaby/util:common_types",
        "//lullaby/util:logging",
    ],
)

cc_library(
    name = "encode_jpg",
    srcs = [
        "encode_jpg.cc",
    ],
    hdrs = [
        "encode_jpg.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@libjpeg_turbo//:jpeg",
        "//lullaby/modules/render",
        "//lullaby/util:common_types",
        "//lullaby/util:logging",
    ],
)

cc_library(
    name = "encode_ktx",
    srcs = [
        "encode_ktx.cc",
    ],
    hdrs = [
        "encode_ktx.h",
        "encode_texture.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//lullaby/modules/render",
        "//lullaby/util:common_types",
        "//lullaby/util:logging",
        "//lullaby/tools/pack_ktx:pack_ktx_lib",
    ],
)

cc_library(
    name = "encode_png",
    srcs = [
        "encode_png.cc",
    ],
    hdrs = [
        "encode_png.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//lullaby/modules/render",
        "//lullaby/util:common_types",
        "//lullaby/util:logging",
        "@libpng//:libpng",
    ],
)

cc_library(
    name = "encode_webp",
    srcs = [
        "encode_webp.cc",
    ],
    hdrs = [
        "encode_webp.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@libwebp//:webp_encode",
        "//lullaby/modules/render",
        "//lullaby/util:common_types",
        "//lullaby/util:logging",
    ],
)

cc_library(
    name = "mipmap_generator",
    srcs = [
        "mipmap_generator.cc",
    ],
    hdrs = [
        "mipmap_generator.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//lullaby/modules/render",
        "//lullaby/util:common_types",
        "//lullaby/util:logging",
    ],
)


cc_binary(
    name = "texture_pipeline",
    srcs = [
        "main.cc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":encode_astc",
        ":encode_jpg",
        ":encode_ktx",
        ":encode_png",
        ":encode_webp",
        ":mipmap_generator",
        "//lullaby/modules/render",
        "//lullaby/util:arg_parser",
        "//lullaby/util:common_types",
        "//lullaby/util:filename",
        "//lullaby/util:logging",
        "//lullaby/tools/common:file_utils",
    ],
)
