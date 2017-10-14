
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

GUNIT_PORTABLE_DEPS = [
    
    
    "@gtest//:gtest_main",
]

# For testing we use the software renderer. Satisfying this dependency is
# required through lullaby's use of fplbase_stdlib.
TEST_ONLY_GL_DEPS = [
    "@//third_party/gl",
]


cc_test(
    name = "animation_system_tests",
    srcs = [
        "animation_system_test.cc",
    ],
    deps = [
        ":portable_test_macros",
        "//lullaby/generated",
        "//lullaby/modules/animation_channels:transform_channels",
        "//lullaby/modules/ecs",
        "//lullaby/systems/animation",
        "//lullaby/systems/render",
        "//lullaby/systems/render:render_system_mock",
        "//lullaby/systems/transform",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "asset_loader_tests",
    srcs = ["asset_loader_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/modules/file",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "async_processor_tests",
    srcs = ["async_processor_test.cc"],
    deps = [
        "//lullaby/util:async_processor",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "bits_tests",
    srcs = ["bits_test.cc"],
    deps = [
        "//lullaby/util:bits",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "blueprint_tests",
    srcs = ["blueprint_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/modules/ecs",
        "//lullaby/generated",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "buffered_data_tests",
    srcs = ["buffered_data_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/util:buffered_data",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "collision_system_tests",
    srcs = ["collision_system_test.cc"],
    deps = [
        "//lullaby/generated",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/systems/collision",
        "//lullaby/systems/transform",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "config_tests",
    srcs = ["config_test.cc"],
    deps = [
        "//lullaby/generated",
        "//lullaby/modules/config",
        "//lullaby/modules/flatbuffers",
        "//lullaby/util:inward_buffer",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "data_container_tests",
    srcs = ["data_container_test.cc"],
    deps = [
        ":portable_test_macros",
        ":test_data_container",
        "//lullaby/modules/render",
        "//lullaby/util:data_container",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "datastore_system_tests",
    srcs = ["datastore_system_test.cc"],
    deps = [
        "//lullaby/generated",
        "//lullaby/modules/ecs",
        "//lullaby/systems/datastore",
        "//lullaby/util:common_types",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "deform_system_tests",
    srcs = ["deform_system_test.cc"],
    deps = [
        ":mathfu_matchers",
        "//lullaby/generated",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/systems/deform",
        "//lullaby/systems/render",
        "//lullaby/systems/render:render_system_mock",
        "//lullaby/systems/transform",
        "//lullaby/util:registry",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "dependency_checker_tests",
    srcs = ["dependency_checker_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/modules/ecs",
        "//lullaby/util:logging",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "dispatcher_tests",
    srcs = ["dispatcher_test.cc"],
    deps = [
        
        "//lullaby/modules/dispatcher",
        "//lullaby/util:common_types",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "dispatcher_system_tests",
    srcs = ["dispatcher_system_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/generated",
        "//lullaby/modules/script",
        "//lullaby/systems/dispatcher",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "display_list_tests",
    srcs = ["display_list_test.cc"],
    deps = [
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/systems/render:render_system_mock",
        "//lullaby/systems/transform",
        "//lullaby/util:registry",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "edit_text_tests",
    srcs = ["edit_text_test.cc"],
    deps = [
        "//lullaby/systems/render",
        "//lullaby/systems/render:fpl",
        "//lullaby/systems/text_input",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "event_wrapper_tests",
    srcs = ["event_wrapper_test.cc"],
    deps = [
        "@mathfu//:mathfu",
        "//lullaby/modules/dispatcher",
        "//lullaby/util:common_types",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "file_tests",
    srcs = ["file_test.cc"],
    deps = [
        "//lullaby/modules/file",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "fixed_string_tests",
    srcs = ["fixed_string_test.cc"],
    deps = [
        "//lullaby/util:fixed_string",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "flatbuffer_reader_tests",
    srcs = ["flatbuffer_reader_test.cc"],
    deps = [
        "@flatbuffers//:flatbuffers",
        "//lullaby/modules/flatbuffers",
        "//lullaby/util:math",
        "//lullaby/util:typeid",
        "//lullaby/generated:flatc_fbs",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "flatbuffer_writer_tests",
    srcs = ["flatbuffer_writer_test.cc"],
    deps = [
        "@flatbuffers//:flatbuffers",
        "//lullaby/modules/flatbuffers",
        "//lullaby/util:math",
        "//lullaby/util:typeid",
        "//lullaby/generated:flatc_fbs",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "hash_tests",
    srcs = ["hash_test.cc"],
    deps = [
        "//lullaby/util:hash",
        "//lullaby/util:string_view",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "input_manager_tests",
    srcs = ["input_manager_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/modules/input",
        "//lullaby/util:bits",
        "//lullaby/util:clock",
        "//lullaby/util:logging",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "intersections_tests",
    srcs = ["intersections_test.cc"],
    deps = [
        ":mathfu_matchers",
        ":portable_test_macros",
        "//lullaby/util:intersections",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "inward_buffer_tests",
    srcs = ["inward_buffer_test.cc"],
    deps = [
        "//lullaby/util:inward_buffer",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "job_processor_tests",
    srcs = ["job_processor_test.cc"],
    deps = [
        "//lullaby/util:job_processor",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "layout_tests",
    srcs = ["layout_test.cc"],
    deps = [
        "//lullaby/modules/layout",
        "//lullaby/modules/ecs",
        "//lullaby/modules/dispatcher",
        "//lullaby/systems/layout:layout_box",
        "//lullaby/systems/transform",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "layout_box_system_tests",
    srcs = ["layout_box_system_test.cc"],
    deps = [
        ":mathfu_matchers",
        "//lullaby/generated",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/file",
        "//lullaby/systems/layout:layout_box",
        "//lullaby/systems/transform",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "layout_system_tests",
    srcs = ["layout_system_test.cc"],
    deps = [
        "//lullaby/generated",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/file",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/layout",
        "//lullaby/systems/layout:layout_box",
        "//lullaby/systems/transform",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "make_unique_tests",
    srcs = ["make_unique_test.cc"],
    deps = [
        "//lullaby/util:logging",
        "//lullaby/util:make_unique",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "mapped_structure_of_arrays_tests",
    srcs = ["mapped_structure_of_arrays_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/util:mapped_structure_of_arrays",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "math_util_tests",
    srcs = ["math_util_test.cc"],
    deps = [
        ":mathfu_matchers",
        ":portable_test_macros",
        "//lullaby/util:math",
        "@mathfu//:mathfu",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_library(
    name = "mathfu_matchers",
    testonly = 1,
    hdrs = [
        "mathfu_matchers.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@mathfu//:mathfu",
        "//lullaby/util:logging",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "mathfu_matchers_tests",
    srcs = ["mathfu_matchers_test.cc"],
    deps = [
        ":mathfu_matchers",
        "@mathfu//:mathfu",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "mesh_data_tests",
    srcs = ["mesh_data_test.cc"],
    deps = [
        ":portable_test_macros",
        ":test_data_container",
        "//lullaby/modules/render",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "mesh_util_tests",
    srcs = ["mesh_util_test.cc"],
    deps = [
        ":mathfu_matchers",
        ":portable_test_macros",
        "//lullaby/modules/render",
        "@mathfu//:mathfu",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "name_system_tests",
    srcs = ["name_system_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/generated",
        "//lullaby/modules/ecs",
        "//lullaby/systems/name",
        "//lullaby/systems/transform",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "nine_patch_tests",
    srcs = ["nine_patch_test.cc"],
    deps = [
        ":mathfu_matchers",
        ":portable_test_macros",
        "//lullaby/modules/render",
        "//lullaby/systems/render",
        "@mathfu//:mathfu",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "optional_tests",
    srcs = ["optional_test.cc"],
    deps = [
        "//lullaby/util:optional",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "periodic_function_tests",
    srcs = ["periodic_function_test.cc"],
    deps = [
        "//lullaby/util:periodic_function",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "physics_system_tests",
    srcs = ["physics_system_test.cc"],
    deps = [
        ":mathfu_matchers",
        ":portable_test_macros",
        "//lullaby/generated",
        "//lullaby/events",
        "//lullaby/modules/ecs",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/physics",
        "//lullaby/systems/transform",
        "//lullaby/util:make_unique",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_library(
    name = "portable_test_macros",
    testonly = 1,
    hdrs = [
        "portable_test_macros.h",
    ],
    visibility = ["//visibility:public"],
    deps = GUNIT_PORTABLE_DEPS,
)


cc_test(
    name = "queued_dispatcher_tests",
    srcs = ["queued_dispatcher_test.cc"],
    deps = [
        "//lullaby/modules/dispatcher",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "registry_tests",
    srcs = ["registry_test.cc"],
    deps = [
        "//lullaby/util:registry",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "resource_manager_tests",
    srcs = ["resource_manager_test.cc"],
    deps = [
        "//lullaby/util:resource_manager",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "reticle_system_tests",
    srcs = ["reticle_system_test.cc"],
    defines = ["DISABLE_GOOGLE_STRING"],
    deps = [
        "//lullaby/generated",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/input",
        "//lullaby/systems/collision",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/render",
        "//lullaby/systems/render:render_system_mock",
        "//lullaby/systems/reticle",
        "//lullaby/systems/transform",
        "//lullaby/util:clock",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "scheduled_processor_tests",
    srcs = ["scheduled_processor_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/util:scheduled_processor",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "serialize_tests",
    srcs = ["serialize_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/modules/serialize",
        "//lullaby/util:hash",
        "//lullaby/util:optional",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "span_tests",
    srcs = [
        "span_test.cc",
    ],
    deps = [
        "//lullaby/util:span",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "string_preprocessor_tests",
    srcs = ["string_preprocessor_test.cc"],
    deps = [
        "//lullaby/util:string_preprocessor",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "string_view_tests",
    srcs = ["string_view_test.cc"],
    deps = [
        "//lullaby/util:hash",
        "//lullaby/util:string_view",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "structure_of_arrays_tests",
    srcs = ["structure_of_arrays_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/util:structure_of_arrays",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_library(
    name = "test_data_container",
    testonly = 1,
    hdrs = [
        "test_data_container.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//lullaby/util:data_container",
    ],
)


cc_test(
    name = "text_input_system_tests",
    srcs = ["text_input_system_test.cc"],
    deps = [
        "//lullaby/generated",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/render",
        "//lullaby/systems/render:render_system_mock",
        "//lullaby/systems/text_input",
        "//lullaby/systems/transform",
        "//lullaby/util:math",
        "@mathfu//:mathfu",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "text_system_tests",
    srcs = ["text_system_test.cc"],
    defines = ["DISABLE_GOOGLE_STRING"],
    deps = [
        "//lullaby/generated",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/systems/render",
        "//lullaby/systems/render:render_system_mock",
        "//lullaby/systems/text",
        "//lullaby/systems/text:flatui",
        "//lullaby/systems/transform",
        "//lullaby/util:registry",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "thread_safe_deque_tests",
    srcs = ["thread_safe_deque_test.cc"],
    deps = [
        "//lullaby/util:thread_safe_deque",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "thread_safe_queue_tests",
    srcs = ["thread_safe_queue_test.cc"],
    deps = [
        "//lullaby/util:thread_safe_queue",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "time_tests",
    srcs = [
        "time_test.cc",
    ],
    deps = [
        "//lullaby/util:clock",
        "//lullaby/util:time",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "transform_system_tests",
    srcs = ["transform_system_test.cc"],
    deps = [
        ":mathfu_matchers",
        ":portable_test_macros",
        "//lullaby/generated",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/systems/transform",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "triangle_mesh_tests",
    srcs = ["triangle_mesh_test.cc"],
    deps = [
        "//lullaby/modules/render",
        "//lullaby/systems/render",
        "//lullaby/util:math",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "typed_scheduled_processor_tests",
    srcs = ["typed_scheduled_processor_test.cc"],
    deps = [
        "//lullaby/util:typed_scheduled_processor",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "unordered_vector_map_tests",
    srcs = ["unordered_vector_map_test.cc"],
    deps = [
        "//lullaby/util:unordered_vector_map",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "utf8_string_tests",
    srcs = ["utf8_string_test.cc"],
    deps = [
        "//lullaby/util:utf8",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "variant_tests",
    srcs = ["variant_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/modules/serialize",
        "//lullaby/util:logging",
        "//lullaby/util:variant",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "vertex_tests",
    srcs = ["vertex_test.cc"],
    deps = [
        "//lullaby/modules/render",
        "//lullaby/systems/render",
        "@mathfu//:mathfu",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "vertex_format_tests",
    srcs = ["vertex_format_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/modules/render",
        "//lullaby/systems/render",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

