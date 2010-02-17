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

#include "gstgoovideoenc.h"
#include "gstgooutils.h"

GST_BOILERPLATE (GstGooVideoEnc, gst_goo_videoenc, GstGooVideoFilter, GST_TYPE_GOO_VIDEO_FILTER);

GST_DEBUG_CATEGORY_STATIC (gst_goo_videoenc_debug);
#define GST_CAT_DEFAULT gst_goo_videoenc_debug

static GstCaps* gst_goo_videoenc_transform_caps (GstGooVideoFilter* filter,
	GstPadDirection direction, GstCaps* caps);
static gboolean gst_goo_videoenc_configure_caps (GstGooVideoFilter* filter,
	GstCaps* in, GstCaps* out);
static GstBuffer* gst_goo_videoenc_extra_buffer_processing (GstGooVideoFilter* filter,
	GstBuffer *buffer);


/* signals */
enum
{
	LAST_SIGNAL
};

/* args */
enum
{
	PROP_0,
	PROP_CONTROLRATE,
	PROP_BITRATE
};


/* default values */
#define DEFAULT_WIDTH 176
#define DEFAULT_HEIGHT 144
#define DEFAULT_COLOR_FORMAT OMX_COLOR_FormatYUV420PackedPlanar
#define DEFAULT_FRAMERATE 15
#define DEFAULT_FRAMEINTERVAL 30
#define DEFAULT_BITRATE 368000
#define DEFAULT_CONTROLRATE GOO_TI_VIDEO_ENCODER_CR_VARIABLE

static GstStaticPadTemplate sink_factory =
        GST_STATIC_PAD_TEMPLATE (
                "sink",
                GST_PAD_SINK,
                GST_PAD_ALWAYS,
		GST_STATIC_CAPS ("video/x-raw-yuv, "
				"format = (fourcc) { YUY2, I420, UYVY }, "
				"width = (int) [16, 4096], "
				"height = (int) [16, 4096], "
				"framerate = (GstFraction) [1/1, 120/1]"));


static void
gst_goo_videoenc_base_init (gpointer g_klass)
{
	GST_DEBUG_CATEGORY_INIT (gst_goo_videoenc_debug, "goovideoenc", 0,
		"Gst OpenMax parent class for video encoders");

	GstElementClass* e_klass = GST_ELEMENT_CLASS (g_klass);

	gst_element_class_add_pad_template (e_klass,
										gst_static_pad_template_get
										(&sink_factory));

	return;
}

