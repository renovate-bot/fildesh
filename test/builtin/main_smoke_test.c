#include <assert.h>

#include "include/fildesh/fildesh_compat_fd.h"
#include "src/builtin/fildesh_builtin.h"

static
  int
prepare_argv(
    char** argv, int argc_max,
    const unsigned char* argstr, size_t n)
{
  int argc = 0;
  FildeshX in[1];
  FildeshX slice;
  *in = FildeshX_of_bytestring(argstr, n+1);
  for (slice = slicechr_FildeshX(in, '\0');
       slice.at;
       slice = slicechr_FildeshX(in, '\0'))
  {
    argv[argc++] = slice.at;
    assert(argc < argc_max);
  }
  argv[argc] = NULL;
  return argc;
}

static void prepare_stdio_fds() {
  FildeshCompat_fd fds[2];
  fildesh_compat_fd_pipe(&fds[0], &fds[1]);
  fildesh_compat_fd_move_to(0, fds[0]);
  fildesh_compat_fd_move_to(1, fds[1]);
}

static
  int
builtin_main_test(const unsigned char* argstr, size_t n)
{
  char* argv[11];
  unsigned argc = prepare_argv(argv, 10, argstr, n);
  prepare_stdio_fds();
  return fildesh_builtin_main(argv[0], argc, argv);
}
#define MAIN_TEST(e, s)  assert(e == builtin_main_test(fildesh_bytestrlit(s)))

static void builtin_cmp_test() {
  MAIN_TEST(0, "cmp\0/dev/null\0/dev/null");
  MAIN_TEST(0, "cmptxt\0/dev/null\0/dev/null");

  MAIN_TEST(64, "cmp\0/dev/null\0/dev/null\0too_many_files");
  MAIN_TEST(64, "cmptxt\0/dev/null\0/dev/null\0too_many_files");

  MAIN_TEST(64, "cmp\0-");
  MAIN_TEST(64, "cmptxt\0-");
}

int main() {
  MAIN_TEST(64, "add\0invalid_argument");

  /* Need -x-lut.*/
  MAIN_TEST(64, "bestmatch\0-x\0/dev/null");
  /* Missing -d arg.*/
  MAIN_TEST(64, "bestmatch\0-x-lut\0/dev/null\0-d");
  /* Bad flag.*/
  MAIN_TEST(64, "bestmatch\0--invalid-flag");

  builtin_cmp_test();

  MAIN_TEST(64, "delimend");

  MAIN_TEST(64, "elastic_pthread\0-o");

  MAIN_TEST(64, "execfd");
  MAIN_TEST(64, "execfd\0--");
  MAIN_TEST(64, "execfd\0""3\0--\0missing\0index\0three");

  MAIN_TEST(64, "replace_string");

  MAIN_TEST(64, "sponge\0too\0many");

  MAIN_TEST(64, "sxpb2json\0invalid_argument");
  MAIN_TEST(64, "sxpb2txtpb\0invalid_argument");
  MAIN_TEST(64, "sxpb2yaml\0invalid_argument");

  MAIN_TEST(64, "time2sec\0no_flagless_arg_is_valid");
  MAIN_TEST(64, "time2sec\0-w");

  MAIN_TEST(64, "transpose\0invalid_argument");
  /* Delimiter flag needs a value.*/
  MAIN_TEST(64, "transpose\0-d");

  /* Not quite enough args.*/
  MAIN_TEST(64, "ujoin\0-x\0-");
  /* Invalid argument.*/
  MAIN_TEST(64, "ujoin\0invalid_argument");
  /* Delimiter missing.*/
  MAIN_TEST(64, "ujoin\0-d");
  /* No default record given.*/
  MAIN_TEST(64, "ujoin\0-x-lut\0/dev/null\0-x\0/dev/null\0-p");

  /* Need to give a command.*/
  MAIN_TEST(64, "xargz\0--");

  /* Help.*/
  MAIN_TEST(1, "zec\0-h");
  /* Need filename after -x.*/
  MAIN_TEST(64, "zec\0-o\0/dev/null\0-x");
  /* Need string after -unless.*/
  MAIN_TEST(64, "zec\0-unless");

  return 0;
}
