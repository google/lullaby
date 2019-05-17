
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

exports_files(["LICENSE"])

common_headers = [
    "animated_texture_processor.h",
    "mesh.h",
    "mesh_factory.h",
    # TODO remove this, make other apps include ":render_stats" instead
    "render_stats.h",
    "render_system.h",
    "render_system_deprecated.inc",
    "render_target.h",
    "render_types.h",
    "shader.h",
    "simple_font.h",
    "texture.h",
    "texture_factory.h",
]

private_headers = [
    "detail/display_list.h",
    "detail/render_pool.h",
    "detail/render_pool_map.h",
    "detail/sort_order.h",
    "detail/sort_order_types.h",
    "detail/uniform_data.h",
]

fpl_headers = [
    "fpl/material.h",
    "fpl/mesh.h",
    "fpl/render_component.h",
    "fpl/render_factory.h",
    "fpl/render_system_fpl.h",
    "fpl/shader.h",
    "fpl/texture.h",
    "fpl/uniform.h",
]

common_deps = [
    "@fplbase//:render_state",
    "//lullaby/modules/config",
    "//lullaby/modules/ecs",
    "//lullaby/modules/file:asset",
    "//lullaby/modules/render:image_data",
    "//lullaby/modules/render:image_decode",
    "//lullaby/modules/render:material_info",
    "//lullaby/modules/render:mesh",
    "//lullaby/modules/render:quad_util",
    "//lullaby/modules/render:render_view",
    "//lullaby/modules/render:shader",
    "//lullaby/modules/render:texture_params",
    "//lullaby/modules/render:vertex",
    "//lullaby/systems/transform",
    "//lullaby/util:entity",
    "//lullaby/util:android_context",
    "//lullaby/util:async_processor",
    "//lullaby/util:bits",
    "//lullaby/util:clock",
    "//lullaby/util:color",
    "//lullaby/util:hash",
    "//lullaby/util:logging",
    "//lullaby/util:make_unique",
    "//lullaby/util:math",
    "//lullaby/util:registry",
    "//lullaby/util:optional",
    "//lullaby/util:string_preprocessor",
    "//lullaby/util:string_view",
    "//lullaby/util:trace",
    "//lullaby/util:typeid",
    "//:fbs",
    "@mathfu//:mathfu",
]

## Main RenderSystem targets

# 'render_system' represents only the interface to a render system, not an
# implementation.  Use this if you just need to access the generic
# RenderSystem API.  Apps will need to depend on an implementation target,
# such as 'fpl'.
cc_library(
    name = "render",
    srcs = [
        "animated_texture_processor.cc",
        "simple_font.cc",
        "texture_factory.cc",
    ],
    hdrs = common_headers + private_headers,
    deps = common_deps,
)


# 'fpl_render_system' provides the RenderSystemFpl and associated classes,
#  but does not bind it to the RenderSystem API.
cc_library(
    name = "fpl_render_system",
    srcs = [
        "fpl/material.cc",
        "fpl/mesh.cc",
        "fpl/render_factory.cc",
        "fpl/render_system_fpl.cc",
        "fpl/shader.cc",
        "fpl/texture.cc",
        "fpl/uniform.cc",
    ],
    hdrs = common_headers + private_headers + fpl_headers,
    copts = [
        "-Ithird_party/GL/gl/include/",
    ],
    deps = common_deps + [
        ":profiler",
        ":render_helpers",
        ":render_stats",
        ":sort_order",
        "@fplbase//:fplbase_fbs",
        "@fplbase//:fplbase",
        "@fplbase//:glplatform",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/file",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/script",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/text",
        "//lullaby/util:filename",
        "//lullaby/util:resource_manager",
    ],
)

