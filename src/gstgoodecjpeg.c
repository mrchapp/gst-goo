/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 * Copyright (C) 2008 Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.1 of the License.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/video/video.h>
#include <goo-ti-jpegdec.h>
#include <string.h>

#include "gstgoodecjpeg.h"
#include "gstgoobuffer.h"
#include "gstghostbuffer.h"

GST_DEBUG_CATEGORY_STATIC (gst_goo_decjpeg_debug);
#define GST_CAT_DEFAULT gst_goo_decjpeg_debug

/* signals */
enum
{
	LAST_SIGNAL
};

/* args */
enum
{
	PROP_0,
	PROP_RESIZE,
	PROP_SLICING,
	PROP_SUBREGIONLEFT,
	PROP_SUBREGIONTOP,
	PROP_SUBREGIONWIDTH,
	PROP_SUBREGIONHEIGHT
};

#define GST_GOO_DECJPEG_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_GOO_DECJPEG, GstGooDecJpegPrivate))

struct _GstGooDecJpegPrivate
{
	guint incount;
	guint outcount;

	gint scalefactor;
	gint width;
	gint height;
	gint in_color;
	gint out_color;
	gint imagesiz;
	guint framerate_numerator;
	guint framerate_denominator;

	gint caps_width;
	gint caps_height;
	gint caps_framerate_numerator;
	gint caps_framerate_denominator;

	GstBuffer* tempbuf;
	GstSegment* segment;
	GstClockTime duration;

	guint64 next_ts;
	gboolean packetized;

	gboolean progressive;

	/* Section decoding */
	guint num_slices;
	GstBuffer* buffer_slice;
	/* SubRegion decoding */
	guint subregion_left;
	guint subregion_top;
	guint subregion_width;
	guint subregion_height;

	guint size;
	gfloat factor_color;
	gint count;
};

#define SCALE_DEFAULT GOO_TI_JPEGDEC_SCALE_NONE
#define SLICING_DEFAULT 0
#define MIN_WIDTH  16
#define MAX_WIDTH  2147483647
#define MIN_HEIGHT 16
#define MAX_HEIGHT 2147483647
#define MAX_SECTIONS 20

#define M_SOF0	0xC0		/* nStart Of Frame N */
#define M_SOF1	0xC1		/* N indicates which compression process */
#define M_SOF2	0xC2		/* Only SOF0-SOF2 are now in common use */
#define M_SOF3	0xC3
#define M_SOF5	0xC5		/* NB: codes C4 and CC are NOT SOF markers */
#define M_SOF6	0xC6
#define M_SOF7	0xC7
#define M_SOF9	0xC9
#define M_SOF10 0xCA
#define M_SOF11 0xCB
#define M_SOF13 0xCD
#define M_SOF14 0xCE
#define M_SOF15 0xCF
#define M_SOI	0xD8		/* nStart Of Image (beginning of datastream) */
#define M_EOI	0xD9		/* End Of Image (end of datastream) */
#define M_SOS	0xDA		/* nStart Of Scan (begins compressed data) */
#define M_JFIF	0xE0		/* Jfif marker */
#define M_EXIF	0xE1		/* Exif marker */
#define M_COM	0xFE		/* Comment  */
#define M_DQT	0xDB
#define M_DHT	0xC4
#define M_DRI	0xDD


static const GstElementDetails details =
	GST_ELEMENT_DETAILS (
		"OpenMax JPEG decoder",
		"Codec/Decoder/Image",
		"Decodes images in JPEG format with OpenMAX",
		"Texas Instrument"
		);

static GstStaticPadTemplate src_factory =
	GST_STATIC_PAD_TEMPLATE (
		"src",
		GST_PAD_SRC,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS (GST_VIDEO_CAPS_YUV ("{ I420, UYVY }") ";"
				 GST_VIDEO_CAPS_RGB_16 ";"
				 GST_VIDEO_CAPS_RGB ";"
				 GST_VIDEO_CAPS_ABGR)
		);

static GstStaticPadTemplate sink_factory =
	GST_STATIC_PAD_TEMPLATE (
		"sink",
		GST_PAD_SINK,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS ("image/jpeg, "
				 "width = (int) [ " G_STRINGIFY (MIN_WIDTH) ", " G_STRINGIFY (MAX_WIDTH) " ], "
				 "height = (int) [ " G_STRINGIFY (MIN_HEIGHT) ", " G_STRINGIFY (MAX_HEIGHT) " ], "
				 "framerate = (fraction) [ 0/1, MAX ]")
		);

#define LOG_CAPS(pad, caps)						\
{									\
	gchar* strcaps = gst_caps_to_string (caps);			\
	GST_INFO_OBJECT (pad, "caps = %s", strcaps ? strcaps : "NULL"); \
	g_free (strcaps);						\
}

#define DEBUG_CAPS(msg, caps)				\
{							\
	gchar* strcaps = gst_caps_to_string (caps);	\
	GST_DEBUG (msg, strcaps);			\
	g_free (strcaps);				\
}

#define GST_GOO_DECJPEG_SCALE (goo_ti_jpegdec_scale_get_type ())

GST_BOILERPLATE (GstGooDecJpeg, gst_goo_decjpeg, GstElement, GST_TYPE_ELEMENT);

static inline gboolean
is_jpeg_start_marker (const guint8* data)
{
	return (data[0] == 0xff && data[1] == 0xd8);
}

static inline gboolean
is_jpeg_end_marker (const guint8* data)
{
	return (data[0] == 0xff && data[1] == 0xd9);
}

static inline gboolean
gst_decjpeg_parse_tag_has_entropy_segment (guint8 tag)
{
	if (tag == 0xda || (tag >= 0xd0 && tag <= 0xd7))
	{
		return TRUE;
	}

	return FALSE;
}

/* Convert a 16 bit unsigned value from file's native byte order */
static inline gint
gst_decjpeg_get_16m (const guint8* data)
{
	return (((guint8 *) data)[0] << 8) | ((guint8 *) data)[1];
}

static gint
gst_decjpeg_get_color_format (const guint8* data)
{
	guchar Nf;
	gint j, i, temp;
	gshort H[4], V[4];

	Nf = data[7];

	if (Nf != 3)
	{
		goto done;
	}

	for (j = 0; j < Nf; j++)
	{
		i = j * 3 + 7 + 2;
		/* H[j]: upper 4 bits of a byte, horizontal sampling factor. */
		/* V[j]: lower 4 bits of a byte, vertical sampling factor.   */
		H[j] = (0x0f & (data[i] >> 4));
		/* printf ("h[%d] = %x\t", j, H[j]);  */
		V[j] = (0x0f & data[i]);
		/* printf ("v[%d] = %x\n", j, V[j]);  */
	}

	temp = (V[0] * H[0]) / (V[1] * H[1]);
	/* printf ("temp = %x\n", temp); */

	if (temp == 4 && H[0] == 2)
	{
		return OMX_COLOR_FormatYUV420PackedPlanar;
	}

	if (temp == 4 && H[0] == 4)
	{
		return OMX_COLOR_FormatYUV411Planar;
	}

	if (temp == 2)
	{
		return OMX_COLOR_FormatCbYCrY; /* YUV422 interleaved,
						  little endian */
	}

	if (temp == 1)
	{
		return OMX_COLOR_FormatYUV444Interleaved;
	}

done:
	return OMX_COLOR_FormatUnused;
}

