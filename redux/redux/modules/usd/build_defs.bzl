"""Utilities for dealing with the USD library."""

def copy_plugins_and_schemas(name, srcs, outdir):
    """Copies USD plugins and schemas into the given output folder.

    Args:
      name: string, the output filegroup containing the plugin and schema files.
      srcs: input targets for the plugin and schema files..
      outdir: string, output folder.
    """

    outs = []
    for src in srcs:
        package = src.split(":")[-1].split("_")[0]
        package_outdir = "{}/{}".format(outdir, package)

        json_out = "{}/plugInfo.json".format(package_outdir)
        usda_out = "{}/generatedSchema.usda".format(package_outdir)

        native.genrule(
            name = name + "_" + package,
            srcs = [src],
            outs = [json_out, usda_out],
            cmd = "touch $(location " + json_out + "); " +
                  "touch $(location " + usda_out + "); " +
                  "for S in $(SRCS); do " +
                  "  if [[ $$S == *plugInfo.json ]]; then " +
                  "    sed -e 's/@PLUG_INFO_LIBRARY_PATH@//' " +
                  "        -e 's/@PLUG_INFO_RESOURCE_PATH@//' " +
                  "        -e 's/@PLUG_INFO_ROOT@/./' " +
                  "    $$S > $(location " + json_out + "); " +
                  "  fi; " +
                  "  if [[ $$S == *generatedSchema.usda ]]; then " +
                  "    cp $$S $(location " + usda_out + "); " +
                  "  fi; " +
                  "done;",
        )
        outs.append(json_out)
        outs.append(usda_out)

    native.filegroup(
        name = name,
        srcs = outs,
    )
