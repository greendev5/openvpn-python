ACLOCAL_AMFLAGS = -I m4

dist_doc_DATA = README

lib_LTLIBRARIES = openvpn-python.la
openvpn_python_la_SOURCES = \
    src/py_plugin.c \
    src/py_plugin.h \
    src/plugin_utils.c \
    src/plugin_utils.h \
    src/py_handlers.c \
    src/py_handlers.h \
    src/deferred_queue.c \
    src/deferred_queue.h \
    src/py_server.c \
    src/py_server.h \
    src/openvpn-plugin.h
openvpn_python_la_LIBADD = @PYTHON_LDFLAGS@ -lc
openvpn_python_la_LDFLAGS = \
    -export-symbols "$(srcdir)/openvpn-python.exports" \
    -module \
    -shared \
    -avoid-version \
    -no-undefined
openvpn_python_la_CFLAGS = $(AM_CFLAGS) @PYTHON_CFLAGS@


noinst_PROGRAMS = py_server_test
py_server_test_SOURCES = \
    tests/py_server_test.c \
    src/plugin_utils.c \
    src/py_server.c \
    src/py_handlers.c \
    src/deferred_queue.c
py_server_test_LDADD = @PYTHON_LDFLAGS@
py_server_test_CFLAGS = $(AM_CFLAGS) @PYTHON_CFLAGS@