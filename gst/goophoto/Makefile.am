plugin_LTLIBRARIES = libgstgoophoto.la

libgstgoophoto_la_SOURCES = \
	gstgoophotobin.c

libgstgoophoto_la_CFLAGS = \
	$(GST_CFLAGS) \
	$(GST_BASE_CFLAGS) \
	$(LIBGOO_TI_CFLAGS)

libgstgoophoto_la_LIBADD = \
	-lgstinterfaces-@GST_MAJORMINOR@ \
	$(GST_LIBS) \
	$(GST_BASE_LIBS) \
	$(LIBGOO_TI_LIBS)

libgstgoophoto_la_LDFLAGS = \
	$(GST_PLUGIN_LDFLAGS)

#noinst_HEADERS =