cc_library(
    name = "fpl",
    srcs = [
        "fpl/render_system_impl.cc",
    ],
    hdrs = common_headers + private_headers + fpl_headers,
    defines = ["LULLABY_RENDER_BACKEND_FPL"],
    deps = common_deps + [
        ":binding_impl",
        ":fpl_render_system",
        "@fplbase//:fplbase_fbs",
        "@fplbase//:fplbase",
        "@fplbase//:glplatform",
        "//lullaby/events",
        "//lullaby/modules/file",
        "//lullaby/systems/text",
        "//lullaby/util:filename",
        "//lullaby/util:resource_manager",
    ],
    alwayslink = 1,
)

NEXT_RENDERER_SRCS = [
    "next/gl_helpers.cc",
    "next/material.cc",
    "next/mesh.cc",
    "next/mesh_factory.cc",
    "next/next_renderer.cc",
    "next/render_state.cc",
    "next/render_state_manager.cc",
    "next/render_system_impl.cc",
    "next/render_system_next.cc",
    "next/render_target.cc",
    "next/shader.cc",
    "next/shader_data.cc",
    "next/shader_factory.cc",
    "next/texture.cc",
    "next/texture_atlas.cc",
    "next/texture_factory.cc",
]

NEXT_RENDERER_HEADERS = common_headers + private_headers + [
    "next/detail/glplatform.h",
    "next/gl_helpers.h",
    "next/material.h",
    "next/mesh.h",
    "next/mesh_factory.h",
    "next/next_renderer.h",
    "next/render_component.h",
    "next/render_handle.h",
    "next/render_state.h",
    "next/render_state_manager.h",
    "next/render_system_next.h",
    "next/render_target.h",
    "next/shader.h",
    "next/shader_data.h",
    "next/shader_factory.h",
    "next/texture.h",
    "next/texture_atlas.h",
    "next/texture_factory.h",
]

NEXT_RENDERER_DEPS = common_deps + [
    ":binding_impl",
    ":profiler",
    ":render",
    ":render_helpers",
    ":render_stats",
    ":sort_order",
    ":uniform_data",
    "@fplbase//:fplbase_fbs",
    "@fplbase//:glplatform",
    "//lullaby/events",
    "//lullaby/modules/dispatcher",
    "//lullaby/modules/file",
    "//lullaby/modules/flatbuffers",
    "//lullaby/modules/script",
    "//lullaby/systems/dispatcher",
    "//lullaby/util:flatbuffer_reader",
    "//lullaby/util:buffered_data",
    "//lullaby/util:enum_hash",
    "//lullaby/util:filename",
    "//lullaby/util:fixed_string",
    "//lullaby/util:resource_manager",
    "//lullaby/util:span",
    "//lullaby/util:time",
    "//lullaby/util:unordered_vector_map",
    "//lullaby/util:variant",
]

NEXT_RENDERER_DEFINES = [
    "LULLABY_RENDER_BACKEND_NEXT",
] + select({
    "//:target_os_windows": [],
    "//conditions:default": ["GL_GLEXT_PROTOTYPES"],
})

NEXT_RENDERER_COPTS = [
    "-Ithird_party/GL/gl/include/",
    "-UNOGDI",
]

cc_library(
    name = "next",
    srcs = NEXT_RENDERER_SRCS,
    hdrs = NEXT_RENDERER_HEADERS,
    copts = NEXT_RENDERER_COPTS,
    defines = NEXT_RENDERER_DEFINES,
    linkopts = select({
        "//:target_os_android": [
            "-lGLESv3 -lGLESv2 -lGLESv1_CM -lEGL -landroid",
        ],
        "//conditions:default": [],
    }),
    deps = NEXT_RENDERER_DEPS + select({
        "//:target_os_ios": [":next_ios"],
        "//conditions:default": [],
    }),
    alwayslink = 1,
)


