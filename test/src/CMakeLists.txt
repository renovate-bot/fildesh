if (FILDESH_BUILTIN_ELASTIC_AIO_ON)
  add_executable(src_chatty_test
    "chatty_test.c"
  )
  target_link_libraries(src_chatty_test fildesh_lib ${FILDESH_AIO_LIBS})
  set_property(TARGET src_chatty_test APPEND PROPERTY COMPILE_DEFINITIONS
    "_POSIX_C_SOURCE=201112L"
  )
  add_test(NAME src_chatty_test COMMAND src_chatty_test)
endif()
