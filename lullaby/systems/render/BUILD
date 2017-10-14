
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
    "//lullaby/util:bits",
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
        ":render_helpers",
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
    name = "next",
    srcs = [
        "detail/render_stats.cc",
        "detail/sort_order.cc",
        "next/mesh.cc",
        "next/mesh_factory.cc",
        "next/render_factory.cc",
        "next/render_state.cc",
        "next/render_system_impl.cc",
        "next/render_system_next.cc",
        "next/shader.cc",
        "next/shader_factory.cc",
        "next/texture.cc",
        "next/texture_atlas.cc",
        "next/texture_atlas_factory.cc",
        "next/texture_factory.cc",
    ],
    hdrs = common_headers + private_headers + [
        "next/mesh.h",
        "next/mesh_factory.h",
        "next/render_component.h",
        "next/render_factory.h",
        "next/render_state.h",
        "next/render_system_next.h",
        "next/shader.h",
        "next/shader_factory.h",
        "next/texture.h",
        "next/texture_atlas.h",
        "next/texture_factory.h",
        "next/texture_atlas_factory.h",
    ],
    copts = [
        "-Ithird_party/GL/gl/include/",
    ],
    defines = ["LULLABY_RENDER_BACKEND_NEXT"],
    deps = common_deps + [
        ":binding_impl",
        ":profiler",
        ":render",
        ":render_helpers",
        "//third_party/fplbase:fplbase_fbs",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/file",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/script",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/rig",
        "//lullaby/util:async_processor",
        "//lullaby/util:buffered_data",
        "//lullaby/util:color",
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
        "@gtest//:gtest_main",
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
