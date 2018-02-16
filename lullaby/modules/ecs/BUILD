
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "ecs",
    srcs = [
        "blueprint.cc",
        "blueprint_type.cc",
        "entity_factory.cc",
        "system.cc",
    ],
    hdrs = [
        "blueprint.h",
        "blueprint_tree.h",
        "blueprint_type.h",
        "component.h",
        "entity_factory.h",
        "system.h",
    ],
    deps = [
        "@flatbuffers//:flatbuffers",
        "//lullaby/modules/file",
        "//lullaby/util:dependency_checker",
        "//lullaby/util:entity",
        "//lullaby/util:filename",
        "//lullaby/util:flatbuffer_reader",
        "//lullaby/util:flatbuffer_writer",
        "//lullaby/util:hash",
        "//lullaby/util:logging",
        "//lullaby/util:make_unique",
        "//lullaby/util:optional",
        "//lullaby/util:registry",
        "//lullaby/util:resource_manager",
        "//lullaby/util:span",
        "//lullaby/util:string_view",
        "//lullaby/util:typeid",
        "//lullaby/util:unordered_vector_map",
    ],
)
