add_executable(command_alias_test
  "alias_test.c"
  "${PROJECT_SOURCE_DIR}/src/command/alias.c"
  "${PROJECT_SOURCE_DIR}/src/command/alias.h"
)
target_link_libraries(command_alias_test ${Fildesh_LIBRARIES})
add_test(NAME command_alias_test COMMAND command_alias_test)
