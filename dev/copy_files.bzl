def copy_files(
        name,
        srcs = [],
        outs = []):
    """
    We have dependencies on data files outside of our project, and the filegroups
    provided by that project include extra stuff we don't want. This function
    will extract the provided filenames from the provided filegroups and copy them
    into a temporary output directory for use by subsequent build rules.
    Args:
      name: string, name of the output filegroup to generate
      filenames: list of strings, the file names we want to use
      filegroups: list of strings, the provided filegroups that contain the
                  filenames
    """
    native.genrule(
        name = "%s_copy" % name,
        srcs = srcs,
        outs = outs,
        cmd = "(for file in $(SRCS) ; do cp $$file $(@D) ; done)",
        message = "copying files for %s" % name,
    )
    native.filegroup(
        name = name,
        srcs = outs,
    )
