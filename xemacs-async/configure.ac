AC_INIT([xemacs-async], [0.1.0], [xemacs-dev@xemacs.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_CHECK_LIB([uv], [uv_loop_new], [], [AC_MSG_ERROR([libuv required])])
AC_CHECK_LIB([msgpackc], [msgpack_pack_init], [], [AC_MSG_ERROR([msgpack-c required])])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT