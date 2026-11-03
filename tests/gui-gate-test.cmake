# Driver for the GUI gates:
#   cmake -DGATE=<gate .el> -DXEMACS=<binary> -DDIR=<working dir> -P gui-gate-test.cmake
#
# The gates open a frame, so they need a window system.  On a host with no
# display XEmacs takes its terminal path and exits on the non-tty stdin ctest
# hands it, before any Lisp runs, so the display check happens here rather than
# in the gate.  With DISPLAY or WAYLAND_DISPLAY set, the gate's own output is
# passed through and its exit status decides the test.

if(NOT DEFINED GATE OR NOT DEFINED XEMACS OR NOT DEFINED DIR)
  message(FATAL_ERROR
    "Usage: cmake -DGATE=<gate .el> -DXEMACS=<binary> -DDIR=<working dir> -P gui-gate-test.cmake")
endif()

if(DEFINED ENV{DISPLAY} AND NOT "$ENV{DISPLAY}" STREQUAL "")
  set(gui_display "DISPLAY=$ENV{DISPLAY}")
elseif(DEFINED ENV{WAYLAND_DISPLAY} AND NOT "$ENV{WAYLAND_DISPLAY}" STREQUAL "")
  set(gui_display "WAYLAND_DISPLAY=$ENV{WAYLAND_DISPLAY}")
else()
  set(gui_display "")
endif()

if(gui_display STREQUAL "")
  message("SKIP: no display reachable; set DISPLAY to run ${GATE}")
  return()
endif()

message("INFO: running ${GATE} under ${gui_display}")

execute_process(
  COMMAND "${XEMACS}" -no-init-file -no-site-file -l "${GATE}"
  WORKING_DIRECTORY "${DIR}"
  RESULT_VARIABLE gate_result)

if(NOT gate_result EQUAL 0)
  message(FATAL_ERROR "${GATE} exited ${gate_result}")
endif()
