
#include "lace.h"
#include "lace_compat_fd.h"
#include "lace_compat_file.h"
#include "lace_compat_sh.h"
#include "lace_compat_string.h"
#include "utilace.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static
  void
show_usage()
{
#define f(s)  fputs(s, stderr); fputc('\n', stderr);
  f("Usage: execfd [OPTIONS] [argi]* -- [arg|fd]*");
  f("  Each argi is an index that has a file descriptor in it.");
  f("  Index zero is the executable name." );
  f(" OPTIONS:");
  f("  -exe filename -- Name of executable to write. Only used when index 0 is present.");
  f("  -stdin filename -- Standard input for the spawned process.");
  f("  -stdout filename -- Standard output for the spawned process.");
  f("  -inheritfd fd -- File descriptor for spawned process to inherit.");
  f("    Only useful when invoked as builtin.");
#undef f
}

/** Read from the file descriptor /in/ and write to file /name/.
 * If the input stream contains no data, the file will not be
 * written (or overwritten).
 **/
static
  void
pipe_to_file(lace_fd_t fd, const char* name)
{
  LaceX* in = open_fd_LaceX(fd);
  LaceO* out = NULL;

  if (in) {
    read_LaceX(in);
    if (in->size > 0) {
      out = open_LaceOF(name);
    }
  }

  if (in && out) {
    for (; in->size > 0; read_LaceX(in)) {
      memcpy(grow_LaceO(out, in->size), in->at, in->size);
      maybe_flush_LaceO(out);
      in->size = 0;
    }
  }
  close_LaceX(in);
  close_LaceO(out);
}


static
  char*
readin_fd(lace_fd_t fd, bool scrap_newline)
{
  LaceX* in = open_fd_LaceX(fd);
  char* s = slurp_LaceX(in);
  if (scrap_newline && in->size >= 1 && s[in->size-1] == '\n') {
    s[in->size-1] = '\0';
    if (in->size >= 2 && s[in->size-2] == '\r') {
      s[in->size-2] = '\0';
    }
  }
  in->at = NULL;
  in->alloc_lgsize = 0;
  close_LaceX(in);
  return s;
}

  int
lace_builtin_execfd_main(unsigned argc, char** argv,
                         LaceX** inputv, LaceO** outputv)
{
  int exstatus = 0;
  unsigned argi;
  unsigned off = 0;
  unsigned i;
  lace_fd_t stdin_fd = 0;
  lace_fd_t stdout_fd = 1;
  unsigned inherit_count;
  lace_fd_t* fds_to_inherit;
  unsigned char* bt;
  char* exe = NULL;
  char** spawn_argv;

  assert(!inputv);
  assert(!outputv);

  if (argc < 3) {show_usage(); return 64;}

  fds_to_inherit = (lace_fd_t*) malloc(sizeof(lace_fd_t) * (argc-2));
  inherit_count = 0;
  fds_to_inherit[inherit_count] = -1;

  bt = (unsigned char*) malloc(argc);
  memset(bt, 0, argc);

  spawn_argv = (char**)malloc(sizeof(char*) * (argc+1));
  for (i = 0; i < argc+1; ++i) {
    spawn_argv[i] = NULL;
  }

  argi = 1;
  while (argv[argi] && 0 != strcmp(argv[argi], "--") && exstatus == 0) {
    if (0 == strcmp(argv[argi], "-exe")) {
      exe = argv[++argi];
    } else if (0 == strcmp(argv[argi], "-stdin")) {
      stdin_fd = lace_arg_open_readonly(argv[++argi]);
      if (stdin_fd < 0) {
        lace_log_errorf("Cannot open -stdin: %s", argv[argi]);
        exstatus = 66;
      }
    } else if (0 == strcmp(argv[argi], "-stdout")) {
      stdout_fd = lace_arg_open_writeonly(argv[++argi]);
      if (stdout_fd < 0) {
        lace_log_errorf("Cannot open -stdout: %s", argv[argi]);
        exstatus = 73;
      }
    } else if (0 == strcmp(argv[argi], "-inheritfd")) {
      lace_fd_t fd = -1;
      if (lace_parse_int(&fd, argv[++argi]) && fd >= 0) {
        fds_to_inherit[inherit_count++] = fd;
        fds_to_inherit[inherit_count] = -1;
      } else {
        lace_log_errorf("Cannot parse -inheritfd: %s", argv[argi]);
        exstatus = 64;
      }
    } else {
      int idx = 0;
      if (lace_parse_int(&idx, argv[argi]) &&
          idx >= 0 && (unsigned)idx < argc)
      {
        bt[idx] = 1;
      } else {
        lace_log_errorf("Cannot parse index from arg: %s", argv[argi]);
        exstatus = 64;
      }
    }
    ++ argi;
  }
  off = argi+1;

  if (exstatus == 0) {
    if (!argv[off-1] || !argv[off]) {
      exstatus = 64;
    }
  }

  if (exstatus == 0) {
    for (i = argc-off; i < argc; ++i) {
      if (bt[i] != 0) {
        lace_log_errorf("Index %u is out of range.", argv[i]);
        exstatus = 64;
      }
    }
  }

  for (i = 0; i < argc-off && exstatus == 0; ++i) {
    int fd = -1;
    spawn_argv[off+i] = argv[off+i];
    if (bt[i] == 0) {continue;}
    if (!lace_parse_int(&fd, argv[off+i]) || fd < 0) {
      lace_log_errorf("Cannot parse fd from arg: %s", argv[off+i]);
      exstatus = 64;
    } else if (i == 0) {
      if (exe) {
        pipe_to_file(fd, exe);
        spawn_argv[off+i] = lace_compat_string_duplicate(exe);
        lace_compat_file_chmod_u_rwx(exe, 1, 1, 1);
      } else {
        lace_log_error("Need to provide -exe argument.");
        exstatus = 64;
      }
    } else {
      spawn_argv[off+i] = readin_fd(fd, true);
    }
  }
  free(bt);

  if (exstatus != 0) {
    show_usage();
  }
#if defined(LACE_BUILTIN_LIBRARY) || defined(UNIT_TESTING)
  else if (lace_specific_util(argv[off])) {
    spawn_argv[off-2] = argv[0];
    spawn_argv[off-1] = (char*)"-as";
    exstatus = lace_compat_fd_spawnvp_wait(
        stdin_fd, stdout_fd, 2, fds_to_inherit,
        (const char**)&spawn_argv[off-2]);
  } else {
    exstatus = lace_compat_fd_spawnvp_wait(
        stdin_fd, stdout_fd, 2, fds_to_inherit,
        (const char**)&spawn_argv[off]);
  }
#else
  else {
    exstatus = -1;
    lace_compat_fd_move_to(0, stdin_fd);
    lace_compat_fd_move_to(1, stdout_fd);
    lace_compat_sh_exec((const char**)&spawn_argv[off]);
  }
#endif

  free(fds_to_inherit);
  for (argi = off; argi < argc; ++argi) {
    if (spawn_argv[argi] && spawn_argv[argi] != argv[argi]) {
      free(spawn_argv[argi]);
    }
  }
  free(spawn_argv);

  if (exstatus < 0) {exstatus = 126;}
  return exstatus;
}

#if !defined(LACE_BUILTIN_LIBRARY) && !defined(UNIT_TESTING)
  int
main(int argc, char** argv)
{
  return lace_builtin_execfd_main((unsigned)argc, argv, NULL, NULL);
}
#endif
