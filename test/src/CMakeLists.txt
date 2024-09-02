add_executable(lib_xo_delimit_edge_test
  "delimit_edge_test.c")
target_link_libraries(lib_xo_delimit_edge_test fildesh_lib)
add_test(NAME lib_xo_delimit_edge_test COMMAND lib_xo_delimit_edge_test)

add_executable(lib_xo_getline_test
  "getline_test.c")
target_link_libraries(lib_xo_getline_test fildesh_lib)
add_test(NAME lib_xo_getline_test COMMAND lib_xo_getline_test)

add_executable(lib_xo_gets_test
  "gets_test.c")
target_link_libraries(lib_xo_gets_test fildesh_lib)
add_test(NAME lib_xo_gets_test COMMAND lib_xo_gets_test)

add_executable(lib_xo_parse_int_test
  "parse_int_test.c")
target_link_libraries(lib_xo_parse_int_test fildesh_lib)
add_test(NAME lib_xo_parse_int_test COMMAND lib_xo_parse_int_test)

add_executable(lib_xo_puts_test
  "puts_test.c")
target_link_libraries(lib_xo_puts_test fildesh_lib)
add_test(NAME lib_xo_puts_test COMMAND lib_xo_puts_test)

add_executable(lib_xo_read_file_lines_test
  "read_file_lines_test.c")
target_link_libraries(lib_xo_read_file_lines_test fildesh_lib)
add_test(NAME lib_xo_read_file_lines_test
  COMMAND lib_xo_read_file_lines_test
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_executable(lib_xo_sibling_file_test
  "sibling_file_test.c")
target_link_libraries(lib_xo_sibling_file_test fildesh_lib)
add_test(NAME lib_xo_sibling_file_test COMMAND
  "${CMAKE_COMMAND}" -E env
  "TEST_TMPDIR=${CMAKE_CURRENT_BINARY_DIR}"
  --
  "$<TARGET_FILE:lib_xo_sibling_file_test>"
)

add_executable(lib_xo_skipstr_test
  "skipstr_test.c")
target_link_libraries(lib_xo_skipstr_test fildesh_lib)
add_test(NAME lib_xo_skipstr_test COMMAND lib_xo_skipstr_test)

add_executable(lib_xo_slicechrs_test
  "slicechrs_test.c")
target_link_libraries(lib_xo_slicechrs_test fildesh_lib)
add_test(NAME lib_xo_slicechrs_test COMMAND lib_xo_slicechrs_test)

add_executable(lib_xo_urandom_variety_test
  "urandom_variety_test.c")
target_link_libraries(lib_xo_urandom_variety_test fildesh_lib)
add_test(NAME lib_xo_urandom_variety_test COMMAND lib_xo_urandom_variety_test)
