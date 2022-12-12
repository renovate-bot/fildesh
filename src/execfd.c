
#include <fildesh/fildesh.h>
#include "fildesh_builtin.h"
#include "include/fildesh/fildesh_compat_fd.h"
#include "include/fildesh/fildesh_compat_file.h"
#include "include/fildesh/fildesh_compat_sh.h"
#include "include/fildesh/fildesh_compat_string.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static
  void
show_usage()
{
#define f(s)  fputs(s, stderr); fputc('\n', stderr);
  f("Usage: execfd [OPTIONS] FORMAT -- [arg|fd]*");
  f(" FORMAT uses 2 bytes to represent the type of each argument and whether to concatenate them.");
  f("   a -- Literal string argument.");
  f("   _ -- Readable file descriptor (integer).");
  f("   + -- Concatenate args.");
  f(" OPTIONS:");
  f("  -exe filename -- Name of executable to write. Only used when index 0 is present.");
  f("  -stdin filename -- Standard input for the spawned process.");
  f("  -stdout filename -- Standard output for the spawned process.");
  f("  -inheritfd fd -- File descriptor for spawned process to inherit.");
  f("    Only useful when invoked as builtin.");
  f("  -waitfd fd -- Wait on this file descriptor to close.");
  f("  -exitfd fd -- Close this file descriptor on exit.");
  f("  -o? filename -- Print exit status to this file upon exit.");
#undef f
}

/** Read from the file descriptor /in/ and write to file /name/.
 * If the input stream contains no data, the file will not be
 * written (or overwritten).
 **/
static
  int
pipe_to_file(fildesh_fd_t fd, const char* name)
{
  FildeshX* in = open_fd_FildeshX(fd);
  FildeshO* out = NULL;
  int exstatus = 0;

  if (!in) {
    exstatus = 66;
    fildesh_log_errorf("Cannot open input fd: %d", fd);
  }
  if (exstatus == 0) {
    read_FildeshX(in);
    if (in->size == 0) {
      exstatus = 66;
      fildesh_log_errorf("Empty input fd: %d", fd);
    }
  }
  if (exstatus == 0) {
    out = open_FildeshOF(name);
    if (!out) {
      exstatus = 73;
      fildesh_log_errorf("Cannot open output file: %s", name);
    }
  }

  if (exstatus == 0) {
    for (; in->size > 0; read_FildeshX(in)) {
      memcpy(grow_FildeshO(out, in->size), in->at, in->size);
      maybe_flush_FildeshO(out);
      in->size = 0;
    }
  }
  close_FildeshX(in);
  close_FildeshO(out);
  return exstatus;
}


static
  char*
readin_fd(FildeshO* buf, fildesh_fd_t fd, bool scrap_newline)
{
  FildeshX* in = open_fd_FildeshX(fd);
  char* s = slurp_FildeshX(in);
  if (scrap_newline && in->size >= 1 && s[in->size-1] == '\n') {
    s[in->size-1] = '\0';
    if (in->size >= 2 && s[in->size-2] == '\r') {
      s[in->size-2] = '\0';
    }
  }
  puts_FildeshO(buf, s);
  close_FildeshX(in);
  return s;
}

static
  void
next_spawn_arg(size_t* spawn_offsets,
               unsigned* spawn_argc,
               FildeshO* spawn_buf)
{
  putc_FildeshO(spawn_buf, '\0');
  *spawn_argc += 1;
  spawn_offsets[*spawn_argc] = spawn_buf->size;
}

  int