static gboolean
gst_goo_decjpeg_find_header (GstGooDecJpeg* self)
{
	GstGooDecJpegPrivate* priv = GST_GOO_DECJPEG_GET_PRIVATE (self);

	const guint8 *data;
	guint size;

	data = GST_BUFFER_DATA (priv->tempbuf);
	size = GST_BUFFER_SIZE (priv->tempbuf);

	g_return_val_if_fail (size >= 2, FALSE);

	while (!is_jpeg_start_marker (data) || data[2] != 0xff) {
		const guint8 *marker;
		GstBuffer *tmp;
		guint off;

		marker = (guint8 *) memchr (data + 1, 0xff, size - 1 - 2);
		if (marker == NULL)
		{
			off = size - 1;		  /* keep last byte */
		}
		else
		{
			off = marker - data;
		}

		tmp = gst_buffer_create_sub (priv->tempbuf, off, size - off);
		gst_buffer_unref (priv->tempbuf);
		priv->tempbuf = tmp;

		data = GST_BUFFER_DATA (priv->tempbuf);
		size = GST_BUFFER_SIZE (priv->tempbuf);

		if (size < 2)
			return FALSE; /* wait for more data */
	}

	return TRUE;		      /* got header */
}

static gboolean
gst_goo_decjpeg_ensure_header (GstGooDecJpeg* self)
{
	GstGooDecJpegPrivate* priv = GST_GOO_DECJPEG_GET_PRIVATE (self);
	g_return_val_if_fail (priv->tempbuf != NULL, FALSE);

check_header:

	/* we need at least a start marker (0xff 0xd8)
	 *   and an end marker (0xff 0xd9) */
	if (GST_BUFFER_SIZE (priv->tempbuf) <= 4)
	{
		GST_DEBUG_OBJECT (self, "Not enough data");
		return FALSE;		    /* we need more data */
	}

	if (!is_jpeg_start_marker (GST_BUFFER_DATA (priv->tempbuf)))
	{
		GST_DEBUG_OBJECT (self,
				  "Not a JPEG header, resyncing to header...");
		if (!gst_goo_decjpeg_find_header (self))
		{
			GST_DEBUG_OBJECT (self,
					  "No JPEG header in current buffer");
			return FALSE;		  /* we need more data */
		}
		GST_DEBUG_OBJECT (self, "Found JPEG header");
		goto check_header;	    /* buffer might have changed */
	}

	return TRUE;
}

/* returns image length in bytes if parsed
 * successfully, otherwise 0 */
static guint
gst_goo_decjpeg_parse_image_data (GstGooDecJpeg* self)
{
	GstGooDecJpegPrivate* priv = GST_GOO_DECJPEG_GET_PRIVATE (self);

	guint8 *start, *data, *end;
	guint size;

	size = GST_BUFFER_SIZE (priv->tempbuf);
	start = GST_BUFFER_DATA (priv->tempbuf);
	end = start + size;
	data = start;

	g_return_val_if_fail (is_jpeg_start_marker (data), 0);

	GST_DEBUG_OBJECT (self, "Parsing jpeg image data (%u bytes)", size);

	/* skip start marker */
	data += 2;

	while (TRUE) {
		guint frame_len;

		/* enough bytes left for EOI marker?
		   (we need 0xff 0xNN, thus end-1) */
		if (data >= end - 1)
		{
			GST_DEBUG_OBJECT (self, "at end of input and no EOI "
					  "marker found, need more data");
			return 0;
		}

		if (is_jpeg_end_marker (data))
		{
			GST_DEBUG_OBJECT (self,
					  "0x%08x: end marker", data - start);
			goto found_eoi;
		}

		/* do we need to resync? */
		if (*data != 0xff)
		{
			GST_DEBUG_OBJECT (self,
					  "Lost sync at 0x%08x, resyncing",
					  data - start);
			/* at the very least we expect 0xff 0xNN, thus end-1 */
			while (*data != 0xff && data < end - 1)
			{
				++data;
			}
			if (is_jpeg_end_marker (data))
			{
				GST_DEBUG_OBJECT (self, "resynced to end marker");
				goto found_eoi;
			}
			/* we need 0xFF 0xNN 0xLL 0xLL */
			if (data >= end - 1 - 2)
			{
				GST_DEBUG_OBJECT (self, "at end of input, "
						  "without new sync, need "
						  "more data");
				return 0;
			}
			/* check if we will still be in sync if we interpret
			 * this as a sync point and skip this frame */
			frame_len = GST_READ_UINT16_BE (data + 2);
			GST_DEBUG_OBJECT (self, "possible sync at 0x%08x, "
					  "frame_len=%u",
				   data - start, frame_len);
			if (data + 2 + frame_len >= end - 1 ||
			    data[2 + frame_len] != 0xff)
			{
				/* ignore and continue resyncing until we hit
				   the end of our data or find a sync point
				   that looks okay */
				++data;
				continue;
			}
			GST_DEBUG_OBJECT (self, "found sync at %p",
					  data - size);
		}
		while (*data == 0xff)
		{
			++data;
		}
		if (data + 2 >= end)
		{
			return 0;
		}
		if (*data >= 0xd0 && *data <= 0xd7)
		{
			frame_len = 0;
		}
		else
		{
			frame_len = GST_READ_UINT16_BE (data + 1);
		}
		GST_DEBUG_OBJECT (self, "0x%08x: tag %02x, frame_len=%u",
				  data - start - 1, *data, frame_len);
		/* the frame length includes the 2 bytes for the length; here
		 * we want at least 2 more bytes at the end for an end marker,
		 * thus end-2 */
		if (data + 1 + frame_len >= end - 2)
		{
			/* theoretically we could have lost sync and not
			 * really need more data, but that's just tough luck
			 * and a broken image then */
			GST_DEBUG_OBJECT (self, "at end of input and no EOI "
					  "marker found, need more data");
			return 0;
		}
		if (gst_decjpeg_parse_tag_has_entropy_segment (*data)) /* <--- */
		{
			guint8 *d2 = data + 1 + frame_len;
			guint eseglen = 0;

			GST_DEBUG_OBJECT (self, "0x%08x: finding entropy "
					  "segment length", data - start - 1);
			while (TRUE)
			{
				if (d2[eseglen] == 0xff &&
				    d2[eseglen + 1] != 0x00)
				{
					break;
				}
				if (d2 + eseglen >= end - 1)
				{
					return 0;	  /* need more data */
				}
				++eseglen;

			}
			frame_len += eseglen;
			GST_DEBUG_OBJECT (self, "entropy segment length=%u => "
					  "frame_len=%u", eseglen, frame_len);
		}
		data += 1 + frame_len;
	}

found_eoi:
	/* data is assumed to point to the 0xff sync point of the
	 *  EOI marker (so there is one more byte after that) */
	g_assert (is_jpeg_end_marker (data));
	return ((data + 1) - start + 1);
}

