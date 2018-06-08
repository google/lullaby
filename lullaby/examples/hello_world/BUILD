load(
    "@//dev:generate_entity_schema.bzl",
    "generate_entity_schema",
)
load(
    "//lullaby/examples/example_app:build_defs.bzl",
    "lullaby_example",
)

licenses(["notice"])  # Apache 2.0

generate_entity_schema(
    src = "schemas/entity.fbs",
    library_name = "entity_library",
    schema_name = "entity_schema",
)

lullaby_example(
    name = "hello_world",
    srcs = [
        "src/hello_world.cc",
    ],
    hdrs = [
        "src/hello_world.h",
    ],
    data = [
        "//lullaby/examples/hello_world/data",
    ],
    ports = [
        "android",
        "desktop",
        "daydream",
    ],
    deps = [
        ":entity_library",
        "//lullaby/examples/example_app",
        "//lullaby/modules/ecs",
        "//lullaby/systems/render:next",
        "//lullaby/systems/text:flatui",
        "//lullaby/systems/transform",
    ],
)
