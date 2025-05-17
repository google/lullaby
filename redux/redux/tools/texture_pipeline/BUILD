# Tool for building texture files.

licenses(["notice"])

package(
    
    default_visibility = ["//redux:visibility"],
)

cc_binary(
    name = "texture_pipeline",
    srcs = ["main.cc"],
    deps = [
        ":generate_mipmaps",
        "@absl//absl/flags:flag",
        "@absl//absl/flags:parse",
        "@absl//absl/log",
        "@absl//absl/log:check",
        "@absl//absl/status:statusor",
        "//redux/modules/base:filepath",
        "//redux/modules/codecs:decode_image",
        "//redux/modules/codecs:encode_png",
        "//redux/modules/codecs:encode_webp",
        "//redux/tools/common:file_utils",
    ],
)

cc_library(
    name = "generate_mipmaps",
    srcs = ["generate_mipmaps.cc"],
    hdrs = ["generate_mipmaps.h"],
    deps = [
        "@absl//absl/log:check",
        "//redux/modules/base:logging",
        "//redux/modules/graphics:image_data",
        "//redux/modules/graphics:image_utils",
    ],
)
