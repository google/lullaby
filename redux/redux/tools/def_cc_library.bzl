"""Generates a cc_library from a def file."""

_COMMON_DEPS = [
    "//redux/modules/base:hash",
    "//redux/modules/base:typeid",
    "//redux/modules/math:bounds",
    "//redux/modules/math:quaternion",
    "//redux/modules/math:vector",
    "//redux/modules/var",
]

def def_cc_library(
        name,
        src,
        deps = [],
        visibility = None):
    """Generates a cc_library from flatbuffer schemas.

    Args:
      name: Rule name.
      src: Source .def file.
      deps: Optional, list of cc_library dependencies.
      visibility: Visibility of the generated library.

    Outs:
      cc_library(name): library with generated header file
    """
    defc = "//redux/tools/def_code_generator"
    out = src.replace(".def", "_generated.h")

    args = [
        "$(location {})".format(defc),
        "--input $(location {})".format(src),
        "--output $(location {})".format(out),
    ]
    native.genrule(
        name = "generate_{}".format(name),
        srcs = [src],
        outs = [out],
        tools = [defc],
        cmd = " ".join(args),
        message = "Generating header file for: {}".format(name),
        visibility = ["//visibility:private"],
    )

    # Generate the cc_library from the generated code.
    native.cc_library(
        name = name,
        hdrs = [out],
        deps = depset(_COMMON_DEPS + deps).to_list(),
        visibility = visibility,
    )
