ACLOCAL_AMFLAGS = -I m4

dist_doc_DATA = README

lib_LTLIBRARIES = openvpn-python.la

openvpn_python_la_SOURCES = \
    src/py_plugin.c \
    src/py_plugin.h \
    src/openvpn-plugin.h

openvpn_python_la_LIBADD = -lpthread -lc

openvpn_python_la_LDFLAGS = \
    -export-symbols "$(srcdir)/openvpn-python.exports" \
    -module \
    -shared \
    -avoid-version \
    -no-undefined