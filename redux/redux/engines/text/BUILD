# Text rasterization engine.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "text",
    srcs = [
        "font.cc",
        "internal/locale.cc",
        "internal/sdf_computer.cc",
        "internal/text_layout.cc",
        "text_engine.cc",
    ],
    hdrs = [
        "font.h",
        "internal/glyph.h",
        "internal/locale.h",
        "internal/sdf_computer.h",
        "internal/text_layout.h",
        "text_engine.h",
        "text_enums.h",
    ],
    deps = [
        "@absl//absl/types:span",
        "//redux/modules/base:asset_loader",
        "//redux/modules/base:data_builder",
        "//redux/modules/base:data_container",
        "//redux/modules/base:hash",
        "//redux/modules/base:logging",
        "//redux/modules/base:registry",
        "//redux/modules/base:resource_manager",
        "//redux/modules/base:static_registry",
        "//redux/modules/base:typeid",
        "//redux/modules/graphics:image_atlaser",
        "//redux/modules/graphics:image_data",
        "//redux/modules/graphics:mesh_data",
        "//redux/modules/math",
        "//redux/modules/math:vector",
    ],
)
