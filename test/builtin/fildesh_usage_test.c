/** Ensure that builtins return bad statuses for invalid usage.**/
#include "include/fildesh/fildesh_compat_fd.h"
#include "include/fildesh/fildesh_compat_file.h"
#include <assert.h>
#include <stdlib.h>

static void
ujoin_usage_tests(const char* fildesh_exe) {
  int istat;
  /* Second open should fail.*/
  istat = fildesh_compat_fd_spawnlp_wait(
      0, 1, 2, NULL, fildesh_exe, "-as", "ujoin",
      "-x-lut", "-", "-x", "-", NULL);
  assert(istat == 66);
}

int main(int argc, char** argv) {
  const char* fildesh_exe = argv[1];
  assert(argc == 2 && fildesh_exe && "Need fildesh executable.");

  ujoin_usage_tests(fildesh_exe);

  return 0;
}
