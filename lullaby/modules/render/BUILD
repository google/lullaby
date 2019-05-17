
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

config_setting(
    name = "astc_cpu_decode",
    values = {
        "define": "astc_cpu_decode=1",
    },
    visibility = ["//visibility:public"],
)

# DEPRECATED: prefer depending on smaller targets that contain your dependency
# instead of this catch-all target.
cc_library(
    name = "render",
    hdrs = [
        "image_data.h",
        "image_decode.h",
        "image_decode_ktx.h",
        "image_util.h",
        "material_info.h",
        "mesh_data.h",
        "mesh_util.h",
        "nine_patch.h",
        "quad_util.h",
        "render_view.h",
        "sanitize_shader_source.h",
        "shader_description.h",
        "shader_snippets_selector.h",
        "texture_params.h",
        "vertex.h",
        "vertex_format.h",
        "vertex_format_util.h",
    ],
    defines = select({
        ":disable_untrusted_loaders": [
            "LULLABY_DISABLE_STB_LOADERS=1",
            "LULLABY_DISABLE_WEBP_LOADER=1",
            "LULLABY_DISABLE_ASTC_LOADERS=1",
        ],
        "//conditions:default": [
            "LULLABY_DISABLE_STB_LOADERS=0",
            "LULLABY_DISABLE_WEBP_LOADER=0",
            "LULLABY_DISABLE_ASTC_LOADERS=0",
        ],
    }) + select({
        ":astc_cpu_decode": ["LULLABY_ASTC_CPU_DECODE"],
        "//conditions:default": [],
    }),
    deps = [
        ":image_data",
        ":image_decode",
        ":material_info",
        ":mesh",
        ":mesh_util",
        ":nine_patch",
        ":render_view",
        ":shader",
        ":texture_params",
        ":quad_util",
        ":vertex",
        "@//third_party/gl",
        "@fplbase//:glplatform",
        "//:fbs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/input",
        "//lullaby/util:clock",
        "//lullaby/util:color",
        "//lullaby/util:data_container",
        "//lullaby/util:hash",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "//lullaby/util:optional",
        "//lullaby/util:registry",
        "//lullaby/util:span",
        "//lullaby/util:string_view",
        "//lullaby/util:time",
        "//lullaby/util:variant",
        "@mathfu//:mathfu",
    ] + select({
        ":disable_untrusted_loaders": [],
        "//conditions:default": [
            "@libwebp//:webp_decode",
            "@libwebp//:webp_demux",
            "@stb//:stb",
        ],
    }) + select({
        ":astc_cpu_decode": [":image_decode_astc"],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "image_data",
    srcs = [
        "image_data.cc",
    ],
    hdrs = [
        "image_data.h",
    ],
    deps = [
        "//lullaby/util:data_container",
        "//lullaby/util:logging",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "image_decode",
    srcs = [
        "image_decode.cc",
        "image_decode_ktx.cc",
        "image_util.cc",
    ],
    hdrs = [
        "image_decode.h",
        "image_decode_ktx.h",
        "image_util.h",
    ],
    defines = select({
        ":disable_untrusted_loaders": [
            "LULLABY_DISABLE_STB_LOADERS=1",
            "LULLABY_DISABLE_WEBP_LOADER=1",
            "LULLABY_DISABLE_ASTC_LOADERS=1",
        ],
        "//conditions:default": [
            "LULLABY_DISABLE_STB_LOADERS=0",
            "LULLABY_DISABLE_WEBP_LOADER=0",
            "LULLABY_DISABLE_ASTC_LOADERS=0",
        ],
    }) + select({
        ":astc_cpu_decode": ["LULLABY_ASTC_CPU_DECODE"],
        "//conditions:default": [],
    }),
    deps = [
        ":image_data",
        "@//third_party/gl",
        "//lullaby/util:clock",
        "//lullaby/util:color",
        "//lullaby/util:data_container",
        "//lullaby/util:logging",
        "//lullaby/util:time",
        "@mathfu//:mathfu",
    ] + select({
        ":disable_untrusted_loaders": [],
        "//conditions:default": [
            "@libwebp//:webp_decode",
            "@libwebp//:webp_demux",
            "@stb//:stb",
        ],
    }) + select({
        ":astc_cpu_decode": [":image_decode_astc"],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "image_decode_astc",
    srcs = select({
        ":disable_untrusted_loaders": [],
        "//conditions:default": ["image_decode_astc.cc"],
    }),
    hdrs = select({
        ":disable_untrusted_loaders": [],
        "//conditions:default": ["image_decode_astc.h"],
    }),
    deps = [
        ":image_data",
        "@astc_codec//:astc_codec",
        "//lullaby/util:data_container",
        "//lullaby/util:logging",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "material_info",
    hdrs = [
        "material_info.h",
    ],
    deps = [
        "//:fbs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/util:hash",
        "//lullaby/util:variant",
    ],
)

cc_library(
    name = "mesh",
    srcs = [
        "mesh_data.cc",
    ],
    hdrs = [
        "mesh_data.h",
    ],
    deps = [
        ":vertex",
        "//lullaby/util:data_container",
        "//lullaby/util:optional",
    ],
)

cc_library(
    name = "mesh_util",
    srcs = [
        "mesh_util.cc",
    ],
    hdrs = [
        "mesh_util.h",
    ],
    deps = [
        ":mesh",
        ":quad_util",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "nine_patch",
    srcs = [
        "nine_patch.cc",
    ],
    hdrs = [
        "nine_patch.h",
    ],
    deps = [
        ":mesh",
        ":vertex",
        "//:fbs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/util:logging",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "quad_util",
    srcs = [
        "quad_util.cc",
    ],
    hdrs = [
        "quad_util.h",
    ],
    deps = [
        ":mesh",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "render_view",
    srcs = [
        "render_view.cc",
    ],
    hdrs = [
        "render_view.h",
    ],
    deps = [
        "//lullaby/modules/input",
        "//lullaby/util:math",
        "//lullaby/util:registry",
        "//lullaby/util:span",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "shader",
    srcs = [
        "sanitize_shader_source.cc",
        "shader_snippets_selector.cc",
    ],
    hdrs = [
        "sanitize_shader_source.h",
        "shader_description.h",
        "shader_snippets_selector.h",
    ],
    deps = [
        "@fplbase//:glplatform",
        "//:fbs",
        "//lullaby/util:logging",
        "//lullaby/util:optional",
        "//lullaby/util:span",
        "//lullaby/util:string_view",
    ],
)

cc_library(
    name = "tangent_generation",
    srcs = [
        "tangent_generation.cc",
    ],
    hdrs = [
        "tangent_generation.h",
    ],
    deps = [
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "texture_params",
    hdrs = [
        "texture_params.h",
    ],
    deps = [
        ":image_data",
        "//:fbs",
    ],
)

cc_library(
    name = "vertex",
    srcs = [
        "vertex.cc",
        "vertex_format.cc",
        "vertex_format_util.cc",
    ],
    hdrs = [
        "vertex.h",
        "vertex_format.h",
        "vertex_format_util.h",
    ],
    deps = [
        "//:fbs",
        "//lullaby/util:color",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "//lullaby/util:optional",
        "@mathfu//:mathfu",
    ],
)
