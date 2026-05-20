lib_LIBRARIES = libxemacsasync.a
libxemacsasync_a_SOURCES = src/event_loop.c src/rpc_server.c src/rpc_client.c src/file_watch.c src/service_disc.c
include_HEADERS = include/xemacs-async.h
AM_CFLAGS = -I$(srcdir)/include

check_PROGRAMS = test_event_loop test_service_discovery
test_event_loop_SOURCES = tests/async/test_event_loop.c
test_event_loop_LDADD = libxemacsasync.a -luv -lmsgpackc
test_service_discovery_SOURCES = tests/async/test_service_discovery.c
test_service_discovery_LDADD = libxemacsasync.a -luv -lmsgpackc
TESTS = $(check_PROGRAMS)


