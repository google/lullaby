
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

common_headers = [
    "material.h",
    "mesh.h",
    "render_stats.h",
    "render_system.h",
    "shader.h",
    "simple_font.h",
    "texture.h",
    "uniform.h",
]

private_headers = [
    "detail/display_list.h",
    "detail/render_pool.h",
    "detail/render_pool_map.h",
    "detail/sort_order.h",
]

fpl_headers = [
    "fpl/mesh.h",
    "fpl/pano.h",
    "fpl/pano_model.h",
    "fpl/pano_viewer.h",
    "fpl/render_component.h",
    "fpl/render_factory.h",
    "fpl/render_system_fpl.h",
    "fpl/shader.h",
    "fpl/texture.h",
]

common_deps = [
    "@fplbase//:fplbase",
    "//lullaby/modules/config",
    "//lullaby/modules/ecs",
    "//lullaby/modules/ecs:entity",
    "//lullaby/modules/render",
    "//lullaby/systems/text",
    "//lullaby/systems/transform",
    "//lullaby/util:hash",
    "//lullaby/util:logging",
    "//lullaby/util:make_unique",
    "//lullaby/util:math",
    "//lullaby/util:registry",
    "//lullaby/util:string_preprocessor",
    "//lullaby/util:trace",
    "//lullaby/generated",
    "@mathfu//:mathfu",
]

# 'render_system' represents only the interface to a render system, not an
# implementation.  Use this if you just need to access the generic
# RenderSystem API.  Apps will need to depend on an implementation target,
# such as 'fpl'.
cc_library(
    name = "render",
    srcs = [
        "material.cc",
        "simple_font.cc",
        "uniform.cc",
    ],
    hdrs = common_headers,
    deps = common_deps,
)


# 'fpl_render_system' provides the RenderSystemFpl and associated classes,
#  but does not bind it to the RenderSystem API.
cc_library(
    name = "fpl_render_system",
    srcs = [
        "detail/render_stats.cc",
        "detail/sort_order.cc",
        "fpl/mesh.cc",
        "fpl/render_factory.cc",
        "fpl/render_system_fpl.cc",
        "fpl/shader.cc",
        "fpl/texture.cc",
    ],
    hdrs = common_headers + private_headers + fpl_headers,
    copts = [
        "-Ithird_party/GL/gl/include/",
    ],
    deps = common_deps + [
        ":profiler",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/file",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/script",
        "//lullaby/systems/dispatcher",
        "//lullaby/util:async_processor",
        "//lullaby/util:resource_manager",
        "//lullaby/util:typeid",
    ],
)

cc_library(
    name = "binding_impl",
    hdrs = ["detail/render_system.inc"],
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
        "//lullaby/events",
        "//lullaby/modules/file",
        "//lullaby/util:async_processor",
        "//lullaby/util:resource_manager",
        "//lullaby/util:typeid",
    ],
    alwayslink = 1,
)

cc_library(
    name = "next",
    srcs = [
        "detail/render_stats.cc",
        "detail/sort_order.cc",
        "next/mesh.cc",
        "next/render_factory.cc",
        "next/render_state.cc",
        "next/render_system_impl.cc",
        "next/render_system_next.cc",
        "next/shader.cc",
        "next/texture.cc",
    ],
    hdrs = common_headers + private_headers + [
        "next/mesh.h",
        "next/render_component.h",
        "next/render_factory.h",
        "next/render_state.h",
        "next/render_system_next.h",
        "next/shader.h",
        "next/texture.h",
    ],
    copts = [
        "-Ithird_party/GL/gl/include/",
    ],
    defines = ["LULLABY_RENDER_BACKEND_NEXT"],
    deps = common_deps + [
        ":binding_impl",
        ":profiler",
        ":render",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/file",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/script",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/rig",
        "//lullaby/util:async_processor",
        "//lullaby/util:buffered_data",
        "//lullaby/util:resource_manager",
        "//lullaby/util:string_view",
        "//lullaby/util:time",
        "//lullaby/util:typeid",
        "//lullaby/util:unordered_vector_map",
    ],
    alwayslink = 1,
)

cc_library(
    name = "render_system_mock",
    testonly = 1,
    srcs = [
        "detail/port/default/gpu_profiler.cc",
        "detail/render_stats.cc",
        "detail/sort_order.cc",
        "testing/mock_render_system_impl.cc",
    ],
    hdrs = [
        "testing/mock_render_system_impl.h",
        "testing/texture.h",
    ] + common_headers + private_headers,
    defines = ["LULLABY_RENDER_BACKEND_MOCK"],
    deps = common_deps + [
        ":binding_impl",
        ":profiler",
        ":render",
        "//lullaby/util:time",
    ],
    alwayslink = 1,
)

cc_library(
    name = "render_helpers",
    srcs = ["render_helpers.cc"],
    hdrs = ["render_helpers.h"],
    deps = [
        ":render",
        "//lullaby/modules/ecs:entity",
        "//lullaby/systems/transform",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "profiler",
    srcs = [
        "detail/profiler.cc",
    ] + select({
        "//tools/cc_target_os:android": ["detail/port/android/gpu_profiler.cc"],
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
        "//tools/cc_target_os:android": ["@GL//:GLES_khronos_headers:GLES2"],
        "//conditions:default": [],
    }),
)