static gboolean
gst_goo_decjpeg_read_header (GstGooDecJpeg* self)
{
	GstGooDecJpegPrivate* priv = GST_GOO_DECJPEG_GET_PRIVATE (self);

	guint8 *start, *data, *end, *d = NULL;
	guint size;

	size = GST_BUFFER_SIZE (priv->tempbuf);
	start = GST_BUFFER_DATA (priv->tempbuf);
	end = start + size;
	data = start;

	g_return_val_if_fail (is_jpeg_start_marker (data), FALSE);

	gint sectionsread = 0;

	/* skip start marker */
	data += 2;

	for (sectionsread = 0; sectionsread < MAX_SECTIONS - 1;)
	{
		gint itemlen;
		guint8 marker;
		gint a, ll, lh, got;

		for (a = 0; a < 7; a++)
		{
			marker = *data++;
			if (marker != 0xff)
			{
				break;
			}

			if (a >= 6)
			{
				GST_INFO ("MAX sections");
				goto fail;
			}
		}

		if (marker == 0xff)
		{
			/* 0xff is legal padding, but if we get that many,
			   something's wrong. */
			GST_INFO_OBJECT (self, "To many paddings");
			goto fail;
		}

		itemlen = gst_decjpeg_get_16m (data);
		GST_INFO_OBJECT (self, "marker = %x | itemlen = %d",
				 marker, itemlen);

		if (itemlen < 2)
		{
			/* Invalid marker */
			GST_INFO_OBJECT (self, "Invalid marker");
			goto fail;
		}

		d = g_new (guint8, itemlen);
		memcpy (d, data, itemlen);
		data += itemlen;

		sectionsread++;

		switch (marker)
		{
		case M_SOS:
			goto done;
			break;

		case M_EOI:
			GST_INFO_OBJECT (self, "Premature EOI");
			goto fail;
			break;

		case M_COM:  /* Comment section */
		case M_JFIF: /* non exif image tag */
		case M_EXIF: /* exif image tag */
			break;

		case M_SOF2:
			priv->progressive = TRUE;

		case M_SOF0:
		case M_SOF1:
		case M_SOF3:
		case M_SOF5:
		case M_SOF6:
		case M_SOF7:
		case M_SOF9:
		case M_SOF10:
		case M_SOF11:
		case M_SOF13:
		case M_SOF14:
		case M_SOF15:
		{
			GST_DEBUG_OBJECT (self, "Setting size and color");

			priv->height = gst_decjpeg_get_16m (d + 3);
			priv->width = gst_decjpeg_get_16m (d + 5);
			priv->in_color = gst_decjpeg_get_color_format (d);
			break;
		}
		default:
			break;
		}

		if (G_LIKELY (d))
		{
			g_free (d);
			d = NULL;
		}
	}

fail:
	if (G_LIKELY (d))
	{
		g_free (d);
		d = NULL;
	}
	return FALSE;

done:
	if (G_LIKELY (d))
	{
		g_free (d);
		d = NULL;
	}

	GST_INFO_OBJECT (self,
			 "* progresive = %d | width = %d | height = %d | "
			 "color = %s | size = %d",
			 priv->progressive, priv->width, priv->height,
			 goo_strcolor (priv->in_color), priv->imagesiz);

	return TRUE;
}

static gboolean
gst_goo_decjpeg_sink_setcaps (GstPad* pad, GstCaps* caps)
{
	GstGooDecJpeg* self = GST_GOO_DECJPEG (gst_pad_get_parent (pad));
	GstGooDecJpegPrivate* priv = GST_GOO_DECJPEG_GET_PRIVATE (self);

	GstStructure* structure;
	const GValue* framerate;

	/* do not extract width/height here. we do that in the chain
	 * function on a per-frame basis (including the line[] array
	 * setup) */

	/* But we can take the framerate values and set them on the src pad */

	structure = gst_caps_get_structure (caps, 0);

	framerate = gst_structure_get_value (structure, "framerate");

	if (G_LIKELY (framerate))
	{
		priv->framerate_numerator =
			gst_value_get_fraction_numerator (framerate);
		priv->framerate_denominator =
			gst_value_get_fraction_denominator (framerate);
		priv->packetized = TRUE;
		GST_DEBUG_OBJECT (self, "got framerate of %d/%d "
				  "fps => packetized mode",
				  priv->framerate_numerator,
				  priv->framerate_denominator);
	}

	gst_object_unref (self);

	return TRUE;
}

static gboolean
gst_goo_decjpeg_src_setcaps (GstPad* pad, GstCaps* caps)
{
	GstGooDecJpeg* self = GST_GOO_DECJPEG (gst_pad_get_parent (pad));
	GstGooDecJpegPrivate* priv = GST_GOO_DECJPEG_GET_PRIVATE (self);

	GstStructure* structure;
	guint32 fourcc;

	structure = gst_caps_get_structure (caps, 0);

	GST_DEBUG_OBJECT (self, "src pad structure: %s",
			  gst_structure_get_name (structure));

	if (g_strrstr (gst_structure_get_name (structure), "video/x-raw-yuv"))
	{
		if (gst_structure_get_fourcc (structure, "format", &fourcc))
		{
			switch (fourcc)
			{
			case GST_MAKE_FOURCC ('Y', 'U', 'Y', '2'):
				priv->out_color = OMX_COLOR_FormatYCbYCr;
				break;
			case GST_MAKE_FOURCC ('U', 'Y', 'V', 'Y')://422LE
				priv->out_color = OMX_COLOR_FormatCbYCrY;
				break;
			default:
				return FALSE;
			}
		}
	}
	else if (g_strrstr (gst_structure_get_name (structure),
			    "video/x-raw-rgb"))
	{
		gint bpp;
		if (gst_structure_get_int (structure, "bpp", &bpp))
		{
			switch (bpp)
			{
			case 16:
				priv->out_color = OMX_COLOR_Format16bitRGB565;
				break;
			case 24:
				priv->out_color = OMX_COLOR_Format24bitRGB888;

				break;
			case 32:
				priv->out_color = OMX_COLOR_Format32bitARGB8888;

				break;
			default:
				return FALSE;
			}
		}
	}
	else
	{
		return FALSE;
	}

	gst_object_unref (self);

	return TRUE;
}

