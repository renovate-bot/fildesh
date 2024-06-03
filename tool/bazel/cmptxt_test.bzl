
def _cmptxt_test_impl(ctx):
  executable = ctx.actions.declare_file(ctx.label.name)
  ctx.actions.symlink(
      output=executable,
      target_file=ctx.file._cmptxt,
      is_executable=True,
  )
  runfiles = ctx.runfiles(files=ctx.files.srcs)
  return DefaultInfo(executable=executable, runfiles=runfiles)


_cmptxt_test = rule(
    implementation = _cmptxt_test_impl,
    test = True,
    attrs = {
        "srcs": attr.label_list(allow_files=True),
        "_cmptxt": attr.label(
            default = Label("//tool:cmptxt"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
)


def cmptxt_test(name, srcs, size="small"):
  if type(srcs) != type([]) or len(srcs) != 2:
    fail("The srcs arg must be a list of exactly 2 text files.")

  _cmptxt_test(
      name=name,
      args=[
          "$(location " + srcs[0] + ")",
          "$(location " + srcs[1] + ")",
      ],
      srcs=srcs,
      size=size,
  )
