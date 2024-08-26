#include "fuzz_common.h"

#include <assert.h>

#include "src/syntax/defstr.h"
#include "src/syntax/line.h"

/** Test that line parsing doesn't crash.
 *
 * A "line" in this case should capture a meaningful command, so the
 * line parsing function is expected to skip comments and blank lines.
 **/
  int
LLVMFuzzerTestOneInput(const uint8_t data[], size_t size) {
  size_t text_nlines = 0;
  FildeshAlloc* alloc = open_FildeshAlloc();
  DECLARE_DEFAULT_FildeshAT(char*, cmd_args);
  FildeshKV map[1] = {DEFAULT_FildeshKV};
  FildeshO tmp_out[1] = {DEFAULT_FildeshO};
  FildeshX in[1];
  char* line = NULL;

  map->alloc = alloc;
  *in = FildeshX_of_bytestring(data, size);

  while (avail_FildeshX(in)) {
    size_t old_nlines = text_nlines;
    const char* emsg = parse_fildesh_string_definition(
        in, &text_nlines, map, alloc, tmp_out);
    if (!emsg) {continue;}
    if (emsg[0]) {break;}
    line = fildesh_syntax_parse_line(in, &text_nlines, alloc, tmp_out);
    if (!line) {break;}
    assert(text_nlines > old_nlines);
#ifdef SMOKE_TEST_ON
    /* Fuzzing doesn't like getenv(), so this part is smoke-only.*/
    fildesh_syntax_sep_line(
        cmd_args, line, map, alloc, tmp_out);
#endif
  }

  close_FildeshX(in);
  close_FildeshO(tmp_out);
  close_FildeshKV(map);
  close_FildeshAT(cmd_args);
  close_FildeshAlloc(alloc);
  return 0;
}

