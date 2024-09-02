add_subdirectory(kv)
add_subdirectory(xo)

add_executable(lib_alloc_test
  "alloc_test.c")
target_link_libraries(lib_alloc_test fildesh_lib)
add_test(NAME lib_alloc_test COMMAND lib_alloc_test)

add_executable(lib_array_test
  "array_test.c")
target_link_libraries(lib_array_test fildesh_lib)
add_test(NAME lib_array_test COMMAND lib_array_test)

add_executable(lib_mascii_test
  "mascii_test.c")
target_link_libraries(lib_mascii_test fildesh_lib)
add_test(NAME lib_mascii_test COMMAND lib_mascii_test)

add_executable(lib_size_test
  "size_test.c")
target_link_libraries(lib_size_test fildesh_lib)
add_test(NAME lib_size_test COMMAND lib_size_test)

add_executable(lib_string_test
  "string_test.c")
target_link_libraries(lib_string_test fildesh_lib)
add_test(NAME lib_string_test COMMAND lib_string_test)
