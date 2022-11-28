# Freetype2 based implementation for glyph rasterization.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "rasterizer",
    srcs = [
        "rasterizer.cc",
    ],
    deps = [
        "@freetype//:freetype",
        "//redux/engines/text",
        "//redux/modules/graphics:color",
        "//redux/modules/graphics:enums",
        "//redux/modules/graphics:image_data",
    ],
)