static GstCaps*
get_possible_outcaps (GstGooDecJpeg* self)
{
	GST_DEBUG_OBJECT (self, "");
	GstGooDecJpegPrivate* priv = GST_GOO_DECJPEG_GET_PRIVATE (self);

	gfloat scalefactor = (priv->scalefactor != 12) ?
		priv->scalefactor * 1.0 / 100.0 :
		12.5 / 100.0;

	guint width = priv->width * scalefactor;
	guint height = priv->height * scalefactor;

	/*Subregion decoding negotiation*/

	if((priv->scalefactor == SCALE_DEFAULT) && (priv->subregion_width != 0 && priv->subregion_height != 0) ){
		width = priv->subregion_width;
		height = priv->subregion_height;
	}


        /* framerate == 0/1 is a still frame */
        if (priv->framerate_denominator == 0)
        {
                priv->framerate_numerator = 0;
                priv->framerate_denominator = 1;
        }

	GstCaps* caps = gst_caps_new_empty ();

        if (priv->in_color != OMX_COLOR_FormatYUV411Planar)
        {
		/* OMX_COLOR_FormatCbYCrY */
                gst_caps_append_structure (caps,
                         gst_structure_new ("video/x-raw-yuv",
                                            "format", GST_TYPE_FOURCC,
                                            GST_MAKE_FOURCC ('U','Y','V','Y'),
                                            "width", G_TYPE_INT, width,
                                            "height", G_TYPE_INT, height,
                                            "framerate", GST_TYPE_FRACTION,
                                            priv->framerate_numerator,
                                            priv->framerate_denominator,
                                            NULL));
		priv->factor_color = 2.0;//422
        }




        if (priv->in_color != OMX_COLOR_FormatYUV444Interleaved)
        {
		/* OMX_COLOR_FormatYUV420PackedPlanar */
		gst_caps_append_structure (caps,
                         gst_structure_new ("video/x-raw-yuv",
                                            "format", GST_TYPE_FOURCC,
                                            GST_MAKE_FOURCC ('I','4','2','0'),
                                            "width", G_TYPE_INT, width,
                                            "height", G_TYPE_INT, height,
                                            "framerate", GST_TYPE_FRACTION,
                                            priv->framerate_numerator,
                                            priv->framerate_denominator,
                                            NULL));
		if(priv->factor_color == 0.0)
			priv->factor_color = 1.5;//420
        }



        /* OMX_COLOR_Format16bitRGB565 */
	gst_caps_append_structure (caps,
		gst_structure_new ("video/x-raw-rgb",
				   "bpp", G_TYPE_INT, 16,
				   "depth", G_TYPE_INT, 16,
				   "endianness", G_TYPE_INT,  G_BYTE_ORDER,
				   "red_mask", G_TYPE_INT, 0xf800,
				   "green_mask", G_TYPE_INT, 0x07e0,
				   "blue_mask", G_TYPE_INT, 0x001f,
				   "width", G_TYPE_INT, width,
				   "height", G_TYPE_INT, height,
				   "framerate", GST_TYPE_FRACTION,
				   priv->framerate_numerator,
				   priv->framerate_denominator, NULL));


        /* OMX_COLOR_Format24bitRGB888 */
        gst_caps_append_structure (caps,
		gst_structure_new ("video/x-raw-rgb",
				   "bpp", G_TYPE_INT, 24,
				   "depth", G_TYPE_INT, 24,
				   "endianness", G_TYPE_INT,  BIG_ENDIAN,
				   "red_mask", G_TYPE_INT, 0x00ff0000,
				   "green_mask", G_TYPE_INT, 0x0000ff00,
				   "blue_mask", G_TYPE_INT, 0x000000ff,
				   "width", G_TYPE_INT, width,
				   "height", G_TYPE_INT, height,
				   "framerate", GST_TYPE_FRACTION,
				   priv->framerate_numerator,
				   priv->framerate_denominator, NULL));



        /* OMX_COLOR_Format32bitARGB8888 */
        gst_caps_append_structure (caps,
		gst_structure_new ("video/x-raw-rgb",
				   "bpp", G_TYPE_INT, 32,
				   "depth", G_TYPE_INT, 32,
				   "endianness", G_TYPE_INT,  BIG_ENDIAN,
				   "red_mask", G_TYPE_INT,   0x000000ff,
				   "green_mask", G_TYPE_INT, 0x0000ff00,
				   "blue_mask", G_TYPE_INT,  0x00ff0000,
				   "alpha_mask", G_TYPE_INT, 0xff000000,
				   "width", G_TYPE_INT, width,
				   "height", G_TYPE_INT, height,
				   "framerate", GST_TYPE_FRACTION,
				   priv->framerate_numerator,
				   priv->framerate_denominator, NULL));


        priv->caps_width = width;
        priv->caps_height = height;
        priv->caps_framerate_numerator = priv->framerate_numerator;
        priv->caps_framerate_denominator = priv->framerate_denominator;

        if (priv->in_color == OMX_COLOR_Format16bitRGB565)
		priv->factor_color = 2.0;
	else if (priv->in_color == OMX_COLOR_Format24bitRGB888 )
		priv->factor_color = 3.0;
	else if (priv->in_color == OMX_COLOR_Format32bitARGB8888)
		priv->factor_color = 4.0;



	if (caps != NULL)
	{
		DEBUG_CAPS ("Proposed caps %s", caps);
	}
	else
	{
		GST_DEBUG_OBJECT (self, "Caps are null!");
	}

	return caps;
}

/* given a set of possible output caps, lets negociate with the peer pad */
static gboolean
negotiate_caps (GstGooDecJpeg* self)
{
	GST_DEBUG_OBJECT (self, "");

	GstGooDecJpegPrivate* priv = GST_GOO_DECJPEG_GET_PRIVATE (self);
	gboolean ret = TRUE;

	GstPad* otherpeer = gst_pad_get_peer (self->srcpad);
	GstCaps* othercaps = get_possible_outcaps (self);

	if (othercaps != NULL)
	{
		GstCaps* intersect;
		const GstCaps *templ_caps;

		templ_caps = gst_pad_get_pad_template_caps (self->srcpad);
		intersect = gst_caps_intersect (othercaps, templ_caps);
                gst_caps_unref (othercaps);
		othercaps = intersect;
	}

	if (othercaps == NULL || gst_caps_is_empty (othercaps))
	{
		goto no_transform;
	}

	if (!gst_caps_is_fixed (othercaps) && otherpeer != NULL)
	{
		GstCaps* peercaps = gst_pad_get_caps (otherpeer);
		GstCaps* intersect = gst_caps_intersect (peercaps, othercaps);
		gst_caps_unref (peercaps);
		gst_caps_unref (othercaps);

		othercaps = intersect;
	}

	if (gst_caps_is_empty (othercaps))
	{
		goto no_transform;
	}

	if (!gst_caps_is_fixed (othercaps))
	{
		GstCaps* temp = gst_caps_copy_nth (othercaps, 0);
		gst_caps_unref (othercaps);
		othercaps = temp;
		gst_pad_fixate_caps (self->srcpad, othercaps);
	}

	/* GST_DEBUG_OBJECT (self, "setting caps %" GST_PTR_FORMAT, caps); */

	if (!gst_caps_is_fixed (othercaps))
	{
		goto could_not_fixate;
	}

	if (otherpeer != NULL && !gst_pad_accept_caps (otherpeer, othercaps))
	{
		goto peer_no_accept;
	}

	DEBUG_CAPS ("setting caps = %s", othercaps);



	gst_pad_set_caps (self->srcpad, othercaps);

done:
	if (otherpeer)
	{
		gst_object_unref (otherpeer);
	}
	if (othercaps)
	{
		gst_caps_unref (othercaps);
	}

	return ret;

no_transform:
	{
		DEBUG_CAPS ("negotiation returned useless %s", othercaps);
		ret = FALSE;
		goto done;
	}

could_not_fixate:
	{
		DEBUG_CAPS ("FAILED to fixate %s", othercaps);
		ret = FALSE;
		goto done;
	}

peer_no_accept:
	{
		LOG_CAPS (self->srcpad, othercaps);
		ret = FALSE;
		goto done;
	}
}

