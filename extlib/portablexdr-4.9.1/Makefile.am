ACLOCAL_AMFLAGS = -I m4

nobase_include_HEADERS = rpc/rpc.h rpc/types.h rpc/xdr.h

lib_LTLIBRARIES = libportablexdr.la
libportablexdr_la_SOURCES = xdr_array.c xdr_float.c xdr_mem.c xdr_reference.c \
	xdr.c xdr_intXX_t.c xdr_rec.c xdr_stdio.c \
	byteswap.h \
	rpc/rpc.h rpc/types.h rpc/xdr.h
libportablexdr_la_CFLAGS = -Wall -Werror -fno-strict-aliasing
libportablexdr_la_LDFLAGS = @MINGW_EXTRA_LDFLAGS@

# Replacement 'rpcgen'.  Don't call it 'rpcgen' however so that
# we can avoid conflicting with the system rpcgen.
BUILT_SOURCES = rpcgen_parse.h
AM_YFLAGS = -d
bin_PROGRAMS = portable-rpcgen
portable_rpcgen_SOURCES = rpcgen_parse.y rpcgen_scan.l \
	rpcgen_int.h rpcgen_main.c rpcgen_ast.c rpcgen_codegen.c
portable_rpcgen_CFLAGS = -Wall
#portable_rpcgen_CFLAGS += -DYYDEBUG
