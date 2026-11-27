# CopyLispNoElc.cmake
#
# Copy lisp/ into the build tree, EXCLUDING *.elc.
#
# Why exclude: a .elc left in the source tree by an in-source build may
# predate the current compiler (e.g. a fix in bytecomp.el) yet carries a
# .elc-version header that update-elc accepts.  copy_directory re-stamps
# it with the current time, so update-elc sees a .elc newer than its .el
# and skips recompiling it -- silently shipping stale bytecode from a
# "clean" build (the #166 recurrence).  The build tree must compile its
# own .elc from .el, never inherit one.
#
# Invoked at BUILD time from src/CMakeLists.txt with:
#   cmake -DSRC_LISP_DIR=<src>/lisp -DBUILD_DIR=<bin> -P copy-lisp-no-elc.cmake
file(COPY "${SRC_LISP_DIR}" DESTINATION "${BUILD_DIR}"
     PATTERN "*.elc" EXCLUDE)