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

# The live ELPA test is network-labeled and SKIPPED unless enabled: it
# exits with SKIP_RETURN_CODE when ELPA_LIVE is unset (the elisp early-
# exit path calls (kill-emacs 125)), so a default ctest reports it as
# skipped rather than passed.  When enabled it does several live HTTPS
# fetches per harness pass, so give it a generous network-sized timeout
# instead of the default 120 s.
set_tests_properties(elpa-live-tests PROPERTIES
  LABELS "network"
  TIMEOUT 300
  SKIP_RETURN_CODE 125)
add_test(NAME xemacs_build_fixture COMMAND ${CMAKE_COMMAND} -E true)
set_tests_properties(xemacs_build_fixture PROPERTIES FIXTURES_SETUP xemacs_build)

# Convenience target: `make check` runs ctest
add_custom_target(check
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  DEPENDS build-all
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  COMMENT "Running automated test suite"
)
