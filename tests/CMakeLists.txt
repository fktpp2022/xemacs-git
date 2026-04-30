# CTest definitions for XEmacs automated test suite
#
# Usage:
#   ctest                        # run all tests
#   ctest -R lisp-tests          # run a specific test
#   ctest --output-on-failure    # show output on failure

set(XEMACS_TEST_COMMAND
  "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROGNAME}"
  -batch -l test-harness -f batch-test-emacs
)

file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/automated/*.el")
list(SORT TEST_FILES)

foreach(test_file ${TEST_FILES})
  get_filename_component(test_name "${test_file}" NAME_WE)
  add_test(
    NAME "${test_name}"
    COMMAND ${XEMACS_TEST_COMMAND} "${test_file}"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
  set_tests_properties("${test_name}" PROPERTIES
    TIMEOUT 120
    FIXTURES_REQUIRED xemacs_build
  )
endforeach()

# Fixture: ensures the build has completed before tests run
add_test(NAME xemacs_build_fixture COMMAND ${CMAKE_COMMAND} -E true)
set_tests_properties(xemacs_build_fixture PROPERTIES FIXTURES_SETUP xemacs_build)

# Convenience target: `make check` runs ctest
add_custom_target(check
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  DEPENDS build-all
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  COMMENT "Running automated test suite"
)
