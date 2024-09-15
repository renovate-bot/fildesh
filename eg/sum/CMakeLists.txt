
add_test(NAME eg_sum_add_test COMMAND ${Fildesh_EXECUTABLE}
  "${CMAKE_CURRENT_SOURCE_DIR}/add_test.fildesh"
)

add_test(NAME eg_sum_cycle_test COMMAND comparispawn
  "${CMAKE_CURRENT_SOURCE_DIR}/cycle_expect.txt"
  "${Fildesh_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/cycle.fildesh"
)

add_test(NAME eg_sum_dag_test COMMAND comparispawn
  "${CMAKE_CURRENT_SOURCE_DIR}/dag_expect.txt"
  "${Fildesh_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/dag.fildesh"
)

add_test(NAME eg_sum_readme_test COMMAND ${Fildesh_EXECUTABLE}
  "${CMAKE_CURRENT_SOURCE_DIR}/readme_test.fildesh"
  --example_file "${CMAKE_CURRENT_SOURCE_DIR}/add_test.fildesh"
  --readme_file "${PROJECT_SOURCE_DIR}/README.md"
)
