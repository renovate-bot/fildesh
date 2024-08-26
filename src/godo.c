#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fildesh/fildesh.h>

#include "include/fildesh/fildesh_compat_errno.h"
#include "include/fildesh/fildesh_compat_fd.h"
#include "include/fildesh/fildesh_compat_sh.h"

  int
fildesh_main_command(unsigned argc, char** argv)
{
  int exstatus = -1;
  unsigned argi = 1;
  if (argi < argc && 0 == strcmp("--", argv[argi])) {
    argi += 1;
  }
  if (argi < argc) {
    exstatus = fildesh_compat_fd_spawnvp_wait(
        0, 1, 2, NULL, (const char**)&argv[argi]);
  }
  if (exstatus < 0) {exstatus = 127;}
  return exstatus;
}

  int
fildesh_main_godo(unsigned argc, char** argv)
{
  const char* directory = argv[1];
  if (argc < 3) {
    fildesh_log_error("Usage: godo PATH COMMAND [ARG...]");
    return 64;
  }

  if (directory[0] == '.' && directory[1] == '\0') {
    /* Skip.*/
  }
  else if (0 != fildesh_compat_sh_chdir(directory)) {
    fildesh_log_errorf("Failed to chdir() to: %s", directory);
    return 66;
  }

  argv[1] = argv[0];
  return fildesh_main_command(argc-1, &argv[1]);
}

#ifndef FILDESH_BUILTIN_LIBRARY
  int
main(int argc, char** argv)
{
  return fildesh_main_godo(argc, argv);
}
#endif
