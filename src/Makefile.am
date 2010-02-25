plugin_LTLIBRARIES = libgstgoo.la

libgstgoo_la_SOURCES = \
	gstgoodecwbamr.c \
	gstgoodecnbamr.c \
	gstgoofilter.c \
	gstgooaudiofilter.c \
	gstgootransform.c \
	gstgoobuffer.c \
	gstghostbuffer.c \
	gstgoodecmp3.c \
	gstgoodecwma.c \
	gstgoodecpcm.c \
	gstgoodecimaadpcm.c \
	gstgoodecgsmhr.c \
	gstgoodecg722.c \
	gstgoodecg711.c \
	gstgoodecaac.c \
	gstgoodecarmaac.c \
	gstgooencpcm.c \
	gstgooencnbamr.c \
	gstgooencgsmhr.c \
	gstgooencwbamr.c \
	gstgooencgsmfr.c \
	gstgooencaac.c \
	gstgooencarmaac.c \
	gstdasfsrc.c \
	gstdasfsink.c \
	gstgoo.c \
	gstgoodecmpeg4.c \
	gstgoodecmpeg4720p.c \
	gstgoodecspark.c \
	gstgoosem.c \
	gstgooadapter.c \
	gstgooencjpeg.c \
	gstgoodecjpeg.c \
	gstgoodech264.c \
	gstgoodech264720p.c \
	gstgoodech263.c \
	gstgoodecwmv.c \
	gstgooencmpeg4.c \
	gstgooench264.c \
	gstgooench263.c \
	gstgoosinkpp.c \
    gstgoofiltervpp.c \
	gstgoodecmpeg2.c \
	gstgoovideofilter.c \
    gstgoovideodec.c \
    gstgoovideodec720p.c \    
	gstgoovideoenc.c \
	gstgooutils.c \
	gstgoodecgsmfr.c

libgstgoo_la_CFLAGS = \
	$(GST_CFLAGS) \
	$(GST_BASE_CFLAGS) \
	$(LIBGOO_TI_CFLAGS)

libgstgoo_la_LIBADD = \
	-lgstinterfaces-@GST_MAJORMINOR@ \
	-lgstvideo-@GST_MAJORMINOR@ \
	-lgstaudio-@GST_MAJORMINOR@ \
	$(GST_LIBS) \
	$(GST_BASE_LIBS) \
	$(LIBGOO_TI_LIBS)

libgstgoo_la_LDFLAGS = \
	$(GST_PLUGIN_LDFLAGS)

noinst_HEADERS = \
	gstgoodecwbamr.h \
	gstgoodecnbamr.h \
	gstgoofilter.h \
	gstgooaudiofilter.h \
	gstgootransform.h \
	gstgoobuffer.h \
	gstghostbuffer.h \
	gstgoodecmp3.h \
	gstgoodecwma.h \
	gstgoodecpcm.h \
	gstgoodecimaadpcm.h \
	gstgoodecgsmhr.h \
	gstgoodecg722.h \
	gstgoodecg711.h \
	gstgoodecaac.h \
	gstgoodecarmaac.h \
	gstgooencpcm.h \
	gstgooencnbamr.h \
	gstgooencgsmhr.h \
	gstgooencwbamr.h \
	gstgooencgsmfr.h \
	gstgooencaac.h \
	gstgooencarmaac.h \
	gstdasfsrc.h \
	gstdasfsink.h \
	gstgoodecmpeg4.h \
	gstgoodecmpeg4720p.h \
	gstgoodecspark.h \
	gstgoosem.h \
	gstgooadapter.h \
	gstgooencjpeg.h \
	gstgoodecjpeg.h \
	gstgoodech264.h \
	gstgoodech264720p.h \
	gstgoodech263.h \
	gstgoodecwmv.h \
	gstgooencmpeg4.h \
	gstgooench264.h \
	gstgooench263.h \
	gstgoosinkpp.h \
	gstgoofiltervpp.h \
	gstgoodecmpeg2.h \
	gstgoovideofilter.h \
    gstgoovideodec.h \
    gstgoovideodec720p.h \    
	gstgoovideoenc.h \
	gstgooutils.h \
	gstgoodecgsmfr.h

if WITH_TI_CAMERA
libgstgoo_la_SOURCES += gstgoocamera.c
noinst_HEADERS += gstgoocamera.h
endif
