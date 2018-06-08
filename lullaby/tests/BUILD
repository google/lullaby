
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

GUNIT_PORTABLE_DEPS = [
    "@gtest//:gtest_main",
    
]

# Lullaby uses fplbase_stdlib, which requires a GL implementation to be linked
# in.  The implementation is not actually used during test execution.
TEST_ONLY_GL_DEPS = [
]


cc_test(
    name = "animation_system_tests",
    srcs = [
        "animation_system_test.cc",
    ],
    deps = [
        ":portable_test_macros",
        "//:fbs",
        "//lullaby/modules/animation_channels:transform_channels",
        "//lullaby/modules/ecs",
        "//lullaby/systems/animation",
        "//lullaby/systems/render",
        "//lullaby/systems/render:render_system_mock",
        "//lullaby/systems/transform",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "arg_parser_tests",
    srcs = ["arg_parser_test.cc"],
    deps = [
        "//lullaby/util:logging",
        "//lullaby/util:arg_parser",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "asset_loader_tests",
    srcs = ["asset_loader_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/modules/file",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "async_processor_tests",
    srcs = ["async_processor_test.cc"],
    deps = [
        "//lullaby/util:async_processor",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "async_render_object_tests",
    srcs = ["async_render_object_test.cc"],
    deps = [
        "//lullaby/systems/render:async_render_object",
        "//lullaby/util:make_unique",
    ] + GUNIT_PORTABLE_DEPS,
)


cc_test(
    name = "bits_tests",
    srcs = ["bits_test.cc"],
    deps = [
        "//lullaby/util:bits",
    ] + GUNIT_PORTABLE_DEPS,
)


cc_test(
    name = "blueprint_tests",
    srcs = ["blueprint_test.cc"],
    deps = [
        ":portable_test_macros",
        "@flatbuffers//:flatbuffers",
        "//lullaby/modules/ecs",
        "//lullaby/util:flatbuffer_reader",
        "//:fbs",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "buffered_data_tests",
    srcs = ["buffered_data_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/util:buffered_data",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "collision_system_tests",
    srcs = ["collision_system_test.cc"],
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/systems/collision",
        "//lullaby/systems/transform",
    ] + GUNIT_PORTABLE_DEPS,
)


cc_test(
    name = "data_container_tests",
    srcs = ["data_container_test.cc"],
    deps = [
        ":portable_test_macros",
        ":test_data_container",
        "//lullaby/modules/render",
        "//lullaby/util:data_container",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "datastore_system_tests",
    srcs = ["datastore_system_test.cc"],
    deps = [
        "//:fbs",
        "//lullaby/modules/ecs",
        "//lullaby/systems/datastore",
        "//lullaby/util:common_types",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "debug_camera_tests",
    size = "small",
    srcs = ["debug_camera_test.cc"],
    deps = [
        ":mathfu_matchers",
        "//lullaby/modules/debug",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/input",
        "//lullaby/systems/render:render_system_mock",
        "//lullaby/util:time",
        "//lullaby/util:registry",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "debug_hash_tests",
    srcs = ["debug_hash_test.cc"],
    defines = ["LULLABY_DEBUG_HASH"],
    deps = [
        "//lullaby/util:debug_hash",
        "//lullaby/util:string_view",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "deform_system_tests",
    srcs = ["deform_system_test.cc"],
    deps = [
        ":mathfu_matchers",
        "//:fbs",
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
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "dispatcher_system_tests",
    srcs = ["dispatcher_system_test.cc"],
    deps = [
        ":portable_test_macros",
        "//:fbs",
        "//lullaby/modules/script",
        "//lullaby/systems/dispatcher",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "dispatcher_tests",
    srcs = ["dispatcher_test.cc"],
    deps = [
        "//lullaby/modules/dispatcher",
        "//lullaby/util:common_types",
    ] + GUNIT_PORTABLE_DEPS,
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
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "events_tests",
    srcs = ["events_test.cc"],
    deps = [
        "@mathfu//:mathfu",
        "//lullaby/events",
        "//lullaby/modules/serialize",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "expected_tests",
    srcs = [
        "expected_test.cc",
    ],
    deps = [
        "//lullaby/util:expected",
    ] + GUNIT_PORTABLE_DEPS,
)


cc_test(
    name = "file_tests",
    srcs = ["file_test.cc"],
    deps = [
        "//lullaby/util:filename",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "fixed_string_tests",
    srcs = ["fixed_string_test.cc"],
    deps = [
        "//lullaby/util:fixed_string",
    ] + GUNIT_PORTABLE_DEPS,
)


cc_test(
    name = "function_binder_tests",
    srcs = ["function_binder_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/modules/script",
        "//lullaby/util:variant",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "hash_tests",
    srcs = ["hash_test.cc"],
    deps = [
        "//lullaby/util:hash",
        "//lullaby/util:string_view",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "image_util_tests",
    srcs = ["image_util_test.cc"],
    deps = [
        "@mathfu//:mathfu",
        "//lullaby/modules/render",
        "//lullaby/util:logging",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "input_focus_locker_tests",
    srcs = ["input_focus_locker_test.cc"],
    deps = [
        "@mathfu//:mathfu",
        "//:fbs",
        "//lullaby/systems/transform",
        "//lullaby/modules/ecs",
        "//lullaby/modules/input",
        "//lullaby/modules/reticle",
        "//lullaby/modules/input_processor",
        "//lullaby/util:registry",
        "//lullaby/systems/render",
        "//lullaby/systems/render:render_system_mock",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "input_manager_tests",
    srcs = ["input_manager_test.cc"],
    deps = [
        ":portable_test_macros",
        ":mathfu_matchers",
        "//lullaby/modules/input",
        "//lullaby/util:bits",
        "//lullaby/util:clock",
        "//lullaby/util:logging",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "input_processor_tests",
    srcs = ["input_processor_test.cc"],
    deps = [
        "//:fbs",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/transform",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/input_processor",
        "//lullaby/util:clock",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "interpolation_tests",
    srcs = ["interpolation_tests.cc"],
    deps = [
        ":mathfu_matchers",
        "//lullaby/util:interpolation",
        "//lullaby/util:math",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "intersections_tests",
    srcs = ["intersections_test.cc"],
    deps = [
        ":mathfu_matchers",
        ":portable_test_macros",
        "//lullaby/util:intersections",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "inward_buffer_tests",
    srcs = ["inward_buffer_test.cc"],
    deps = [
        "//lullaby/util:inward_buffer",
    ] + GUNIT_PORTABLE_DEPS,
)


cc_test(
    name = "job_processor_tests",
    srcs = ["job_processor_test.cc"],
    deps = [
        "//lullaby/util:job_processor",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "layout_box_system_tests",
    srcs = ["layout_box_system_test.cc"],
    deps = [
        ":mathfu_matchers",
        "//:fbs",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/file",
        "//lullaby/systems/layout:layout_box",
        "//lullaby/systems/transform",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "layout_system_tests",
    srcs = ["layout_system_test.cc"],
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/file",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/layout",
        "//lullaby/systems/layout:layout_box",
        "//lullaby/systems/transform",
    ] + GUNIT_PORTABLE_DEPS,
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
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "log_tag_tests",
    srcs = ["log_tag_test.cc"],
    deps = [
        "//lullaby/systems/render:render_system_mock",
        "//lullaby/modules/debug",
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
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "material_tests",
    srcs = ["material_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/systems/render:next",
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
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "mathfu_fb_conversions_tests",
    srcs = ["mathfu_fb_conversions_test.cc"],
    deps = [
        "//lullaby/util:math",
        "//lullaby/modules/flatbuffers",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "mathfu_gvr_conversions_tests",
    srcs = ["mathfu_gvr_conversions_test.cc"],
    deps = [
        "//lullaby/modules/gvr",
    ] + GUNIT_PORTABLE_DEPS,
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
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "mesh_util_tests",
    srcs = ["mesh_util_test.cc"],
    deps = [
        ":mathfu_matchers",
        ":portable_test_macros",
        "//lullaby/modules/render",
        "@mathfu//:mathfu",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "mock_render_system_tests",
    srcs = ["mock_render_system_test.cc"],
    deps = [
        "//lullaby/modules/ecs",
        "//lullaby/util:entity",
        "//lullaby/systems/render",
        "//lullaby/systems/render:render_system_mock",
        "//lullaby/util:registry",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "name_system_tests",
    srcs = ["name_system_test.cc"],
    deps = [
        ":portable_test_macros",
        "//:fbs",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/systems/name",
        "//lullaby/systems/transform",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "nine_patch_system_tests",
    srcs = ["nine_patch_system_test.cc"],
    defines = ["DISABLE_GOOGLE_STRING"],
    deps = [
        ":mathfu_matchers",
        "//:fbs",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/systems/nine_patch",
        "//lullaby/systems/render",
        "//lullaby/systems/render:render_system_mock",
        "//lullaby/systems/transform",
        "//lullaby/util:registry",
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
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "periodic_function_tests",
    srcs = ["periodic_function_test.cc"],
    deps = [
        "//lullaby/util:periodic_function",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "physics_system_tests",
    srcs = ["physics_system_test.cc"],
    deps = [
        ":mathfu_matchers",
        ":portable_test_macros",
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/ecs",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/physics",
        "//lullaby/systems/transform",
        "//lullaby/util:make_unique",
    ] + GUNIT_PORTABLE_DEPS,
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
    ] + GUNIT_PORTABLE_DEPS,
)


cc_test(
    name = "registry_tests",
    srcs = ["registry_test.cc"],
    deps = [
        "//lullaby/util:registry",
    ] + GUNIT_PORTABLE_DEPS,
)


cc_test(
    name = "resource_manager_tests",
    srcs = ["resource_manager_test.cc"],
    deps = [
        "//lullaby/util:resource_manager",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "reticle_system_tests",
    srcs = ["reticle_system_test.cc"],
    defines = ["DISABLE_GOOGLE_STRING"],
    deps = [
        "//:fbs",
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
        "//lullaby/util:device_util",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "sanitize_shader_source_tests",
    srcs = ["sanitize_shader_source_test.cc"],
    deps = [
        "//lullaby/modules/render",
        "//lullaby/util:logging",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "scheduled_processor_tests",
    srcs = ["scheduled_processor_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/util:scheduled_processor",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "script_system_tests",
    srcs = ["script_system_test.cc"],
    deps = [
        "//:fbs",
        "//lullaby/modules/ecs",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/script",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/script",
        "//lullaby/systems/transform",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "scroll_tests",
    srcs = ["scroll_test.cc"],
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/input",
        "//lullaby/systems/animation",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/render:render_system_mock",
        "//lullaby/systems/scroll",
        "//lullaby/systems/transform",
        "//lullaby/util:math",
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
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "shader_clip_system_tests",
    srcs = ["shader_clip_system_test.cc"],
    deps = [
        ":mathfu_matchers",
        "//:fbs",
        "//lullaby/contrib/shader_clip",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/systems/render",
        "//lullaby/systems/render:render_system_mock",
        "//lullaby/systems/transform",
        "//lullaby/util:make_unique",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "sort_order_tests",
    srcs = ["sort_order_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/modules/ecs",
        "//lullaby/systems/render",
        "//lullaby/systems/render:sort_order",
        "//lullaby/systems/transform",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "span_tests",
    srcs = [
        "span_test.cc",
    ],
    deps = [
        "//lullaby/util:span",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "stategraph_tests",
    srcs = [
        "stategraph_signal_test.cc",
        "stategraph_state_test.cc",
        "stategraph_test.cc",
        "stategraph_track_test.cc",
    ],
    deps = [
        ":portable_test_macros",
        "//lullaby/modules/stategraph",
        "//lullaby/util:common_types",
        "//lullaby/util:make_unique",
        "//lullaby/util:optional",
        "//lullaby/util:time",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "string_preprocessor_tests",
    srcs = ["string_preprocessor_test.cc"],
    deps = [
        "//lullaby/util:string_preprocessor",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "string_view_tests",
    srcs = ["string_view_test.cc"],
    deps = [
        "//lullaby/util:hash",
        "//lullaby/util:string_view",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "structure_of_arrays_tests",
    srcs = ["structure_of_arrays_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/util:structure_of_arrays",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "tagged_file_loader_tests",
    srcs = ["tagged_file_loader_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/modules/file",
        "//lullaby/modules/file:mock_tagged_file_loader",
        "//lullaby/modules/file:tagged_file_loader",
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
        "//:fbs",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/render",
        "//lullaby/systems/render:render_system_mock",
        "//lullaby/systems/text",
        "//lullaby/systems/text_input",
        "//lullaby/systems/text:stub",
        "//lullaby/systems/transform",
        "//lullaby/util:make_unique",
        "//lullaby/util:math",
        "@mathfu//:mathfu",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


cc_test(
    name = "thread_safe_deque_tests",
    srcs = ["thread_safe_deque_test.cc"],
    deps = [
        "//lullaby/util:thread_safe_deque",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "thread_safe_queue_tests",
    srcs = ["thread_safe_queue_test.cc"],
    deps = [
        "//lullaby/util:thread_safe_queue",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "time_tests",
    srcs = [
        "time_test.cc",
    ],
    deps = [
        "//lullaby/util:clock",
        "//lullaby/util:time",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "transform_system_tests",
    srcs = ["transform_system_test.cc"],
    deps = [
        ":mathfu_matchers",
        ":portable_test_macros",
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/systems/transform",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "type_name_generator_tests",
    srcs = ["type_name_generator_test.cc"],
    deps = [
        "//lullaby/util:type_name_generator",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "typed_scheduled_processor_tests",
    srcs = ["typed_scheduled_processor_test.cc"],
    deps = [
        "//lullaby/util:typed_scheduled_processor",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "uniform_linker_tests",
    srcs = ["uniform_linker_test.cc"],
    deps = [
        "//lullaby/systems/render:uniform_linker",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "uniform_tests",
    srcs = ["uniform_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/systems/render:next",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "unordered_vector_map_tests",
    srcs = ["unordered_vector_map_test.cc"],
    deps = [
        "//lullaby/util:unordered_vector_map",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "utf8_string_tests",
    srcs = ["utf8_string_test.cc"],
    deps = [
        "//lullaby/util:utf8",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "variant_converter_tests",
    srcs = ["variant_converter_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/modules/function",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "variant_fb_conversions_tests",
    srcs = ["variant_fb_conversions_test.cc"],
    deps = [
        "//lullaby/modules/flatbuffers",
        "//lullaby/util:math",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "variant_tests",
    srcs = ["variant_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/modules/serialize",
        "//lullaby/util:logging",
        "//lullaby/util:variant",
    ] + GUNIT_PORTABLE_DEPS,
)

cc_test(
    name = "vertex_format_tests",
    srcs = ["vertex_format_test.cc"],
    deps = [
        ":portable_test_macros",
        "//lullaby/modules/render",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)

cc_test(
    name = "vertex_tests",
    srcs = ["vertex_test.cc"],
    deps = [
        "//lullaby/modules/render",
        "@mathfu//:mathfu",
    ] + GUNIT_PORTABLE_DEPS + TEST_ONLY_GL_DEPS,
)


