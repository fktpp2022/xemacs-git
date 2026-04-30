# Build info pages from texinfo sources
#
# Finds makeinfo (or texi2any) and creates a target to build all info
# files from the man/ directory into the build tree's info/ directory.

find_program(MAKEINFO NAMES makeinfo texi2any)

if(NOT MAKEINFO)
  message(STATUS "makeinfo not found - info pages will not be built")
  return()
endif()

message(STATUS "Found makeinfo: ${MAKEINFO}")

set(INFO_OUTPUT_DIR "${CMAKE_BINARY_DIR}/info")
file(MAKE_DIRECTORY "${INFO_OUTPUT_DIR}")

set(MAN_DIR "${CMAKE_SOURCE_DIR}/man")

# Simple manuals: single .texi -> .info
set(SIMPLE_MANUALS beta cl emodules external-widget info standards termcap widget xemacs-faq)

# Subdirectory manuals: main .texi lives in a subdir, needs -I flag
set(SUBDIR_MANUALS xemacs lispref internals new-users-guide)

set(INFO_OUTPUTS "")

foreach(_manual ${SIMPLE_MANUALS})
  set(_src "${MAN_DIR}/${_manual}.texi")
  set(_out "${INFO_OUTPUT_DIR}/${_manual}.info")
  if(EXISTS "${_src}")
    add_custom_command(
      OUTPUT "${_out}"
      DEPENDS "${_src}"
      COMMAND "${MAKEINFO}" --no-split -o "${_out}" "${_src}"
      WORKING_DIRECTORY "${MAN_DIR}"
      COMMENT "Generating ${_manual}.info"
      VERBATIM
    )
    list(APPEND INFO_OUTPUTS "${_out}")
  endif()
endforeach()

foreach(_manual ${SUBDIR_MANUALS})
  set(_src "${MAN_DIR}/${_manual}/${_manual}.texi")
  set(_out "${INFO_OUTPUT_DIR}/${_manual}.info")
  if(EXISTS "${_src}")
    add_custom_command(
      OUTPUT "${_out}"
      DEPENDS "${_src}"
      COMMAND "${MAKEINFO}" -I "${MAN_DIR}/${_manual}" -o "${_out}" "${_src}"
      WORKING_DIRECTORY "${MAN_DIR}"
      COMMENT "Generating ${_manual}.info"
      VERBATIM
    )
    list(APPEND INFO_OUTPUTS "${_out}")
  endif()
endforeach()

add_custom_target(info_pages ALL DEPENDS ${INFO_OUTPUTS})

# Install built info pages
if(XEMACS_INSTALL_INFO)
  install(DIRECTORY "${INFO_OUTPUT_DIR}/"
    DESTINATION ${INST_INFODIR}
    COMPONENT Info
    FILES_MATCHING
      PATTERN "*.info"
      PATTERN "*.info-[0-9]"
      PATTERN "*.info-[0-9][0-9]"
  )
endif()