cc_library(
    name = "render_system_mock",
    testonly = 1,
    srcs = [
        "testing/mesh.cc",
        "testing/mock_render_system_impl.cc",
        "testing/texture.cc",
    ],
    hdrs = [
        "testing/mock_render_system_impl.h",
        "testing/texture.h",
        "testing/mesh.h",
    ] + common_headers + private_headers,
    defines = ["LULLABY_RENDER_BACKEND_MOCK"],
    deps = common_deps + [
        ":binding_impl",
        ":render",
        ":render_helpers",
        "@gtest//:gtest",
    ],
    alwayslink = 1,
)


## Utilities and dependencies for RenderSystems

cc_library(
    name = "async_render_object",
    srcs = [
        "next/detail/async_render_object.cc",
        "next/detail/async_render_object_factory.cc",
    ],
    hdrs = [
        "next/detail/async_render_object.h",
        "next/detail/async_render_object_factory.h",
        "next/detail/render_asset.h",
    ],
    deps = [
        "//lullaby/modules/file",
        "//lullaby/util:thread_safe_deque",
        "//lullaby/util:thread_safe_queue",
    ],
)

cc_library(
    name = "binding_impl",
    hdrs = ["detail/render_system.inc"],
    deps = [
        ":render",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/script",
        "//lullaby/systems/transform",
    ],
)

cc_library(
    name = "custom_render_pass",
    srcs = ["custom_render_pass_manager.cc"],
    hdrs = ["custom_render_pass_manager.h"],
    deps = [
        "@fplbase//:glplatform",
        "//lullaby/events",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/render",
        "//lullaby/systems/transform",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "image_texture",
    srcs = ["image_texture.cc"],
    hdrs = ["image_texture.h"],
    deps = [
        ":render",
        "//lullaby/modules/render:image_data",
        "//lullaby/util:registry",
    ],
)

cc_library(
    name = "profiler",
    srcs = [
        "detail/profiler.cc",
    ] + select({
        "//conditions:default": ["detail/port/default/gpu_profiler.cc"],
    }),
    hdrs = [
        "detail/gpu_profiler.h",
        "detail/profiler.h",
    ],
    deps = [
        ":render",
        "//lullaby/util:clock",
        "//lullaby/util:logging",
        "//lullaby/util:time",
        "//lullaby/util:typeid",
    ] + select({
        "//conditions:default": [],
    }),
)


cc_library(
    name = "render_helpers",
    srcs = ["render_helpers.cc"],
    hdrs = ["render_helpers.h"],
    deps = [
        ":render",
        "//lullaby/systems/transform",
        "//lullaby/util:entity",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "render_jni",
    srcs = select({
        "//:target_os_android": [
            "render_system_jni.cc",
        ],
        "//conditions:default": [],
    }),
    deps = [
        ":render",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs:ecs_jni",
        "//lullaby/modules/jni:jni_util",
        "//lullaby/modules/jni:registry_jni",
    ],
)

cc_library(
    name = "render_stats",
    srcs = ["detail/render_stats.cc"],
    hdrs = ["render_stats.h"],
    deps = [
        ":profiler",
        ":render",
        "//lullaby/util:math",
        "//lullaby/util:registry",
        "@mathfu//:mathfu",
    ],
)


cc_library(
    name = "shader_uniform_initializer",
    srcs = [
        "shader_uniform_initializer.cc",
    ],
    hdrs = [
        "shader_uniform_initializer.h",
    ],
    linkstatic = 1,
    deps = [
        ":render",
        "//lullaby/util:entity",
        "//lullaby/util:registry",
    ],
)

cc_library(
    name = "sort_order",
    srcs = ["detail/sort_order.cc"],
    hdrs = [
        "detail/sort_order.h",
        "detail/sort_order_types.h",
    ],
    deps = [
        "//lullaby/systems/transform",
        "//lullaby/util:entity",
        "//lullaby/util:logging",
        "//lullaby/util:registry",
    ],
)

cc_library(
    name = "uniform_data",
    srcs = ["detail/uniform_data.cc"],
    hdrs = ["detail/uniform_data.h"],
    deps = [
        "//:fbs",
        "//lullaby/util:logging",
    ],
)
