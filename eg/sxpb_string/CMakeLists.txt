add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/grammar.embed.h"
  COMMAND ${FildeshTool_cembed_EXECUTABLE}
  -o "${CMAKE_CURRENT_BINARY_DIR}/grammar.embed.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/grammar.sxpb"
  DEPENDS "grammar.sxpb"
)

add_executable(eg_sxpb_string_grammar_test
  "grammar_test.c"
  "${CMAKE_CURRENT_BINARY_DIR}/grammar.embed.h"
)
set_property(TARGET eg_sxpb_string_grammar_test APPEND PROPERTY INCLUDE_DIRECTORIES
  "${CMAKE_BINARY_DIR}")
target_link_libraries(eg_sxpb_string_grammar_test ${FildeshSxproto_LIBRARIES})
add_test(NAME eg_sxpb_string_grammar_test COMMAND eg_sxpb_string_grammar_test)
