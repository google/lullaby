
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "aligned_alloc",
    hdrs = [
        "aligned_alloc.h",
    ],
)

cc_library(
    name = "arc",
    hdrs = [
        "arc.h",
    ],
    deps = [
        ":math",
    ],
)

cc_library(
    name = "async_processor",
    hdrs = [
        "async_processor.h",
    ],
    deps = [
        ":thread_safe_deque",
    ],
)

cc_library(
    name = "bits",
    hdrs = [
        "bits.h",
    ],
)

cc_library(
    name = "buffered_data",
    hdrs = [
        "buffered_data.h",
    ],
    deps = [
        ":logging",
    ],
)

cc_library(
    name = "built_in_functions",
    hdrs = [
        "built_in_functions.h",
    ],
    deps = [
        ":hash",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "clock",
    hdrs = ["clock.h"],
    deps = [
        ":typeid",
    ],
)

cc_library(
    name = "color",
    hdrs = ["color.h"],
    deps = [
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "common_types",
    hdrs = [
        "common_types.h",
    ],
    deps = [
        ":color",
        ":math",
        ":typeid",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "data_container",
    hdrs = [
        "data_container.h",
    ],
    deps = [
        ":logging",
    ],
)

cc_library(
    name = "dependency_checker",
    srcs = [
        "dependency_checker.cc",
    ],
    hdrs = [
        "dependency_checker.h",
    ],
    deps = [
        ":logging",
        ":typeid",
    ],
)

cc_library(
    name = "fixed_string",
    hdrs = ["fixed_string.h"],
    deps = [
        ":hash",
        ":logging",
        ":string_view",
    ],
)

cc_library(
    name = "hash",
    srcs = [
        "hash.cc",
    ],
    hdrs = [
        "hash.h",
    ],
    deps = [
        ":string_view",
    ],
)

cc_library(
    name = "interpolation",
    srcs = [
        "interpolation.cc",
    ],
    hdrs = [
        "interpolation.h",
    ],
    deps = [
        ":math",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "intersections",
    srcs = [
        "intersections.cc",
    ],
    hdrs = [
        "intersections.h",
    ],
    deps = [
        ":math",
    ],
)

cc_library(
    name = "inward_buffer",
    srcs = [
        "inward_buffer.cc",
    ],
    hdrs = [
        "inward_buffer.h",
    ],
    deps = [
        ":logging",
    ],
)

cc_library(
    name = "job_processor",
    hdrs = [
        "job_processor.h",
    ],
    deps = [
        ":async_processor",
        ":logging",
        ":typeid",
    ],
)

cc_library(
    name = "logging",
    hdrs = [
        "logging.h",
    ],
    deps = [
    ],
)

cc_library(
    name = "macros",
    hdrs = [
        "macros.h",
    ],
)

cc_library(
    name = "make_unique",
    hdrs = [
        "make_unique.h",
    ],
)

cc_library(
    name = "mapped_structure_of_arrays",
    hdrs = [
        "mapped_structure_of_arrays.h",
    ],
    deps = [
        ":logging",
        ":structure_of_arrays",
    ],
)

cc_library(
    name = "math",
    srcs = [
        "math.cc",
    ],
    hdrs = [
        "math.h",
    ],
    deps = [
        ":hash",
        ":logging",
        "@mathfu//:mathfu",
    ],
)

cc_library(
    name = "optional",
    hdrs = [
        "optional.h",
    ],
    deps = [
        ":hash",
        ":logging",
    ],
)

cc_library(
    name = "periodic_function",
    hdrs = [
        "periodic_function.h",
    ],
    deps = [
        ":clock",
    ],
)

cc_library(
    name = "profiler",
    srcs = [
        "profiler.cc",
    ],
    hdrs = [
        "profiler.h",
    ],
    deps = [
        ":logging",
        ":string_view",
    ],
)

cc_library(
    name = "random_number_generator",
    hdrs = ["random_number_generator.h"],
    deps = [
        ":typeid",
    ],
)

cc_library(
    name = "registry",
    hdrs = ["registry.h"],
    deps = [
        ":dependency_checker",
        ":logging",
        ":time",
        ":typeid",
    ],
)

cc_library(
    name = "resource_manager",
    hdrs = ["resource_manager.h"],
    deps = [
        ":hash",
    ],
)

cc_library(
    name = "scheduled_processor",
    srcs = ["scheduled_processor.cc"],
    hdrs = ["scheduled_processor.h"],
    deps = [
        ":clock",
        ":logging",
        ":typeid",
    ],
)

cc_library(
    name = "scoped_java_local_ref",
    hdrs = [
        "scoped_java_local_ref.h",
    ],
    deps = [
        ":logging",
        ":typeid",
    ],
)

cc_library(
    name = "selector",
    srcs = [
    ],
    hdrs = [
        "selector.h",
    ],
    deps = [
        ":optional",
        ":span",
        ":variant",
    ],
)

cc_library(
    name = "span",
    hdrs = [
        "span.h",
    ],
)

cc_library(
    name = "string_preprocessor",
    srcs = [
        "string_preprocessor.cc",
    ],
    hdrs = [
        "string_preprocessor.h",
    ],
    deps = [
        ":logging",
        ":typeid",
    ],
)

cc_library(
    name = "string_view",
    hdrs = [
        "string_view.h",
    ],
)

cc_library(
    name = "structure_of_arrays",
    hdrs = [
        "structure_of_arrays.h",
    ],
    deps = [
        ":logging",
    ],
)

cc_library(
    name = "thread_safe_deque",
    hdrs = [
        "thread_safe_deque.h",
    ],
    deps = [
        ":logging",
    ],
)

cc_library(
    name = "thread_safe_queue",
    hdrs = [
        "thread_safe_queue.h",
    ],
    deps = [
        ":logging",
    ],
)

cc_library(
    name = "time",
    hdrs = [
        "time.h",
    ],
    deps = [
        ":clock",
    ],
)

cc_library(
    name = "trace",
    hdrs = [
        "trace.h",
    ],
    deps = [
    ],
)

cc_library(
    name = "type_util",
    hdrs = [
        "type_util.h",
    ],
    deps = [
        ":optional",
    ],
)

cc_library(
    name = "typed_scheduled_processor",
    srcs = [
        "typed_scheduled_processor.cc",
    ],
    hdrs = [
        "typed_scheduled_processor.h",
    ],
    deps = [
        ":clock",
        ":scheduled_processor",
        ":typeid",
    ],
)

cc_library(
    name = "typed_pointer",
    hdrs = [
        "typed_pointer.h",
    ],
    deps = [
        ":typeid",
    ],
)

cc_library(
    name = "typeid",
    hdrs = [
        "typeid.h",
    ],
    deps = [
        ":hash",
    ],
)

cc_library(
    name = "type_name_generator",
    hdrs = [
        "type_name_generator.h",
    ],
    deps = [
        ":common_types",
        ":type_util",
        ":typeid",
    ],
)

cc_library(
    name = "unordered_vector_map",
    hdrs = ["unordered_vector_map.h"],
)

cc_library(
    name = "utf8",
    srcs = ["utf8_string.cc"],
    hdrs = ["utf8_string.h"],
)

cc_library(
    name = "variant",
    hdrs = [
        "variant.h",
    ],
    deps = [
        ":logging",
        ":type_util",
        ":typeid",
        "@mathfu//:mathfu",
    ],
)
