/**
 * Read then write.
 **/

#include <fildesh/fildesh.h>
#include <assert.h>
#include <string.h>

  int
fildesh_builtin_sponge_main(unsigned argc, char** argv,
                         FildeshX** inputv, FildeshO** outputv)
{
  FildeshX* in = NULL;
  FildeshO* out = NULL;
  int exstatus = 0;
  unsigned argi = 1;

  for (argi = 1; argi < argc && exstatus == 0; ++argi) {
    const char* arg = argv[argi];
    if (0 == strcmp(arg, "--")) {
      argi += 1;
      break;
    }
    else if (0 == strcmp(argv[argi], "-x")) {
      in = open_arg_FildeshXF(++argi, argv, inputv);
      if (!in) {
        fildesh_log_errorf("Cannot open file for reading: %s", argv[argi]);
        exstatus = 66;
      }
    }
    else {
      break;
    }
  }

  if (exstatus == 0 && (argc <= argi || argi + 1 < argc)) {
    fildesh_log_errorf("At most one argument expected.");
    exstatus = 64;
  }

  if (exstatus == 0 && !in) {
    in = open_arg_FildeshXF(0, argv, inputv);
    if (!in) {
      fildesh_log_errorf("Cannot open stdin");
      exstatus = 66;
    }
  }

  if (exstatus == 0) {
    slurp_FildeshX(in);

    if (argi < argc) {
      out = open_arg_FildeshOF(argi, argv, outputv);
      if (!out) {
        fildesh_log_errorf("Cannot open output file: %s", argv[argi]);
        exstatus = 73;
      }
    }
    else {
      out = open_arg_FildeshOF(0, argv, outputv);
      if (!out) {
        fildesh_log_errorf("Cannot open stdout");
        exstatus = 70;
      }
    }
  }
  if (exstatus == 0) {
    putslice_FildeshO(out, *in);
  }

  close_FildeshX(in);
  close_FildeshO(out);
  return exstatus;
}

#ifndef FILDESH_BUILTIN_LIBRARY
int main(int argc, char** argv) {
  return fildesh_builtin_sponge_main((unsigned)argc, argv, NULL, NULL);
}
#endif
