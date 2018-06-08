"""Build Animations assets from source 3d asset files or json files."""

load("@motive//:motive:build_defs.bzl", "motive_anim_assets")

def build_animation(
    name,
    srcs,
    defining_animation = "",
    strip_prefix = "",
    visibility = ["//visibility:public"]):
  """Generates a motiveanim binary from source files.

  Args:
    name: name for the filegroup contain the set of generated motiveanim files
    srcs: list of 3d animation fbx files
    defining_animation: optional, name of single "defining animation" used to
                        initialize the animation system to efficiently play all
                        generated animations
    strip_prefix: optional string, will be stripped from all input file paths
                  in output file generation. All subdirectories after
                  strip_prefix will be retained.
    visibility: The visibility of the entity target. Defaults to public.
  """
  motive_anim_assets(
      name = name,
      additional_anim_pipeline_args = "--norepeat",
      defining_anim = defining_animation,
      fbx_srcs = srcs,
      strip_prefix = strip_prefix,
      visibility = visibility,
  )
