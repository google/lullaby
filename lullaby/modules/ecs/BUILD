
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "ecs",
    srcs = [
        "blueprint.cc",
        "blueprint_builder.cc",
        "blueprint_reader.cc",
        "blueprint_type.cc",
        "blueprint_writer.cc",
        "component_handlers.cc",
        "entity_factory.cc",
        "system.cc",
    ],
    hdrs = [
        "blueprint.h",
        "blueprint_builder.h",
        "blueprint_reader.h",
        "blueprint_tree.h",
        "blueprint_type.h",
        "blueprint_writer.h",
        "component.h",
        "component_handlers.h",
        "entity_factory.h",
        "system.h",
    ],
    deps = [
        "@flatbuffers//:flatbuffers",
        "//:fbs",
        "//lullaby/modules/file",
        "//lullaby/modules/script",
        "//lullaby/util:dependency_checker",
        "//lullaby/util:entity",
        "//lullaby/util:filename",
        "//lullaby/util:flatbuffer_reader",
        "//lullaby/util:flatbuffer_writer",
        "//lullaby/util:hash",
        "//lullaby/util:inward_buffer",
        "//lullaby/util:logging",
        "//lullaby/util:make_unique",
        "//lullaby/util:optional",
        "//lullaby/util:registry",
        "//lullaby/util:resource_manager",
        "//lullaby/util:span",
        "//lullaby/util:string_view",
        "//lullaby/util:typeid",
        "//lullaby/util:unordered_vector_map",
        "//lullaby/util:variant",
    ],
)

cc_library(
    name = "ecs_jni",
    srcs = [
        "entity_factory_jni.cc",
    ],
    hdrs = [
        "entity_factory_jni.h",
    ],
    deps = [
        ":ecs",
        "//lullaby/modules/jni:jni_util",
        "//lullaby/modules/jni:registry_jni",
        "//lullaby/util:logging",
    ],
)