static void
gst_goo_videoenc_set_property (GObject* object, guint prop_id,
				const GValue* value, GParamSpec* pspec)
{
	GstGooVideoEnc* self = GST_GOO_VIDEOENC (object);
	GooComponent* component = GST_GOO_VIDEO_FILTER (self)->component;

	switch (prop_id)
	{
	case PROP_CONTROLRATE:
		g_object_set (G_OBJECT (component), "control-rate", g_value_get_enum (value), NULL);
		break;
	case PROP_BITRATE:
	{
		GooPort* port = GST_GOO_VIDEO_FILTER (self)->outport;
		GOO_PORT_GET_DEFINITION (port)->format.video.nBitrate = g_value_get_ulong (value);
		break;
	}
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_videoenc_get_property (GObject* object, guint prop_id,
				GValue* value, GParamSpec* pspec)
{
	GstGooVideoEnc* self = GST_GOO_VIDEOENC (object);
	GooComponent* component = GST_GOO_VIDEO_FILTER (self)->component;

	switch (prop_id)
	{
	case PROP_CONTROLRATE:
	{
		gint controlrate;
		g_object_get (G_OBJECT (component), "control-rate", &controlrate, NULL);
		g_value_set_enum (value, controlrate);
		break;
	}
	case PROP_BITRATE:
	{
		GooPort* port = GST_GOO_VIDEO_FILTER (self)->outport;
		g_value_set_ulong (value, GOO_PORT_GET_DEFINITION (port)->format.video.nBitrate);
		break;
	}
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_videoenc_class_init (GstGooVideoEncClass* klass)
{
 	GObjectClass* g_klass;
	GParamSpec* pspec;
	GstElementClass* gst_klass;

	/* gobject */
	g_klass = G_OBJECT_CLASS (klass);

	g_klass->set_property =
		GST_DEBUG_FUNCPTR (gst_goo_videoenc_set_property);
	g_klass->get_property =
		GST_DEBUG_FUNCPTR (gst_goo_videoenc_get_property);

	GParamSpec* spec;

	spec = g_param_spec_enum ("control-rate", "Control Rate",
		"Specifies the stream encoding rate control",
		GOO_TI_VIDEO_ENCODER_CONTROL_RATE,
		DEFAULT_CONTROLRATE, G_PARAM_READWRITE);

	g_object_class_install_property (g_klass, PROP_CONTROLRATE, spec);

	g_object_class_install_property (g_klass, PROP_BITRATE,
		g_param_spec_ulong ("bitrate", "Bitrate", "The Bitrate",
		0, G_MAXULONG, DEFAULT_BITRATE, G_PARAM_READWRITE));

	/* GST GOO VIDEO_FILTER */
	GstGooVideoFilterClass* gst_c_klass = GST_GOO_VIDEO_FILTER_CLASS (klass);
	gst_c_klass->transform_caps = GST_DEBUG_FUNCPTR (gst_goo_videoenc_transform_caps);
	gst_c_klass->set_caps = GST_DEBUG_FUNCPTR (gst_goo_videoenc_configure_caps);
	gst_c_klass->extra_buffer_processing_func = GST_DEBUG_FUNCPTR (gst_goo_videoenc_extra_buffer_processing);

	return;
}

static void
gst_goo_videoenc_init (GstGooVideoEnc* self, GstGooVideoEncClass* klass)
{

	return;
}

static GstBuffer*
gst_goo_videoenc_extra_buffer_processing (GstGooVideoFilter* filter, GstBuffer *buffer)
{
	GstGooVideoEnc *self = GST_GOO_VIDEOENC (filter);
        GooComponent* component = GST_GOO_VIDEO_FILTER (self)->component;

	GST_DEBUG_OBJECT (self, "Entering");

	static gboolean frame_interval_flag = FALSE;

	if (!frame_interval_flag)
	{
		GST_DEBUG_OBJECT (self, "Setting frame interval");
		gst_goo_util_ensure_executing (component);
		g_object_set (G_OBJECT (component), "frame-interval", DEFAULT_FRAMEINTERVAL, NULL);
		frame_interval_flag = TRUE;

	}

	GST_DEBUG_OBJECT (self, "Exit");

	return buffer;
}


static void
parse_caps (GstCaps* in, gint* format, gint* width, gint* height, gint* framerate_int)
{
	GstStructure *structure;
	guint32 fourcc;

	structure = gst_caps_get_structure (in, 0);
	gst_structure_get_int (structure, "width", width);
	gst_structure_get_int (structure, "height", height);

	if (g_strrstr (gst_structure_get_name (structure), "video/x-raw-yuv"))
	{
		if (gst_structure_get_fourcc (structure, "format", &fourcc))
		{
			switch (fourcc)
			{
			case GST_MAKE_FOURCC ('Y', 'U', 'Y', '2'):
				*format = OMX_COLOR_FormatYCbYCr;
				break;
			case GST_MAKE_FOURCC ('I', '4', '2', '0'):
				*format = OMX_COLOR_FormatYUV420PackedPlanar;
				break;
			case GST_MAKE_FOURCC ('U', 'Y', 'V', 'Y'):
				*format = OMX_COLOR_FormatCbYCrY;
				break;
			default:
				break;
			}
		}
	}

	{
		const GValue *framerate;
		*framerate_int = DEFAULT_FRAMERATE;

		framerate = gst_structure_get_value (structure, "framerate");

		if (framerate != NULL)
		{
			*framerate_int = gst_value_get_fraction_numerator (framerate) / gst_value_get_fraction_denominator (framerate);
		}

	}

}

static GstCaps*
gst_goo_videoenc_transform_caps (GstGooVideoFilter* filter,
				 GstPadDirection direction, GstCaps* caps)
{
	GstGooVideoEnc* self = GST_GOO_VIDEOENC(filter);
	const GstCaps* template = NULL;
	GstCaps* result = NULL;
	GstStructure *ins, *outs;

	GST_DEBUG_OBJECT (self, "");

	/* this function is always called with a simple caps */
	g_return_val_if_fail (GST_CAPS_IS_SIMPLE (caps), NULL);

	if (direction == GST_PAD_SINK)
	{
		template = gst_pad_get_pad_template_caps (filter->srcpad);
		result = gst_caps_copy (template);
	}
	else if (direction == GST_PAD_SRC)
	{
		template = gst_pad_get_pad_template_caps (filter->sinkpad);
		result = gst_caps_copy (template);
	}

	/* we force the same width, height and framerate */
	ins = gst_caps_get_structure (caps, 0);
	outs = gst_caps_get_structure (result, 0);

	gint num, den;
	gst_structure_get_fraction (ins, "framerate", &num, &den);

	gst_structure_set (outs, "framerate",
		GST_TYPE_FRACTION, num, den, NULL);

	gint width, height;
	gst_structure_get_int (ins, "width", &width);
	gst_structure_get_int (ins, "height", &height);

	gst_structure_set (outs, "width",
		G_TYPE_INT, width, NULL);
	gst_structure_set (outs, "height",
		G_TYPE_INT, height, NULL);

	gchar* strcaps = gst_caps_to_string (result);
	GST_DEBUG_OBJECT (self, "transformed caps %s", strcaps);
	g_free (strcaps);

	return result;

}


static gboolean
omx_sync (GstGooVideoEnc* self, guint color_format, guint width, guint height, guint framerate)
{
	GST_DEBUG_OBJECT (self, "Synchronizing values to omx ports");

	g_assert (GST_GOO_VIDEO_FILTER(self)->component != NULL);
	g_assert (GST_GOO_VIDEO_FILTER(self)->inport != NULL);
	g_assert (GST_GOO_VIDEO_FILTER(self)->outport != NULL);

	/* inport */
	{
		OMX_PARAM_PORTDEFINITIONTYPE* param =
			GOO_PORT_GET_DEFINITION (GST_GOO_VIDEO_FILTER(self)->inport);

		param->format.video.nFrameWidth = width;
		param->format.video.nFrameHeight = height;
		param->format.video.eColorFormat = color_format;
		param->format.video.xFramerate = framerate << 16;
		GST_DEBUG_OBJECT (self, "framerate = %d  xFramerate  = %d", framerate, param->format.video.xFramerate);
	}

	/* outport */
	{
		OMX_PARAM_PORTDEFINITIONTYPE* param =
			GOO_PORT_GET_DEFINITION (GST_GOO_VIDEO_FILTER(self)->outport);

		param->format.video.nFrameWidth = width;
		param->format.video.nFrameHeight = height;
		param->format.video.xFramerate = framerate << 16;
		GST_DEBUG_OBJECT (self, "framerate = %d; xFramerate = %d", framerate, param->format.video.xFramerate);
	}

	return TRUE;
}

static gboolean
gst_goo_videoenc_configure_caps (GstGooVideoFilter* filter,
				  GstCaps* in, GstCaps* out)
{
	GstGooVideoEnc* self = GST_GOO_VIDEOENC (filter);

	gboolean ret = FALSE;
	guint color_format;
	guint width;
	guint height;
	guint framerate;

	GST_DEBUG_OBJECT (self, "Configuring caps");

	g_return_val_if_fail (GST_CAPS_IS_SIMPLE (in), FALSE);
	g_return_val_if_fail (GST_CAPS_IS_SIMPLE (out), FALSE);

	/* Parsing input capabilities */
	parse_caps (in, &color_format, &width, &height, &framerate);

	if (goo_component_get_state (GST_GOO_VIDEO_FILTER(self)->component) == OMX_StateLoaded)
	{
		GST_OBJECT_LOCK (self);
		ret = omx_sync (self, color_format, width, height, framerate);
		GST_OBJECT_UNLOCK (self);
	}
	else if (goo_port_is_tunneled (GST_GOO_VIDEO_FILTER(self)->inport))
	{
		ret=TRUE;
	}
	return ret;

}
