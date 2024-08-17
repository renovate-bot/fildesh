#include <assert.h>
#include <stdlib.h>

#include <fildesh/fildesh.h>

#include "include/fildesh/fildesh_compat_fd.h"
#include "include/fildesh/fildesh_compat_file.h"
#include "fildesh_tool.h"

int
fildesh_builtin_cmptxt_main(
    unsigned argc, char** argv,
    FildeshX** inputv, FildeshO** outputv);

typedef struct CallbackParam CallbackParam;
struct CallbackParam {
  FildeshX lhs[1];
  FildeshX rhs[1];
  int exstatus;
};

FILDESH_TOOL_PIPEM_CALLBACK(run_cmptxt, in_fd, out_fd, CallbackParam*, pa) {
  const unsigned argc = 3;
  char* argv[] = {(char*)"cmptxt", (char*)"", (char*)"", NULL};
  FildeshX* inputv[] = {NULL, NULL, NULL, NULL};
  FildeshO* outputv[] = {NULL, NULL, NULL, NULL};
  (void)in_fd;
  inputv[1] = pa->lhs;
  inputv[2] = pa->rhs;
  outputv[0] = open_fd_FildeshO(out_fd);
  pa->exstatus = fildesh_builtin_cmptxt_main(argc, argv, inputv, outputv);
}

static void
print_skip_output(FildeshO* out, FildeshX* expect, const char* s, size_t n) {
  put_bytestring_FildeshO(out, (const unsigned char*)s, n);
  flush_FildeshO(out);
  skip_bytestring_FildeshX(expect, (const unsigned char*)s, n);
}

static void
comparison_test()
{
  FildeshO* err_out = open_FildeshOF("/dev/stderr");
  char* output_data = NULL;
  size_t output_size;

#define expect_cmptxt(expect_txt, lhs_txt, rhs_txt) do { \
  DECLARE_STRLIT_FildeshX(expect, expect_txt); \
  int expect_status = avail_FildeshX(expect) ? 1 : 0; \
  CallbackParam pa[1]; \
  *pa->lhs = FildeshX_of_strlit(lhs_txt); \
  *pa->rhs = FildeshX_of_strlit(rhs_txt); \
  output_size = fildesh_tool_pipem(0, NULL, run_cmptxt, pa, &output_data); \
  print_skip_output(err_out, expect, output_data, output_size); \
  assert(pa->exstatus == expect_status); \
  assert(!avail_FildeshX(expect)); \
} while (0)

  expect_cmptxt("", "a\nb\nc\nd", "a\nb\nc\nd");
  expect_cmptxt("", "a\nb\nc\nd\n", "a\nb\nc\nd");
  expect_cmptxt("", "a\nb\nc\nd", "a\nb\nc\nd\n");
  expect_cmptxt("", "a\r\nb\nc\nd", "a\nb\r\nc\nd");
  expect_cmptxt(
      "Difference found. No RHS line 4.\n LHS: d\n",
      "a\nb\nc\nd",
      "a\nb\nc");
  expect_cmptxt(
      "Difference found. No LHS line 4.\n RHS: d\n",
      "a\nb\nc\n",
      "a\nb\nc\nd\n");
  expect_cmptxt(
      "Difference found on line 5.\n LHS: d\n RHS: e\n",
      "a\nb\n\nc\nd\n",
      "a\nb\n\nc\ne\n");
  expect_cmptxt(
      "Difference found on line 3.\n LHS: \n RHS: c\n",
      "a\nb\n\nc\nd",
      "a\nb\nc\nd");
  expect_cmptxt(
      "Difference found. No LHS line 5.\n RHS: \n",
      "a\nb\nc\nd",
      "a\nb\nc\nd\n\n");

#undef expect_cmptxt
  free(output_data);
  close_FildeshO(err_out);
}

static void
reset_argv(unsigned argc, char** argv, FildeshX** inputv, FildeshO** outputv) {
  static FildeshX dummy_in[1];
  static FildeshO dummy_out[1];
  unsigned i;
  *dummy_in = FildeshX_of_strlit("any\ncontent\n");
  *dummy_out = default_FildeshO();
  argv[0] = (char*)"cmptxt";
  inputv[0] = dummy_in;
  outputv[0] = dummy_out;
  for (i = 1; i <= argc; ++i) {
    argv[i] = NULL;
    inputv[i] = NULL;
    outputv[i] = NULL;
  }
}

static void
usage_test(char* bad_filename)
{
  unsigned argc;
  char* argv[5];
  FildeshX* inputv[5];
  FildeshO* outputv[5];
  int exstatus;

  argc = 3;
  reset_argv(argc, argv, inputv, outputv);
  argv[1] = (char*)"-o";
  argv[2] = bad_filename;
  exstatus = fildesh_builtin_cmptxt_main(argc, argv, inputv, outputv);
  assert(exstatus == 73);
  assert(!inputv[0]);
  assert(!outputv[0]);

  /* First open should fail.*/
  argc = 3;
  reset_argv(argc, argv, inputv, outputv);
  argv[1] = bad_filename;
  argv[2] = (char*)"-";
  exstatus = fildesh_builtin_cmptxt_main(argc, argv, inputv, outputv);
  assert(exstatus == 66);

  /* Second open should fail.*/
  argc = 3;
  reset_argv(argc, argv, inputv, outputv);
  argv[1] = (char*)"-";
  argv[2] = (char*)"-";
  exstatus = fildesh_builtin_cmptxt_main(argc, argv, inputv, outputv);
  assert(exstatus == 66);
}

int main(int argc, char** argv) {
  char* bad_filename = fildesh_compat_file_catpath(argv[0], "no_file_here");
  assert(argc == 1);

  comparison_test();
  usage_test(bad_filename);

  free(bad_filename);
  return 0;
}
