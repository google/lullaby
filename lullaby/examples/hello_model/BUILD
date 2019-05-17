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
    name = "hello_model",
    srcs = [
        "src/hello_model.cc",
    ],
    hdrs = [
        "src/hello_model.h",
    ],
    data = [
        "//lullaby/examples/hello_model/data",
    ],
    ports = [
        "android",
        "desktop",
        "daydream",
        "ios",
    ],
    deps = [
        ":entity_library",
        "//lullaby/examples/example_app",
        "//lullaby/events",
        "//lullaby/modules/ecs",
        "//lullaby/modules/script",
        "//lullaby/systems/model_asset",
        "//lullaby/systems/render:next",
        "//lullaby/systems/transform",
        "//lullaby/util:clock",
        "//lullaby/util:entity",
        "//lullaby/util:math",
        "@mathfu//:mathfu",
    ],
)
