/** Simple utility to replace all instances of a string.**/

#include <fildesh/fildesh.h>

#include <string.h>

  int
fildesh_builtin_replace_string_main(
    unsigned argc, char** argv, FildeshX** inputv, FildeshO** outputv)
{
  FildeshX* in = NULL;
  FildeshO* out = NULL;
  int exstatus = 0;
  unsigned argi = 1;
  const unsigned char* needle = NULL;
  const unsigned char* replacement = NULL;
  size_t needle_size = 0;
  size_t replacement_size = 0;

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
    else if (0 == strcmp(argv[argi], "-o")) {
      out = open_arg_FildeshOF(++argi, argv, outputv);
      if (!out) {
        fildesh_log_errorf("Cannot open file for writing: %s", argv[argi]);
        exstatus = 73;
      }
    }
    else {
      break;
    }
  }

  if (exstatus == 0 && (argc <= argi || argi + 2 < argc)) {
    fildesh_log_error("Need 1 or 2 arguments.");
    exstatus = 64;
  }
  if (exstatus == 0) {
    needle = (const unsigned char*) argv[argi++];
    needle_size = strlen((char*)needle);
    if (needle_size == 0) {needle_size = 1;}
  }
  if (exstatus == 0 && argi < argc) {
    replacement = (const unsigned char*) argv[argi++];
    replacement_size = strlen((char*)replacement);
    if (replacement_size == 0) {replacement_size = 1;}
  }

  if (exstatus == 0 && !in) {
    in = open_arg_FildeshXF(0, argv, inputv);
    if (!in) {
      fildesh_log_error("Cannot open stdin.");
      exstatus = 66;
    }
  }
  if (exstatus == 0 && !out) {
    out = open_arg_FildeshOF(0, argv, outputv);
    if (!out) {
      fildesh_log_error("Cannot open stdout.");
      exstatus = 70;
    }
  }

  while (exstatus == 0 && 0 < read_FildeshX(in)) {
    FildeshX slice = getslice_FildeshX(in);
    FildeshX span;
    for (span = until_bytestring_FildeshX(&slice, needle, needle_size);
         span.at;
         span = until_bytestring_FildeshX(&slice, needle, needle_size))
    {
      putslice_FildeshO(out, span);
      if (skip_bytestring_FildeshX(&slice, NULL, needle_size)) {
        put_bytestring_FildeshO(out, replacement, replacement_size);
      }
    }
    in->off += slice.off;
    maybe_flush_FildeshX(in);
  }
  if (exstatus == 0) {
    putslice_FildeshO(out, *in);
  }
  close_FildeshX(in);
  close_FildeshO(out);
  return exstatus;
}

#if !defined(FILDESH_BUILTIN_LIBRARY) && !defined(UNIT_TESTING)
  int
main(int argc, char** argv)
{
  return fildesh_builtin_replace_string_main((unsigned)argc, argv, NULL, NULL);
}
#endif
