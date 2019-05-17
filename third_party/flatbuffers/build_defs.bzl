def flatbuffer_cc_library(
        name,
        srcs,
        include_paths = [],
        visibility = ["//visibility:public"]):
    tool = "@flatbuffers//:flatc"
    include_paths_cmd = ["-I %s" % s for s in include_paths]

    hdrs = []
    for src in srcs:
        basename = src.replace(".fbs", "").split("/")[-1]
        out = "%s_generated.h" % basename
        hdrs += [out]

        cmd = [
            "$(location %s)" % (tool),
            " ".join(include_paths_cmd),
            "--no-union-value-namespacing",
            "--gen-name-strings",
            "--gen-mutable",
            "-c",
            "-o $(@D)",
            "$(location %s)" % (src),
        ]

        native.genrule(
            name = "%s_generate_%s" % (name, basename),
            srcs = [src],
            outs = [out],
            tools = [tool] + srcs,
            cmd = " ".join(cmd),
            message = "Generating header file for %s:" % name,
        )

    native.cc_library(
        name = name,
        hdrs = hdrs,
        includes = [".", ".."],
        deps = [
            "@flatbuffers//:flatbuffers",
        ],
        visibility = visibility,
    )