static void
omx_sync (GstGooDecJpeg* self)
{
	GstGooDecJpegPrivate* priv = GST_GOO_DECJPEG_GET_PRIVATE (self);

	GST_DEBUG_OBJECT (self, "configuring params");
	OMX_PARAM_PORTDEFINITIONTYPE* param;

	param = GOO_PORT_GET_DEFINITION (self->inport);
	param->format.image.nFrameWidth = priv->width;
	param->format.image.nFrameHeight = priv->height;
	param->format.image.eColorFormat = priv->in_color;
	param->nBufferSize = priv->imagesiz;

	param = GOO_PORT_GET_DEFINITION (self->outport);
	param->format.image.eColorFormat = priv->out_color;

	g_object_set (self->component, "scale", priv->scalefactor, NULL);
	g_object_set (self->component, "progressive", priv->progressive, NULL);

	/* Section decoding*/
	GOO_TI_JPEGDEC_GET_SECTION_DECODE (self->component)->nMCURow = priv->num_slices;

	/* SubRegion decoding*/
	GOO_TI_JPEGDEC_GET_SUBREGION_DECODE (self->component)->nXOrg = priv->subregion_left;
	GOO_TI_JPEGDEC_GET_SUBREGION_DECODE (self->component)->nYOrg = priv->subregion_top;
	GOO_TI_JPEGDEC_GET_SUBREGION_DECODE (self->component)->nXLength = priv->subregion_width;
	GOO_TI_JPEGDEC_GET_SUBREGION_DECODE (self->component)->nYLength = priv->subregion_height;


	return;
}

static gboolean
configure_omx (GstGooDecJpeg* self)
{


	g_assert (GST_IS_GOO_DECJPEG (self));

	GooComponent *component = self->component;

#if 1
	if (goo_component_get_state (component) == OMX_StateExecuting)
	{
		GST_INFO ("going to idle");
		goo_component_set_state_idle (component);
	}

	if (goo_component_get_state (component) == OMX_StateIdle)
	{
		GST_INFO ("going to loaded");
		goo_component_set_state_loaded (component);
	}
#endif

	omx_sync (self);

	GST_INFO ("going to idle");
	goo_component_set_state_idle (self->component);

	GST_INFO ("going to executing");
	goo_component_set_state_executing (self->component);

	return TRUE;
}

static GstFlowReturn
process_output_buffer (GstGooDecJpeg* self, OMX_BUFFERHEADERTYPE* buffer){
	GstBuffer* out = gst_goo_buffer_new();
	GstFlowReturn ret = GST_FLOW_OK;

	GstGooDecJpegPrivate* priv = GST_GOO_DECJPEG_GET_PRIVATE (self);

	if (buffer->nFilledLen <= 0)
	{
		GST_INFO_OBJECT (self, "Received an empty buffer!");
		goo_component_release_buffer (self->component, buffer);
		return GST_FLOW_OK;
	}

	if (priv->num_slices > 0)
		{
			guint size_to_copy = 0;
			if ((buffer->nFilledLen+priv->count) > priv->size )
				size_to_copy = priv->size - priv->count;
			else
				size_to_copy = buffer->nFilledLen;
			memmove (GST_BUFFER_DATA(priv->buffer_slice) + priv->count,
				 buffer->pBuffer, size_to_copy);
			GST_DEBUG_OBJECT (self, "Output buffer size %d, %d", buffer->nFilledLen,priv->count );
			priv->count += size_to_copy;

		}
	else
		{
			/*There is no section decoding */

			memmove (GST_BUFFER_DATA (priv->buffer_slice),
				 buffer->pBuffer, buffer->nFilledLen);
			GST_DEBUG_OBJECT (self, "Output buffer size %d", buffer->nFilledLen );

		}
	goo_component_release_buffer (self->component, buffer);


	GST_INFO_OBJECT (self, "");

	return ret;
}

static GstFlowReturn gst_goo_decjpeg_process_input_buffer (GstGooDecJpeg* self);


static GstFlowReturn
gst_goo_decjpeg_chain (GstPad* pad, GstBuffer* buffer)
{
	GST_LOG ("");

	GstGooDecJpeg* self = GST_GOO_DECJPEG (gst_pad_get_parent (pad));
	GstGooDecJpegPrivate* priv = GST_GOO_DECJPEG_GET_PRIVATE (self);
	GstFlowReturn ret = GST_FLOW_OK;

	GstClockTime timestamp;
	guint8 *data;
	gulong size;

	if (goo_port_is_tunneled (self->inport))
	{
		/* shall we send a ghost buffer here ? */
		GST_INFO ("port is tunneled");

		ret = GST_FLOW_OK;
		goto process_output;
	}

	if (goo_port_is_eos (self->inport))
	{
		GST_INFO ("port is eos");

		ret = GST_FLOW_UNEXPECTED;
		goto done;
	}

	timestamp = GST_BUFFER_TIMESTAMP (buffer);
	priv->duration = GST_BUFFER_DURATION (buffer);

	if (GST_CLOCK_TIME_IS_VALID (timestamp))
	{
		priv->next_ts = timestamp;
	}

	if (G_LIKELY (priv->tempbuf))
	{
		/* if the src is not emitting all buffers as a child of a common
		 * parent buffer, this could be really expensive.. why not just
		 * copy directly to an OMX buffer?
		 *
		 * btw, if using a filesrc, setting "use-mmap=1", and if necessary
		 * increasing "mmapsize" to a value large enough to map the entire
		 * input file in one shot, will result in all buffers having a common
		 * parent so that this doesn't perform too badly..
		 */
		priv->tempbuf = gst_buffer_join (priv->tempbuf, buffer);
	}
	else
	{
		priv->tempbuf = buffer;
	}

	GST_INFO ("offset=%d, offset_end=%d", buffer->offset, buffer->offset_end);

	/* Don't bother trying to start parsing the data until we get the entire
	 * image... we could get many 100's of more buffers before we get the
	 * entire image, and we don't want to do all the header parsing each time:
	 */
	if (priv->packetized)
	{
		ret = gst_goo_decjpeg_process_input_buffer (self);
	}
	else
	{
		ret = GST_FLOW_OK;
	}

process_output:
	if (goo_port_is_tunneled (self->outport))
	{

		GstBuffer* buffer = gst_ghost_buffer_new ();
		/* gst_buffer_set_caps (buffer, GST_PAD_CAPS (self->srcpad)); */
		gst_pad_push (self->srcpad, buffer);

		goto done;
	}

done:
	gst_object_unref (self);

	return ret;
}

