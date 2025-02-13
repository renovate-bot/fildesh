#define FILDESH_LOG_TRACE_ON 1
#include "src/builtin/fildesh_builtin.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>

#include "include/fildesh/fildesh_compat_file.h"

static int call_sut_main(const char* name, ...)
{
  unsigned argc = 0;
  const char* argv[20];
  va_list argp;

  argv[0] = name;
  va_start(argp, name);
  while (argv[argc]) {
    argc += 1;
    assert(argc < (sizeof(argv)/sizeof(*argv)));
    argv[argc] = va_arg(argp, const char*);
  }
  va_end(argp);
  return fildesh_builtin_expect_failure_main(argc, (char**)argv, NULL, NULL);
}

int main(int argc, char** argv) {
  const char* literal_0_filename;
  const char* literal_127_filename;
  const char* literal_hello_filename;
  char* bad_filename;
  int istat;

  assert(argc == 4);
  literal_0_filename = argv[1];
  literal_127_filename = argv[2];
  literal_hello_filename = argv[3];
  bad_filename = fildesh_compat_file_catpath(
      literal_0_filename, "no_file_here");

  fildesh_log_info("Testing failure cases.");
  fildesh_log_warning("Expect lots of error messages.");


  fildesh_log_trace("Testing obvious usage errors.");

  /* No args.*/
  istat = call_sut_main("expect_failure", NULL);
  assert(istat == 64);

  /* Unknown arg.*/
  istat = call_sut_main("expect_failure", "-unknownarg", NULL);
  assert(istat == 64);

  /* Cannot parse status arg.*/
  istat = call_sut_main("expect_failure", "-status", "notanumber", NULL);
  assert(istat == 65);

  /* Cannot parse status file.*/
  istat = call_sut_main("expect_failure", "-x?", literal_hello_filename, NULL);
  assert(istat == 65);

  /* Cannot open status file.*/
  istat = call_sut_main("expect_failure", "-x?", bad_filename, NULL);
  assert(istat == 66);

  /* Empty file.*/
  istat = call_sut_main("expect_failure", "-x?", "/dev/null", NULL);
  assert(istat == 65);

  /* Cannot open fallthru output status file.*/
  istat = call_sut_main("expect_failure", "-o?", bad_filename, NULL);
  assert(istat == 73);



  fildesh_log_trace("Testing -propagate flag.");

  /* Propagating status without a given status.*/
  istat = call_sut_main("expect_failure", "-propagate", NULL);
  assert(istat == 64);

  /* Propagating status with ambiguous statuses.*/
  istat = call_sut_main(
      "expect_failure", "-status", "69", "-x?", literal_127_filename,
      "-propagate", NULL);
  assert(istat == 64);

  /* Propagating status given by arg.*/
  istat = call_sut_main("expect_failure", "-status", "69", "-propagate", NULL);
  assert(istat == 69);

  istat = call_sut_main("expect_failure", "-status", "0", "-propagate", NULL);
  assert(istat == 0);

  /* Propagating status given by file.*/
  istat = call_sut_main(
      "expect_failure", "-x?", literal_127_filename, "-propagate", NULL);
  assert(istat == 127);

  istat = call_sut_main(
      "expect_failure", "-x?", literal_0_filename, "-propagate", NULL);
  assert(istat == 0);



  fildesh_log_trace("Testing fallthru output flag (-o?).");

  /* Fallthru output returns success if used properly.*/
  istat = call_sut_main(
      "expect_failure", "-error", "Generic error.",
      "-x?", literal_127_filename, "-o?", "/dev/null", NULL);
  assert(istat == 0);

  istat = call_sut_main(
      "expect_failure", "-x?", literal_0_filename, "-o?", "/dev/null", NULL);
  assert(istat == 0);

  istat = call_sut_main(
      "expect_failure", "-status", "127",
      "-x?", literal_0_filename, "-o?", "/dev/null", NULL);
  assert(istat == 0);

  istat = call_sut_main(
      "expect_failure", "-status", "127",  "-error", "Exit code too large!",
      "-x?", literal_127_filename, "-o?", "/dev/null", NULL);
  assert(istat == 0);

  /* Fallthru output is incompatible with status propagation.*/
  istat = call_sut_main(
      "expect_failure", "-x?", literal_127_filename, "-propagate",
      "-o?", "/dev/null", NULL);
  assert(istat == 64);

  /* Fallthru output is required for custom error message.*/
  istat = call_sut_main(
      "expect_failure", "-x?", literal_127_filename,
      "-error", "Exit code too large!", NULL);
  assert(istat == 64);



  fildesh_log_trace("Testing expectations.");

  /* Expecting failure and getting it.*/
  istat = call_sut_main("expect_failure", "-x?", literal_127_filename, NULL);
  assert(istat == 0);

  istat = call_sut_main(
      "expect_failure", "-status", "127", "-x?", literal_127_filename, NULL);
  assert(istat == 0);

  /* Expecting success and getting it.*/
  istat = call_sut_main(
      "expect_failure", "-status", "0", "-x?", literal_0_filename, NULL);
  assert(istat == 0);

  /* Expecting failure but succeeding.*/
  istat = call_sut_main("expect_failure", "-x?", literal_0_filename, NULL);
  assert(istat == 1);

  /* Expecting success but failing.*/
  istat = call_sut_main(
      "expect_failure", "-status", "0", "-x?", literal_127_filename, NULL);
  assert(istat == 1);

  /* Expecting failure but getting a different status.*/
  istat = call_sut_main(
      "expect_failure", "-status", "126", "-x?", literal_127_filename, NULL);
  assert(istat == 1);


  free(bad_filename);
  return 0;
}
