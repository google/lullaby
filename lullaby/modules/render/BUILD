
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

config_setting(
    name = "disable_untrusted_loaders",
    values = {
        "define": "disable_untrusted_loaders=1",
    },
    visibility = ["//visibility:private"],
)

cc_library(
    name = "render",
    srcs = [
        "image_data.cc",
        "image_decode.cc",
        "image_util.cc",
        "mesh_data.cc",
        "mesh_util.cc",
        "nine_patch.cc",
        "render_view.cc",
        "sanitize_shader_source.cc",
        "vertex.cc",
        "vertex_format.cc",
    ],
    hdrs = [
        "image_data.h",
        "image_decode.h",
        "image_util.h",
        "material_info.h",
        "mesh_data.h",
        "mesh_util.h",
        "nine_patch.h",
        "render_view.h",
        "sanitize_shader_source.h",
        "texture_params.h",
        "vertex.h",
        "vertex_format.h",
    ],
    defines = select({
        ":disable_untrusted_loaders": [
            "LULLABY_DISABLE_STB_LOADERS=1",
            "LULLABY_DISABLE_WEBP_LOADER=1",
        ],
        "//conditions:default": [
            "LULLABY_DISABLE_STB_LOADERS=0",
            "LULLABY_DISABLE_WEBP_LOADER=0",
        ],
    }),
    deps = [
        "//:fbs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/input",
        "//lullaby/util:color",
        "//lullaby/util:data_container",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "//lullaby/util:optional",
        "//lullaby/util:registry",
        "//lullaby/util:span",
        "//lullaby/util:string_view",
        "//lullaby/util:variant",
        "@mathfu//:mathfu",
    ] + select({
        ":disable_untrusted_loaders": [],
        "//conditions:default": [
            "@libwebp//:webp_decode",
            "@stb//:stb",
        ],
    }),
)