static GstFlowReturn
gst_goo_decjpeg_process_input_buffer (GstGooDecJpeg* self)
{
	GstGooDecJpegPrivate* priv = GST_GOO_DECJPEG_GET_PRIVATE (self);
	GstFlowReturn ret = GST_FLOW_OK;

	if (!gst_goo_decjpeg_ensure_header (self)) /* <--- */
	{
		goto need_more_data;
	}

	/* If we know that each input buffer contains data
	 * for a whole jpeg image (e.g. MJPEG streams), just
	 * do some sanity checking instead of parsing all of
	 * the jpeg data */
	if (priv->packetized == TRUE)
	{
		priv->imagesiz = GST_BUFFER_SIZE (priv->tempbuf);
	}
	else
	{
		/* Parse jpeg image to handle jpeg input that
		 * is not aligned to buffer boundaries */
		priv->imagesiz = gst_goo_decjpeg_parse_image_data (self); /* <--- */

		if (priv->imagesiz == 0)
		{
			goto need_more_data;
		}
	}

	GST_LOG_OBJECT (self, "image size = %u", priv->imagesiz);

	gst_goo_decjpeg_read_header (self); /* <--- */

	if (priv->width < MIN_WIDTH || priv->width > MAX_WIDTH ||
	    priv->height < MIN_HEIGHT || priv->height > MAX_HEIGHT)
	{
		goto wrong_size;
	}

	gfloat scalefactor = (priv->scalefactor != 12) ?
		priv->scalefactor * 1.0 / 100.0 :
		12.5 / 100.0;
	guint width = priv->width * scalefactor;
	guint height = priv->height * scalefactor;

	/* Subregion decoding */

	if(priv->scalefactor == SCALE_DEFAULT && priv->subregion_width != 0 && priv->subregion_width != 0){

		width = priv->subregion_width;
		height = priv->subregion_height;

		if (width < MIN_WIDTH || width > MAX_WIDTH ||
		    height < MIN_HEIGHT || height > MAX_HEIGHT)
			{
				goto wrong_size;
			}
	}


	if (width != priv->caps_width || height != priv->caps_height ||
	    priv->framerate_numerator != priv->caps_framerate_numerator ||
	    priv->framerate_denominator != priv->caps_framerate_denominator)
	{

		if (!negotiate_caps (self))
		{
			goto negotiation_failure;
		}

		if (!configure_omx (self))
		{
			/* @todo */
		}

	}

	/* if we aren't ready yet */
	if (self->component->cur_state != OMX_StateExecuting)
	{
		goto done;
	}
	else /* we are ready! */
	{
		GST_DEBUG_OBJECT (self, "Pushing data to OMX");
		OMX_BUFFERHEADERTYPE* omx_buffer;
		omx_buffer = goo_port_grab_buffer (self->inport);
		omx_buffer->nFlags = OMX_BUFFERFLAG_EOS;
		memmove (omx_buffer->pBuffer,
			 GST_BUFFER_DATA (priv->tempbuf),
			 GST_BUFFER_SIZE (priv->tempbuf));
		omx_buffer->nFilledLen = GST_BUFFER_SIZE (priv->tempbuf);
		priv->incount++;
		goo_component_release_buffer (self->component, omx_buffer);
		ret = GST_FLOW_OK;

		priv->size = width*height*priv->factor_color;


		/* Ask for memory for buffer_slice */
		priv->buffer_slice = gst_buffer_new_and_alloc(priv->size);
	}

#if 0
        OMX_BUFFERHEADERTYPE* omx_buffer;
        {
                GST_DEBUG_OBJECT (self, "Popping data from OMX");
                omx_buffer = goo_port_grab_buffer (self->outport);

                GST_INFO_OBJECT (self, "Output size = %d",
                                 omx_buffer->nFilledLen);

                if (omx_buffer->nFilledLen <= 0)
                {
                        ret = GST_FLOW_OK;
                        goto done;
                }
        }

        GstBuffer* outbuf = NULL;
        if (gst_pad_alloc_buffer (self->srcpad, priv->outcount,
				  omx_buffer->nFilledLen,
				  GST_PAD_CAPS (self->srcpad),
				  &outbuf) == GST_FLOW_OK)
        {
                memmove (GST_BUFFER_DATA (outbuf), omx_buffer->pBuffer,
                         omx_buffer->nFilledLen);
                goo_component_release_buffer (self->component, omx_buffer);
        }

        /* GstBuffer* gst_buffer = gst_goo_buffer_new (); */
        /* gst_goo_buffer_set_data (gst_buffer, self->component, omx_buffer); */
        /* gst_buffer_set_caps (gst_buffer, GST_PAD_CAPS (self->srcpad)); */

        GST_BUFFER_OFFSET (outbuf) = priv->outcount++;
        GST_BUFFER_TIMESTAMP (outbuf) = priv->next_ts;

        if (priv->packetized &&
            GST_CLOCK_TIME_IS_VALID (priv->next_ts))
        {
                if (GST_CLOCK_TIME_IS_VALID (priv->duration))
                {
                        /* use duration from incoming buffer
                           for outgoing buffer */
                        priv->next_ts += priv->duration;
                }
                else if (priv->framerate_numerator != 0)
                {
                        priv->duration = gst_util_uint64_scale
                                (GST_SECOND,
                                 priv->framerate_denominator,
                                 priv->framerate_numerator);
                        priv->next_ts += priv->duration;
                }
                else
                {
                        priv->duration = GST_CLOCK_TIME_NONE;
                        priv->next_ts = GST_CLOCK_TIME_NONE;
                }
        }
        else
        {
                priv->duration = GST_CLOCK_TIME_NONE;
                priv->next_ts = GST_CLOCK_TIME_NONE;
        }
        GST_BUFFER_DURATION (outbuf) = priv->duration;

        ret = gst_pad_push (self->srcpad, outbuf);

#endif
	ret=GST_FLOW_OK;



done:
	if (GST_BUFFER_SIZE (priv->tempbuf) == priv->imagesiz)
	{
		gst_buffer_unref (priv->tempbuf);
		priv->tempbuf = NULL;
	}
	else
	{
		GstBuffer *buf = gst_buffer_create_sub
			(priv->tempbuf, priv->imagesiz,
			 GST_BUFFER_SIZE (priv->tempbuf) - priv->imagesiz);

		gst_buffer_unref (priv->tempbuf);
		priv->tempbuf = buf;
	}

exit:

	return ret;

	/* special cases */
need_more_data:
	{
		GST_LOG_OBJECT (self, "we need more data");
		ret = GST_FLOW_OK;
		goto exit;
	}
	/* ERRORS */
wrong_size:
	{
		GST_ELEMENT_ERROR (self, STREAM, DECODE,
				   ("Picture is too small or too big (%ux%u)",
				    priv->width, priv->height),
				   ("Picture is too small or too big (%ux%u)",
				    priv->width, priv->height));
		ret = GST_FLOW_ERROR;
		goto done;
	}
negotiation_failure:
	{
		GST_ELEMENT_ERROR (self, STREAM, DECODE,
				   ("Can't negotiate source pad--"),
				   ("Can't negotiate source pad--"));
		ret = GST_FLOW_ERROR;
		goto done;
	}
}

static void
gst_goo_jpeg_dec_wait_for_done(GstGooDecJpeg* self)
{
	g_assert (self != NULL);

	goo_component_wait_for_done (self->component);

	return;
}


static gboolean
gst_goo_decjpeg_event (GstPad* pad, GstEvent* event)
{
	GST_LOG ("");

	GstGooDecJpeg* self = GST_GOO_DECJPEG (gst_pad_get_parent (pad));
	GstGooDecJpegPrivate* priv = GST_GOO_DECJPEG_GET_PRIVATE (self);
	gboolean ret;

	g_assert (self->component != NULL);

	switch (GST_EVENT_TYPE (event))
	{
	case GST_EVENT_NEWSEGMENT:
	{
		gboolean update;
		gdouble rate, applied_rate;
		GstFormat format;
		gint64 start, stop, position;

		gst_event_parse_new_segment_full (event,
						  &update,
						  &rate,
						  &applied_rate,
						  &format,
						  &start,
						  &stop,
						  &position);

		GST_DEBUG_OBJECT (self, "Got NEWSEGMENT [%" GST_TIME_FORMAT
				  " - %" GST_TIME_FORMAT " / %"
				  GST_TIME_FORMAT "]",
				  GST_TIME_ARGS (start), GST_TIME_ARGS (stop),
				  GST_TIME_ARGS (position));

		gst_segment_set_newsegment_full (priv->segment,
						 update,
						 rate,
						 applied_rate,
						 format,
						 start,
						 stop,
						 position);
		ret = gst_pad_event_default (pad, event);
		break;
	}
	case GST_EVENT_EOS:
		{
			GST_INFO_OBJECT (self, "EOS event");
			if (!priv->packetized)
			{
				gst_goo_decjpeg_process_input_buffer (self);
			}
			if (goo_component_get_state (self->component) == OMX_StateExecuting)
				gst_goo_jpeg_dec_wait_for_done(self);
			ret = gst_pad_push_event (self->srcpad, event);
			break;
		}


	default:
		ret = gst_pad_event_default (pad, event);
		break;
	}

	gst_object_unref (self);
	return ret;
}

