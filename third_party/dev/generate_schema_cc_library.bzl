
def _flatc(
    name,
    src,
    srcs,
    include_paths_cmd):
  tool = "@flatbuffers//:flatc"
  out = "generate_flatc_code_%s" % (name)
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
      name = out,
      srcs = [src],
      outs = ["lullaby/generated/flatbuffers/%s_generated.h" % (name)],
      tools = [tool] + srcs,
      cmd = " ".join(cmd),
      message = "Generating flatc file for %s:" % (name))
  return ["lullaby/generated/flatbuffers/%s_generated.h" % (name)]


def _lull(
    name,
    src,
    srcs,
    include_paths_cmd):
  tool = "@//tools/flatbuffer_code_generator:flatbuffer_code_generator"
  out = "generate_lull_code_%s" % (name)
  cmd = [
      "$(location %s)" % (tool),
      " ".join(include_paths_cmd),
      "--no-union-value-namespacing",
      "-x",
      "-o $(@D)",
      "$(location %s)" % (src),
  ]
  native.genrule(
      name = out,
      srcs = [src],
      outs = ["lullaby/generated/%s_generated.h" % (name)],
      tools = [tool] + srcs,
      cmd = " ".join(cmd),
      message = "Generating lull file for %s:" % (name))
  return ["lullaby/generated/%s_generated.h" % (name)]


def generate_schema_cc_library(
    name,
    srcs,
    include_paths = [],
    visibility = ["//visibility:public"]):
  include_paths_cmd = ["-I %s" % (s) for s in include_paths]

  hdrs = []
  for src in srcs:
    basename = src.split("/")[-1].replace(".fbs", "")
    hdrs += _flatc(basename, src, srcs, include_paths_cmd)
    hdrs += _lull(basename, src, srcs, include_paths_cmd)

  native.cc_library(
      name = name,
      hdrs = hdrs,
      deps = [
          "@flatbuffers//:flatbuffers",
          "@mathfu//:mathfu",
          "@//lullaby/util:logging",
          "@//lullaby/util:common_types",
          "@//lullaby/util:optional",
      ],
      # includes = [".", ".."],
      visibility = visibility)
