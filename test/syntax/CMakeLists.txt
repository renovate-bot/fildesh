
add_test(NAME syntax_alias_ox_test COMMAND ${Fildesh_EXECUTABLE}
  -alias "expectish=${FildeshTool_expectish_EXECUTABLE}"
  -alias "shout=${FildeshTool_shout_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/alias_ox_test.fildesh"
)

add_test(NAME syntax_coalesce_undefined_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/coalesce_undefined_test.fildesh")

add_test(NAME syntax_command_test COMMAND ${Fildesh_EXECUTABLE}
  -alias "void=${FildeshTool_shout_EXECUTABLE}"
  -f "${CMAKE_CURRENT_SOURCE_DIR}/command_test.fildesh"
)

add_test(NAME syntax_comment_test COMMAND ${Fildesh_EXECUTABLE}
  -f "${CMAKE_CURRENT_SOURCE_DIR}/comment_test.fildesh"
)

add_test(NAME syntax_concatenate_axfxax_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/concatenate_axfxax_test.fildesh")
add_test(NAME syntax_concatenate_hxfxax_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/concatenate_hxfxax_test.fildesh")

add_test(NAME syntax_concatenate_oooxfxax_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/concatenate_oooxfxax_test.fildesh")
add_test(NAME syntax_concatenate_forkonly_oooxfxax_test COMMAND
  fildesh -forkonly -f
  "${CMAKE_CURRENT_SOURCE_DIR}/concatenate_oooxfxax_test.fildesh")

add_test(NAME syntax_concatenate_inline_hhh_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/concatenate_inline_hhh_test.fildesh")
add_test(NAME syntax_concatenate_inline_named_arg_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/concatenate_inline_named_arg_test.fildesh")

add_test(NAME syntax_dev_null_o_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/dev_null_o_test.fildesh")
add_test(NAME syntax_dev_stderr_ox_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/dev_stderr_ox_test.fildesh")

add_test(NAME syntax_environment_variable_xaxa_test COMMAND
  ${Fildesh_EXECUTABLE}
  -setenv "VARIABLE_A=hello"
  -setenv "VARIABLE_B=world"
  -as command
  --
  "${Fildesh_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/environment_variable_xaxa_test.fildesh"
)

add_test(NAME syntax_escape_string_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/escape_string_test.fildesh")

add_test(NAME syntax_executable_bytes_oxf_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/executable_bytes_oxf_test.fildesh"
  "--input_exe=${FildeshTool_shout_EXECUTABLE}"
)
add_test(NAME syntax_executable_bytes_ofxf_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/executable_bytes_ofxf_test.fildesh"
  "--input_exe=${FildeshTool_shout_EXECUTABLE}"
)

add_test(NAME syntax_expect_failure_oxf_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/expect_failure_oxf_test.fildesh")
add_test(NAME syntax_expect_failure_message_oxoxoxox_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/expect_failure_message_oxoxoxox_test.fildesh")

add_test(NAME syntax_filepath_xofxof_test COMMAND ${Fildesh_EXECUTABLE}
  -setenv "TEST_TMPDIR=${CMAKE_CURRENT_BINARY_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/filepath_xofxof_test.fildesh"
)

add_test(NAME syntax_multiline_string_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/multiline_string_test.fildesh")

add_test(NAME syntax_pump_ofxf_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/pump_ofxf_test.fildesh")
add_test(NAME syntax_pump_oxf_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/pump_oxf_test.fildesh")
add_test(NAME syntax_pump_ofx_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/pump_ofx_test.fildesh")
add_test(NAME syntax_pump_ox_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/pump_ox_test.fildesh")

add_test(NAME syntax_redefine_hhxhhx_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/redefine_hhxhhx_test.fildesh")
add_test(NAME syntax_redefine_hxahxfhxh_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/redefine_hxahxfhxh_test.fildesh")

add_test(NAME syntax_stderr_ooox_test COMMAND fildesh -f
  "${CMAKE_CURRENT_SOURCE_DIR}/stderr_ooox_test.fildesh")
add_test(NAME syntax_stdout_ooox_test COMMAND ${Fildesh_EXECUTABLE}
  -f "${CMAKE_CURRENT_SOURCE_DIR}/stdout_ooox_test.fildesh"
)

add_test(NAME syntax_tmpfile_xofxof_test COMMAND ${Fildesh_EXECUTABLE}
  -alias "expectish=${FildeshTool_expectish_EXECUTABLE}"
  -alias "shout=${FildeshTool_shout_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/tmpfile_xofxof_test.fildesh"
)

add_test(NAME syntax_wait_xaxaxa_test COMMAND ${Fildesh_EXECUTABLE}
  -setenv "TEST_TMPDIR=${CMAKE_CURRENT_BINARY_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/wait_xaxaxa_test.fildesh"
)
add_test(NAME syntax_forkonly_wait_xaxaxa_test COMMAND ${Fildesh_EXECUTABLE}
  -setenv "TEST_TMPDIR=${CMAKE_CURRENT_BINARY_DIR}"
  -forkonly
  "${CMAKE_CURRENT_SOURCE_DIR}/wait_xaxaxa_test.fildesh"
  "--testname=syntax_forkonly_wait_xaxaxa_test"
)