static GstStateChangeReturn
gst_goo_decjpeg_change_state (GstElement* element, GstStateChange transition)
{
	GST_LOG ("");

	GstGooDecJpeg* self = GST_GOO_DECJPEG (element);
	GstStateChangeReturn result;
	GstGooDecJpegPrivate* priv = GST_GOO_DECJPEG_GET_PRIVATE (self);

	g_assert (self->component != NULL);
	g_assert (self->inport != NULL);
	g_assert (self->outport != NULL);

	switch (transition)
	{
	case GST_STATE_CHANGE_NULL_TO_READY:
		break;
	case GST_STATE_CHANGE_READY_TO_PAUSED:
		priv->width = -1;
		priv->height = -1;
		priv->framerate_numerator = 0;
		priv->framerate_denominator = 1;
		priv->packetized = FALSE;
		priv->next_ts = 0;
		gst_segment_init (priv->segment, GST_FORMAT_UNDEFINED);
		break;
	case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
		break;
	default:
		break;
	}

	result = GST_ELEMENT_CLASS (parent_class)->change_state (element,
								 transition);

	switch (transition)
	{
	case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
		/* goo_component_set_state_paused (self->component); */
		break;
	case GST_STATE_CHANGE_PAUSED_TO_READY:

		if (goo_component_get_state (self->component) ==
		    OMX_StateExecuting)
		{
			GST_INFO ("going to idle");
			goo_component_set_state_idle (self->component);
		}

		if (goo_component_get_state (self->component) ==
		    OMX_StateIdle)
		{
			GST_INFO ("going to loaded");
			goo_component_set_state_loaded (self->component);
		}

		break;
	case GST_STATE_CHANGE_READY_TO_NULL:
		break;
	default:
		break;
	}

	return result;
}

