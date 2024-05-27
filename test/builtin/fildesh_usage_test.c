/** Ensure that builtins return bad statuses for invalid usage.**/
#include "include/fildesh/fildesh_compat_fd.h"
#include "include/fildesh/fildesh_compat_file.h"
#include <assert.h>
#include <stdlib.h>

static void bestmatch_usage_tests(const char* fildesh_exe, const char* bad_filename) {
  int istat;
  /* Can't open files.*/
  istat = fildesh_compat_fd_spawnlp_wait(
      0, 1, 2, NULL, fildesh_exe, "-as", "bestmatch",
      "-x-lut", bad_filename, NULL);
  assert(istat == 66);

  istat = fildesh_compat_fd_spawnlp_wait(
      0, 1, 2, NULL, fildesh_exe, "-as", "bestmatch",
      "-x", bad_filename, NULL);
  assert(istat == 66);

  istat = fildesh_compat_fd_spawnlp_wait(
      0, 1, 2, NULL, fildesh_exe, "-as", "bestmatch",
      "-o", bad_filename, NULL);
  assert(istat == 73);
}

static void
elastic_pthread_usage_tests(const char* fildesh_exe, const char* bad_filename)
{
  int istat;
  istat = fildesh_compat_fd_spawnlp_wait(
      0, 1, 2, NULL, fildesh_exe, "-as", "elastic_pthread",
      "-x", NULL);
  assert(istat == 66);

  istat = fildesh_compat_fd_spawnlp_wait(
      0, 1, 2, NULL, fildesh_exe, "-as", "elastic_pthread",
      "-x", bad_filename, NULL);
  assert(istat == 66);

  istat = fildesh_compat_fd_spawnlp_wait(
      0, 1, 2, NULL, fildesh_exe, "-as", "elastic_pthread",
      "-o", bad_filename, NULL);
  assert(istat == 73);
}

static void
execfd_usage_tests(const char* fildesh_exe, const char* bad_filename)
{
  int istat;
  istat = fildesh_compat_fd_spawnlp_wait(
      0, 1, 2, NULL, fildesh_exe, "-as", "execfd",
      "-stdin", bad_filename, NULL);
  assert(istat == 66);

  istat = fildesh_compat_fd_spawnlp_wait(
      0, 1, 2, NULL, fildesh_exe, "-as", "execfd",
      "-stdout", bad_filename, NULL);
  assert(istat == 73);

  istat = fildesh_compat_fd_spawnlp_wait(
      0, 1, 2, NULL, fildesh_exe, "-as", "execfd",
      "-o?", bad_filename, NULL);
  assert(istat == 73);
}

static void
ujoin_usage_tests(const char* fildesh_exe, const char* bad_filename) {
  int istat;
  /* Second open should fail.*/
  istat = fildesh_compat_fd_spawnlp_wait(
      0, 1, 2, NULL, fildesh_exe, "-as", "ujoin",
      "-x-lut", "-", "-x", "-", NULL);
  assert(istat == 66);

  /* Invalid output files.*/
  istat = fildesh_compat_fd_spawnlp_wait(
      0, 1, 2, NULL, fildesh_exe, "-as", "ujoin",
      "-o", bad_filename,  NULL);
  assert(istat == 73);

  istat = fildesh_compat_fd_spawnlp_wait(
      0, 1, 2, NULL, fildesh_exe, "-as", "ujoin",
      "-o-not-found", bad_filename,  NULL);
  assert(istat == 73);

  istat = fildesh_compat_fd_spawnlp_wait(
      0, 1, 2, NULL, fildesh_exe, "-as", "ujoin",
      "-o-conflicts", bad_filename,  NULL);
  assert(istat == 73);
}

static void
zec_usage_tests(const char* fildesh_exe, const char* bad_filename) {
  const char* good_filename = "/dev/null";
  int istat;

  /* Cannot open output file.*/
  istat = fildesh_compat_fd_spawnlp_wait(
      0, 1, 2, NULL, fildesh_exe, "-as", "zec",
      "-o", bad_filename, NULL);
  assert(istat == 73);

  /* Cannot open input file.*/
  istat = fildesh_compat_fd_spawnlp_wait(
      0, 1, 2, NULL, fildesh_exe, "-as", "zec",
      "-x", bad_filename, NULL);
  assert(istat == 66);

  istat = fildesh_compat_fd_spawnlp_wait(
      0, 1, 2, NULL, fildesh_exe, "-as", "zec",
      bad_filename, NULL);
  assert(istat == 66);

  istat = fildesh_compat_fd_spawnlp_wait(
      0, 1, 2, NULL, fildesh_exe, "-as", "zec",
      good_filename, good_filename, bad_filename, NULL);
  assert(istat == 66);

  istat = fildesh_compat_fd_spawnlp_wait(
      0, 1, 2, NULL, fildesh_exe, "-as", "zec",
      "/", "/", good_filename, good_filename, bad_filename, NULL);
  assert(istat == 66);
}

int main(int argc, char** argv) {
  const char* fildesh_exe = argv[1];
  char* bad_filename;
  assert(argc == 2 && fildesh_exe && "Need fildesh executable.");

  bad_filename = fildesh_compat_file_catpath(fildesh_exe, "no_file_here");
  assert(bad_filename);

  bestmatch_usage_tests(fildesh_exe, bad_filename);
  elastic_pthread_usage_tests(fildesh_exe, bad_filename);
  execfd_usage_tests(fildesh_exe, bad_filename);
  ujoin_usage_tests(fildesh_exe, bad_filename);
  zec_usage_tests(fildesh_exe, bad_filename);

  free(bad_filename);
  return 0;
}
