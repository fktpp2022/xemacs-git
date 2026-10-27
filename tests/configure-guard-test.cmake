# Driver for the configure-widget-dialog-guard CTest:
#   cmake -DSRC=<source dir> -DSCRATCH=<scratch dir> -P configure-guard-test.cmake
#
# Each configure runs in its own fresh directory so no cache leaks between
# runs.  The runs are started together because three full configures of this
# tree exceed the test timeout when run back to back on slow machines.  The
# scratch tree is removed before starting and after all runs, including when
# a check fails.

if(NOT DEFINED SRC OR NOT DEFINED SCRATCH)
  message(FATAL_ERROR "Usage: cmake -DSRC=<source dir> -DSCRATCH=<scratch dir> -P configure-guard-test.cmake")
endif()

file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")

set(configure_script "")
foreach(run_name default widgets dialogs)
  set(run_extra "")
  if(run_name STREQUAL "widgets")
    set(run_extra "-DXEMACS_WITH_WIDGETS=ON")
  elseif(run_name STREQUAL "dialogs")
    set(run_extra "-DXEMACS_WITH_DIALOGS=ON")
  endif()
  string(APPEND configure_script
    "( \"${CMAKE_COMMAND}\" -S \"${SRC}\" -B \"${SCRATCH}/${run_name}\" ${run_extra} > \"${SCRATCH}/${run_name}.out\" 2> \"${SCRATCH}/${run_name}.err\"; echo $? > \"${SCRATCH}/${run_name}.status\" ) &\n")
endforeach()
string(APPEND configure_script "wait\n")

execute_process(COMMAND sh -c "${configure_script}" RESULT_VARIABLE shell_result)
if(NOT shell_result EQUAL 0)
  file(REMOVE_RECURSE "${SCRATCH}")
  message(FATAL_ERROR "Failed to start configure runs (exit ${shell_result})")
endif()

function(read_run name out_var err_var result_var)
  file(READ "${SCRATCH}/${name}.out" out)
  file(READ "${SCRATCH}/${name}.err" err)
  file(READ "${SCRATCH}/${name}.status" result)
  string(STRIP "${result}" result)
  set(${out_var} "${out}" PARENT_SCOPE)
  set(${err_var} "${err}" PARENT_SCOPE)
  set(${result_var} "${result}" PARENT_SCOPE)
endfunction()

read_run(default default_out default_err default_result)
read_run(widgets widgets_out widgets_err widgets_result)
read_run(dialogs dialogs_out dialogs_err dialogs_result)

set(failures "")

set(default_output "${default_out}\n${default_err}")
if(NOT default_result EQUAL 0)
  string(APPEND failures "Default configure failed (exit ${default_result}):\n${default_output}\n")
endif()
if(NOT default_output MATCHES "Widgets: disabled \\(unsupported in CMake build\\)")
  string(APPEND failures "Default configure summary does not report widgets as unsupported:\n${default_output}\n")
endif()
if(NOT default_output MATCHES "Dialogs: disabled \\(unsupported in CMake build\\)")
  string(APPEND failures "Default configure summary does not report dialogs as unsupported:\n${default_output}\n")
endif()

set(widgets_output "${widgets_out}\n${widgets_err}")
if(widgets_result EQUAL 0)
  string(APPEND failures "Configure accepted XEMACS_WITH_WIDGETS=ON:\n${widgets_output}\n")
endif()
if(NOT widgets_output MATCHES "XEMACS_WITH_WIDGETS")
  string(APPEND failures "Widget guard message does not name XEMACS_WITH_WIDGETS:\n${widgets_output}\n")
endif()

set(dialogs_output "${dialogs_out}\n${dialogs_err}")
if(dialogs_result EQUAL 0)
  string(APPEND failures "Configure accepted XEMACS_WITH_DIALOGS=ON:\n${dialogs_output}\n")
endif()
if(NOT dialogs_output MATCHES "XEMACS_WITH_DIALOGS")
  string(APPEND failures "Dialog guard message does not name XEMACS_WITH_DIALOGS:\n${dialogs_output}\n")
endif()

file(REMOVE_RECURSE "${SCRATCH}")

if(failures)
  message(FATAL_ERROR "${failures}")
endif()
