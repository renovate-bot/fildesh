#include "test/fuzz/smoke_common.h"

BEGIN_FUZZ_DATA
"Unterminated double quote.\n"
"(: s Str \"value"
NEXT_FUZZ_DATA
/* Success.*/
"(: s Str\r\n \"\\t\\n\\v\\f\\r\\n\")"
NEXT_FUZZ_DATA
"Please wrap variable name in curly braces when it is part of a string.\n"
"splice / \"$s\" /"
NEXT_FUZZ_DATA
"Missing closing curly brace.\n"
"splice / \"${s\" /"
NEXT_FUZZ_DATA
"Variable not known at parse time.\n"
"splice / \"${s}\" /"
NEXT_FUZZ_DATA
"Need space after \"(??\".\n"
"(: s Str (??.self.opt.e  \"value\"))"
NEXT_FUZZ_DATA
"Need closing paren for \"(??\".\n"
"(: s Str (?? .self.opt.e  \"value\""
NEXT_FUZZ_DATA
"Need space after \"(++\".\n"
"(: s Str (++\"a\"  \"b\"))"
NEXT_FUZZ_DATA
"Need closing paren for \"(++\".\n"
"(: s Str (++ \"a\"  \"b\""
NEXT_FUZZ_DATA
"Expected Filepath or Str type.\n"
"(: s UnknownType \"value\")"
NEXT_FUZZ_DATA
"Expected a closing paren.\n"
"(: s Str \"value\""
NEXT_FUZZ_DATA
/* Former memory leak.*/
"(: s Str \na b"
END_FUZZ_DATA