static void
gst_goo_decjpeg_set_property (GObject* object, guint prop_id,
			      const GValue* value, GParamSpec* pspec)
{
	GstGooDecJpegPrivate* priv = GST_GOO_DECJPEG_GET_PRIVATE (object);
	GstGooDecJpeg* self = GST_GOO_DECJPEG (object);

	switch (prop_id)
	{
	case PROP_RESIZE:
		priv->scalefactor = (guint) g_value_get_enum (value);
		break;
		/* Section Decoding*/
	case PROP_SLICING:
		priv->num_slices = g_value_get_uint (value);
		break;
		/* SubRegion Decoding*/
	case PROP_SUBREGIONLEFT:
		priv->subregion_left = g_value_get_uint (value);
		break;
	case PROP_SUBREGIONTOP:
		priv->subregion_top = g_value_get_uint (value);
		break;
	case PROP_SUBREGIONWIDTH:
		priv->subregion_width = g_value_get_uint (value);
		break;
	case PROP_SUBREGIONHEIGHT:
		priv->subregion_height = g_value_get_uint (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_decjpeg_get_property (GObject* object, guint prop_id,
			      GValue* value, GParamSpec* pspec)
{
	GstGooDecJpegPrivate* priv = GST_GOO_DECJPEG_GET_PRIVATE (object);
	GstGooDecJpeg* self = GST_GOO_DECJPEG (object);

	switch (prop_id)
	{
	case PROP_RESIZE:
		g_value_set_enum (value, priv->scalefactor);
		break;
	case PROP_SLICING:
		g_value_set_uint (value, priv->num_slices);
		break;
		/* SubRegion Decoding*/
	case PROP_SUBREGIONLEFT:
		g_value_set_uint (value, priv->subregion_left);
		break;
	case PROP_SUBREGIONTOP:
		g_value_set_uint (value, priv->subregion_top);
		break;
	case PROP_SUBREGIONWIDTH:
		g_value_set_uint (value, priv->subregion_width);
		break;
	case PROP_SUBREGIONHEIGHT:
		g_value_set_uint (value, priv->subregion_height);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
	return;
}

static void
gst_goo_decjpeg_dispose (GObject* object)
{

	G_OBJECT_CLASS (parent_class)->dispose (object);

	GstGooDecJpeg* me = GST_GOO_DECJPEG (object);

	if (G_LIKELY (me->inport))
	{
		GST_DEBUG ("unrefing outport");
		g_object_unref (me->inport);
	}

	if (G_LIKELY (me->outport))
	{
		GST_DEBUG ("unrefing outport");
		g_object_unref (me->outport);
	}

	if (G_LIKELY (me->component))
	{
		GST_DEBUG ("unrefing component");
		G_OBJECT(me->component)->ref_count = 1;
		g_object_unref (me->component);
	}

	if (G_LIKELY (me->factory))
	{
		GST_DEBUG ("unrefing factory");
		g_object_unref (me->factory);
	}

	return;
}

static void
gst_goo_decjpeg_finalize (GObject *object)
{
	GstGooDecJpegPrivate* priv = GST_GOO_DECJPEG_GET_PRIVATE (object);

	if (G_LIKELY (priv->tempbuf))
	{
		gst_buffer_unref (priv->tempbuf);
	}

	if (G_LIKELY (priv->segment))
	{
		gst_segment_free (priv->segment);
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);

	return;
}

static void
gst_goo_decjpeg_base_init (gpointer g_klass)
{
	GST_DEBUG_CATEGORY_INIT (gst_goo_decjpeg_debug, "goodecjpeg", 0,
				 "OpenMAX JPEG encoder element");

	GstElementClass* e_klass = GST_ELEMENT_CLASS (g_klass);

	gst_element_class_add_pad_template (e_klass,
					    gst_static_pad_template_get
					    (&sink_factory));

	gst_element_class_add_pad_template (e_klass,
					    gst_static_pad_template_get
					    (&src_factory));

	gst_element_class_set_details (e_klass, &details);

	return;
}


static void
gst_goo_decjpeg_class_init (GstGooDecJpegClass* klass)
{
	GObjectClass* g_klass;
	GParamSpec* spec;
	GstElementClass* gst_klass;

	g_klass = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GstGooDecJpegPrivate));

	g_klass->set_property =
		GST_DEBUG_FUNCPTR (gst_goo_decjpeg_set_property);
	g_klass->get_property =
		GST_DEBUG_FUNCPTR (gst_goo_decjpeg_get_property);
	g_klass->dispose = GST_DEBUG_FUNCPTR (gst_goo_decjpeg_dispose);
	g_klass->finalize = GST_DEBUG_FUNCPTR (gst_goo_decjpeg_finalize);


	spec = g_param_spec_enum ("resize", "Resize",
				  "Scale the output frame",
				  GST_GOO_DECJPEG_SCALE,
				  SCALE_DEFAULT, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_RESIZE, spec);
	/* Section decoding */
	spec = g_param_spec_uint ("slicing", "Slicing",
				  "Section decoding",
				  0,32,
				  SLICING_DEFAULT, G_PARAM_READWRITE);
	//	GST_DEBUG ("Slicing spec");
	g_object_class_install_property (g_klass, PROP_SLICING, spec);

	/* SubRegion decoding */

	spec = g_param_spec_uint ("subregion-left", "Left subregion decoding",
				 "The number of pixels to start subregion decoding from left",
				 0, G_MAXUINT, 0, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_SUBREGIONLEFT, spec);

	spec = g_param_spec_uint ("subregion-top", "Top subregion decoding",
				 "The number of pixels to start subregion decoding from top",
				 0, G_MAXUINT, 0, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_SUBREGIONTOP, spec);

	spec = g_param_spec_uint ("subregion-width", "Subregion width",
				  "The width of the subregion decoding",
				  0, G_MAXUINT, 0, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_SUBREGIONWIDTH, spec);

	spec = g_param_spec_uint ("subregion-height", "Subregion height",
				  "The height of the subregion decoding",
				  0, G_MAXUINT, 0, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_SUBREGIONHEIGHT, spec);


	/* GST */
	gst_klass = GST_ELEMENT_CLASS (klass);
	gst_klass->change_state =
		GST_DEBUG_FUNCPTR (gst_goo_decjpeg_change_state);


	return;
}

/**
 * gst_goo_video_filter_outport_buffer:
 * @port: A #GooPort instance
 * @buffer: An #OMX_BUFFERHEADERTYPE pointer
 * @data: A pointer to extra data
 *
 * This function is a generic callback for a libgoo's output port and push
 * a new GStreamer's buffer.
 *
 * This method can be reused in derived classes.
 **/
void
gst_goo_dec_jpeg_outport_buffer (GooPort* port, OMX_BUFFERHEADERTYPE* buffer,
				  gpointer data)
{
	g_return_if_fail (buffer->nFlags != OMX_BUFFERFLAG_DATACORRUPT);

	GST_DEBUG ("Enter");


	g_assert (GOO_IS_PORT (port));
	g_assert (buffer != NULL);
	g_assert (GOO_IS_COMPONENT (data));

	GooComponent* component = GOO_COMPONENT (data);
	GstGooDecJpeg* self =
		GST_GOO_DECJPEG (g_object_get_data (G_OBJECT (data), "gst"));
	g_assert (self != NULL);

	GstGooDecJpegPrivate* priv = GST_GOO_DECJPEG_GET_PRIVATE (self);


	process_output_buffer (self, buffer);

	if (buffer->nFlags & OMX_BUFFERFLAG_EOS == 0x1 || goo_port_is_eos (port))
	{



		if (priv->buffer_slice != NULL){

		GST_BUFFER_OFFSET (priv->buffer_slice) = priv->outcount++;
		GST_BUFFER_TIMESTAMP (priv->buffer_slice) = priv->next_ts;

		if (priv->packetized &&
		    GST_CLOCK_TIME_IS_VALID (priv->next_ts))
			{
				if (GST_CLOCK_TIME_IS_VALID (priv->duration))
					{
						// use duration from incoming buffer for outgoing buffer
						priv->next_ts += priv->duration;
					}
				else if (priv->framerate_numerator != 0)
					{
						priv->duration = gst_util_uint64_scale
							(GST_SECOND,
							 priv->framerate_denominator,
							 priv->framerate_numerator);
						priv->next_ts += priv->duration;
					}
				else
					{
						priv->duration = GST_CLOCK_TIME_NONE;
						priv->next_ts = GST_CLOCK_TIME_NONE;
					}
			}
		else
			{
				priv->duration = GST_CLOCK_TIME_NONE;
				priv->next_ts = GST_CLOCK_TIME_NONE;
			}
		GST_BUFFER_DURATION (priv->buffer_slice) = priv->duration;

		gst_buffer_set_caps (priv->buffer_slice, GST_PAD_CAPS (self->srcpad));

		gst_pad_push (self->srcpad, priv->buffer_slice);

	}



		GST_INFO ("EOS flag found in output buffer (%d)",
			  buffer->nFilledLen);


		goo_component_set_done (self->component);

	}
	GST_INFO_OBJECT (self, "");
	return;
}



static void
gst_goo_decjpeg_init (GstGooDecJpeg* self, GstGooDecJpegClass* klass)
{
	GST_DEBUG_OBJECT (self, "");

	GstGooDecJpegPrivate* priv = GST_GOO_DECJPEG_GET_PRIVATE (self);

	priv->incount = 0;
	priv->outcount = 0;
	priv->scalefactor = SCALE_DEFAULT;
	priv->caps_width = priv->width = -1;
	priv->caps_height = priv->height = -1;
	priv->imagesiz = 0;
	priv->in_color = -1;
	priv->out_color = OMX_COLOR_FormatCbYCrY;
	priv->tempbuf = NULL;
	priv->segment = gst_segment_new ();
	priv->caps_framerate_numerator = priv->framerate_numerator = 0;
	priv->caps_framerate_denominator = priv->framerate_denominator = 1;
	priv->packetized = FALSE;
	priv->next_ts = GST_CLOCK_TIME_NONE;
	priv->duration = GST_CLOCK_TIME_NONE;
	priv->num_slices = SLICING_DEFAULT;
	/* Subregion decoding */
	priv->subregion_left = 0;
	priv->subregion_top = 0;
	priv->subregion_width = 0;
	priv->subregion_height = 0;
	priv->size = 0;

	priv->factor_color = 2.0;
	priv->count =0;

	priv->buffer_slice = NULL;

	self->factory = goo_ti_component_factory_get_instance();
	self->component = goo_component_factory_get_component
		(self->factory, GOO_TI_JPEG_DECODER);

	/* inport */
	{
		self->inport = goo_component_get_port (self->component,
						       "input0");
		g_assert (self->inport != NULL);
	}

	/* outport */
	{
		self->outport = goo_component_get_port (self->component,
							"output0");
		g_assert (self->outport != NULL);

		GooPort* port = GST_GOO_DECJPEG (self)->outport;

		/** Use the callback function **/
		goo_port_set_process_buffer_function(port, gst_goo_dec_jpeg_outport_buffer);
	}



	/* GST */
	GstPadTemplate* pad_template;

	pad_template = gst_element_class_get_pad_template
		(GST_ELEMENT_CLASS (klass), "sink");
	g_return_if_fail (pad_template != NULL);
	self->sinkpad = gst_pad_new_from_template (pad_template, "sink");
	gst_pad_set_chain_function
		(self->sinkpad, GST_DEBUG_FUNCPTR (gst_goo_decjpeg_chain));
	gst_pad_set_setcaps_function
		(self->sinkpad,
		 GST_DEBUG_FUNCPTR (gst_goo_decjpeg_sink_setcaps));
	gst_pad_set_event_function
		(self->sinkpad, GST_DEBUG_FUNCPTR (gst_goo_decjpeg_event));
	gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);

	pad_template = gst_element_class_get_pad_template
		(GST_ELEMENT_CLASS (klass), "src");
	g_return_if_fail (pad_template != NULL);
	self->srcpad = gst_pad_new_from_template (pad_template, "src");
	gst_pad_set_setcaps_function
		(self->srcpad,
		 GST_DEBUG_FUNCPTR (gst_goo_decjpeg_src_setcaps));
	gst_element_add_pad (GST_ELEMENT (self), self->srcpad);

	g_object_set_data (G_OBJECT (self->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", self->component);

	return;
}
