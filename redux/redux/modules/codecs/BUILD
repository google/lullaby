# Encoding/decoding functions between common data types and redux data representations.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "decode_image",
    srcs = ["decode_image.cc"],
    hdrs = ["decode_image.h"],
    deps = [
        ":decode_stb",
        ":decode_webp",
        "//redux/modules/base:data_container",
        "//redux/modules/graphics:image_data",
        "//redux/modules/graphics:image_utils",
    ],
)

cc_library(
    name = "decode_stb",
    srcs = ["decode_stb.cc"],
    hdrs = ["decode_stb.h"],
    deps = [
        "//redux/modules/base:data_container",
        "//redux/modules/graphics:image_data",
        "@stblib//:stblib",
    ],
)

cc_library(
    name = "decode_webp",
    srcs = ["decode_webp.cc"],
    hdrs = ["decode_webp.h"],
    deps = [
        "@libwebp//:webp_decode",
        "@libwebp//:webp_demux",
        "//redux/modules/base:data_container",
        "//redux/modules/graphics:image_data",
    ],
)

cc_library(
    name = "encode_png",
    srcs = ["encode_png.cc"],
    hdrs = ["encode_png.h"],
    deps = [
        "//redux/modules/base:data_builder",
        "//redux/modules/base:data_container",
        "//redux/modules/graphics:image_data",
        "//redux/modules/graphics:image_utils",
        "@libpng//:libpng",
    ],
)

cc_library(
    name = "encode_webp",
    srcs = ["encode_webp.cc"],
    hdrs = ["encode_webp.h"],
    deps = [
        "@libwebp//:webp_encode",
        "//redux/modules/base:data_builder",
        "//redux/modules/base:data_container",
        "//redux/modules/graphics:image_data",
        "//redux/modules/graphics:image_utils",
    ],
)
