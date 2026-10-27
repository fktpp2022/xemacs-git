# Driver for the configure-widget-dialog-guard CTest:
#   cmake -DSRC=<source dir> -DSCRATCH=<scratch dir> -P configure-guard-test.cmake
#
# The default configure runs to completion in the scratch directory and its
# summary is checked.  The guard runs reuse that configured directory and only
# change one option, so they fail quickly without repeating detection.

if(NOT DEFINED SRC OR NOT DEFINED SCRATCH)
  message(FATAL_ERROR "Usage: cmake -DSRC=<source dir> -DSCRATCH=<scratch dir> -P configure-guard-test.cmake")
endif()

file(REMOVE_RECURSE "${SCRATCH}")

set(failures "")

function(run_configure name)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -S "${SRC}" -B "${SCRATCH}" ${ARGN}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)
  set("${name}_result" "${result}" PARENT_SCOPE)
  set("${name}_output" "${output}\n${error}" PARENT_SCOPE)
endfunction()

run_configure(default)
if(NOT default_result EQUAL 0)
  string(APPEND failures "Default configure failed (exit ${default_result}):\n${default_output}\n")
endif()
if(NOT default_output MATCHES "Widgets: disabled \\(unsupported in CMake build\\)")
  string(APPEND failures "Default configure summary does not report widgets as unsupported:\n${default_output}\n")
endif()
if(NOT default_output MATCHES "Dialogs: disabled \\(unsupported in CMake build\\)")
  string(APPEND failures "Default configure summary does not report dialogs as unsupported:\n${default_output}\n")
endif()

run_configure(widgets -DXEMACS_WITH_WIDGETS=ON)
if(widgets_result EQUAL 0)
  string(APPEND failures "Configure accepted XEMACS_WITH_WIDGETS=ON:\n${widgets_output}\n")
endif()
if(NOT widgets_output MATCHES "XEMACS_WITH_WIDGETS")
  string(APPEND failures "Widget guard message does not name XEMACS_WITH_WIDGETS:\n${widgets_output}\n")
endif()
if(NOT widgets_output MATCHES "not supported by the CMake")
  string(APPEND failures "Widget guard message does not explain the CMake limitation:\n${widgets_output}\n")
endif()

run_configure(dialogs -DXEMACS_WITH_DIALOGS=ON -DXEMACS_WITH_WIDGETS=OFF)
if(dialogs_result EQUAL 0)
  string(APPEND failures "Configure accepted XEMACS_WITH_DIALOGS=ON:\n${dialogs_output}\n")
endif()
if(NOT dialogs_output MATCHES "XEMACS_WITH_DIALOGS")
  string(APPEND failures "Dialog guard message does not name XEMACS_WITH_DIALOGS:\n${dialogs_output}\n")
endif()
if(NOT dialogs_output MATCHES "not supported by the CMake")
  string(APPEND failures "Dialog guard message does not explain the CMake limitation:\n${dialogs_output}\n")
endif()

file(REMOVE_RECURSE "${SCRATCH}")

if(failures)
  message(FATAL_ERROR "${failures}")
endif()