fildesh_builtin_execfd_main(unsigned argc, char** argv,
                            FildeshX** inputv, FildeshO** outputv)
{
  int exstatus = 0;
  unsigned argi;
  unsigned off = 0;
  unsigned i;
  fildesh_fd_t stdin_fd = 0;
  fildesh_fd_t stdout_fd = 1;
  unsigned inherit_count;
  fildesh_fd_t* fds_to_inherit;
  unsigned exitfd_count;
  fildesh_fd_t* exitfds;
  FildeshO* status_out = NULL;
  char* exe = NULL;
  char** spawn_argv = NULL;
  unsigned spawn_argc = 0;
  size_t* spawn_offsets;
  FildeshO spawn_buf = DEFAULT_FildeshO;
  const char* arg_fmt = NULL;

  assert(!inputv);
  assert(!outputv);

  if (argc < 3) {show_usage(); return 64;}

  fds_to_inherit = (fildesh_fd_t*) malloc(sizeof(fildesh_fd_t) * (argc-2));
  inherit_count = 0;
  fds_to_inherit[inherit_count] = -1;

  exitfds = (fildesh_fd_t*) malloc(sizeof(fildesh_fd_t) * (argc / 2));
  exitfd_count = 0;

  argi = 1;
  while (argv[argi] && 0 != strcmp(argv[argi], "--") && exstatus == 0) {
    if (0 == strcmp(argv[argi], "-exe")) {
      exe = argv[++argi];
    } else if (0 == strcmp(argv[argi], "-stdin")) {
      stdin_fd = fildesh_arg_open_readonly(argv[++argi]);
      if (stdin_fd < 0) {
        fildesh_log_errorf("Cannot open -stdin: %s", argv[argi]);
        exstatus = 66;
      }
    } else if (0 == strcmp(argv[argi], "-stdout")) {
      stdout_fd = fildesh_arg_open_writeonly(argv[++argi]);
      if (stdout_fd < 0) {
        fildesh_log_errorf("Cannot open -stdout: %s", argv[argi]);
        exstatus = 73;
      }
    } else if (0 == strcmp(argv[argi], "-inheritfd")) {
      fildesh_fd_t fd = -1;
      if (fildesh_parse_int(&fd, argv[++argi]) && fd >= 0) {
        fds_to_inherit[inherit_count++] = fd;
        fds_to_inherit[inherit_count] = -1;
      } else {
        fildesh_log_errorf("Cannot parse -inheritfd: %s", argv[argi]);
        exstatus = 64;
      }
    } else if (0 == strcmp(argv[argi], "-waitfd")) {
      fildesh_fd_t fd = -1;
      if (fildesh_parse_int(&fd, argv[++argi]) && fd >= 0) {
        wait_close_FildeshX(open_fd_FildeshX(fd));
      } else {
        fildesh_log_errorf("Cannot parse -waitfd: %s", argv[argi]);
        exstatus = 64;
      }
    } else if (0 == strcmp(argv[argi], "-exitfd")) {
      fildesh_fd_t fd = -1;
      if (fildesh_parse_int(&fd, argv[++argi]) && fd >= 0) {
        exitfds[exitfd_count++] = fildesh_compat_fd_claim(fd);
      } else {
        fildesh_log_errorf("Cannot parse -exitfd: %s", argv[argi]);
        exstatus = 64;
      }
    } else if (0 == strcmp(argv[argi], "-o?")) {
      status_out = open_arg_FildeshOF(++argi, argv, outputv);
      if (!status_out) {
        fildesh_log_errorf("Cannot open -o?: %s", argv[argi]);
        exstatus = 73;
      }
    } else {
      arg_fmt = argv[argi];
    }
    ++ argi;
  }
  off = argi+1;

  if (exstatus == 0) {
    if (!argv[off-1] || !argv[off] || !arg_fmt) {
      exstatus = 64;
    }
    else if (strlen(arg_fmt) != 2*(argc-off)-1) {
      fildesh_log_errorf("Format string %s length is off by %d.",
                         arg_fmt,
                         (int)strlen(arg_fmt) - (int)(2*(argc-off)-1));
      fildesh_log_errorf("argc:%u off:%u", argc, off);
      exstatus = 64;
    }
  }

  if (exstatus == 0) {
    for (i = 0; i < argc-off; ++i) {
      if (!memchr("ax", arg_fmt[2*i], 2)) {
        fildesh_log_errorf("Unrecognized format character %c.", arg_fmt[2*i]);
        exstatus = 64;
      }
      if (!memchr("_+\0", arg_fmt[2*i+1], 3)) {
        fildesh_log_errorf("Unrecognized format delimiter %c.", arg_fmt[2*i+1]);
        exstatus = 64;
      }
    }
  }

  spawn_offsets = (size_t*)malloc(sizeof(size_t) * argc);

  if (exstatus == 0 && fildesh_specific_util(argv[off])) {
    puts_FildeshO(&spawn_buf, argv[0]);
    next_spawn_arg(spawn_offsets, &spawn_argc, &spawn_buf);
    puts_FildeshO(&spawn_buf, "-as");
    next_spawn_arg(spawn_offsets, &spawn_argc, &spawn_buf);
  }

  for (i = 0; i < argc-off && exstatus == 0; ++i) {
    int fd = -1;

    if (i == 0) {
      spawn_offsets[0] = 0;
    } else {
      if (arg_fmt[2*i-1] == '_') {
        next_spawn_arg(spawn_offsets, &spawn_argc, &spawn_buf);
      }
    }

    if (arg_fmt[2*i] == 'a') {
      puts_FildeshO(&spawn_buf, argv[off+i]);
      continue;
    }

    if (!fildesh_parse_int(&fd, argv[off+i]) || fd < 0) {
      fildesh_log_errorf("Cannot parse fd from arg: %s", argv[off+i]);
      exstatus = 64;
    } else if (i == 0) {
      if (exe) {
        exstatus = pipe_to_file(fd, exe);
        if (exstatus == 0) {
          puts_FildeshO(&spawn_buf, exe);
        }
      } else {
        fildesh_log_error("Need to provide -exe argument.");
        exstatus = 64;
      }
    } else {
      readin_fd(&spawn_buf, fd, true);
    }
  }

  if (exstatus == 0) {
    putc_FildeshO(&spawn_buf, '\0');
    spawn_argc += 1;
    spawn_argv = (char**)malloc(sizeof(char*) * (spawn_argc+1));
    for (i = 0; i < spawn_argc; ++i) {
      spawn_argv[i] = &spawn_buf.at[spawn_offsets[i]];
    }
    spawn_argv[spawn_argc] = NULL;
    free(spawn_offsets);
    spawn_offsets = NULL;
  }
  if (exstatus == 0 && exe) {
    fildesh_compat_file_chmod_u_rwx(exe, 1, 1, 1);
  }

  if (exstatus != 0) {
    show_usage();
  } else {
#if defined(FILDESH_BUILTIN_LIBRARY) || defined(UNIT_TESTING)
    exstatus = fildesh_compat_fd_spawnvp_wait(
        stdin_fd, stdout_fd, 2, fds_to_inherit,
        (const char**)spawn_argv);
#else
    fildesh_compat_fd_move_to(0, stdin_fd);
    fildesh_compat_fd_move_to(1, stdout_fd);
    if (exitfd_count == 0 && !status_out) {
      exstatus = -1;
      fildesh_compat_sh_exec((const char**)&spawn_argv[off]);
    } else {
      exstatus = fildesh_compat_sh_spawn((const char**)&spawn_argv[off]);
    }
#endif
    if (status_out && exstatus >= 0) {
      print_int_FildeshO(status_out, exstatus);
      exstatus = 0;
    }
  }

  if (spawn_offsets) {
    free(spawn_offsets);
  }
  if (spawn_argv) {
    free(spawn_argv);
  }
  close_FildeshO(&spawn_buf);

  if (status_out) {
    close_FildeshO(status_out);
  }

  for (i = 0; i < exitfd_count; ++i) {
    fildesh_compat_fd_close(exitfds[i]);
  }
  free(exitfds);

  free(fds_to_inherit);

  if (exstatus < 0) {exstatus = 126;}
  return exstatus;
}

#if !defined(FILDESH_BUILTIN_LIBRARY) && !defined(UNIT_TESTING)
  int
main(int argc, char** argv)
{
  return fildesh_builtin_execfd_main((unsigned)argc, argv, NULL, NULL);
}
#endif
