licenses(["notice"])  # Apache 2.0

# Python script to generate lullaby-specific EntityDef schema using
# client-provided schema containing ComponentDef union.
py_binary(
    name = "generate_entity_schema",
    srcs = ["generate_entity_schema.py"],
    python_version = "PY3",
    srcs_version = "PY2AND3",
    visibility = ["//visibility:public"],
)

# Entity template file used by |generate_entity_schema| binary.
exports_files([
    "entity_schema.template",
])

