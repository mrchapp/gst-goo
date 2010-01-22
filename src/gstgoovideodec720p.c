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

#include "gstgoovideodec720p.h"

GST_BOILERPLATE (GstGooVideoDec720p, gst_goo_videodec720p, GstGooVideoFilter, GST_TYPE_GOO_VIDEO_FILTER);

GST_DEBUG_CATEGORY_STATIC (gst_goo_videodec720p_debug);
#define GST_CAT_DEFAULT gst_goo_videodec720p_debug

static GstCaps* gst_goo_videodec720p_transform_caps (GstGooVideoFilter* filter,
	GstPadDirection direction, GstCaps* caps);
static gboolean gst_goo_videodec720p_configure_caps (GstGooVideoFilter* filter,
	GstCaps* in, GstCaps* out);

/* signals */
enum
{
	LAST_SIGNAL
};

/* args */
enum
{
	PROP_0
};

#define DEFAULT_WIDTH 352
#define DEFAULT_HEIGHT 288
#define DEFAULT_FRAMERATE 15
#define DEFAULT_COLOR_FORMAT OMX_COLOR_FormatCbYCrY

/* default values */

static GstStaticPadTemplate src_factory =
	GST_STATIC_PAD_TEMPLATE ("src",
		GST_PAD_SRC,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS ("video/x-raw-yuv, "
//			"format = (fourcc) { UYVY, I420 }, "
			"format = (fourcc) { UYVY }, "  // Note: I420 seems to produce bad output, so I'm removing it for now so we don't accidentially negotiate that format --Rob
			"width = (int) [16, 4096], "
			"height = (int) [16, 4096], "
			"framerate = (GstFraction) [1/1, 120/1]"));


static void
gst_goo_videodec720p_base_init (gpointer g_klass)
{
	GST_DEBUG_CATEGORY_INIT (gst_goo_videodec720p_debug, "goovideodec720p", 0,
		"Gst OpenMax parent class for video decoders");

	GstElementClass* e_klass = GST_ELEMENT_CLASS (g_klass);

	gst_element_class_add_pad_template (e_klass,
		gst_static_pad_template_get (&src_factory));

	return;
}

static gboolean
gst_goo_videodec720p_set_process_mode (GstGooVideoFilter* filter, guint value)
{

	GstGooVideoDec720p* self = GST_GOO_VIDEODEC720P (filter);
	GooComponent* component = GST_GOO_VIDEO_FILTER (self)->component;

	GST_DEBUG ("Setting process mode to %d", value);

	g_object_set (G_OBJECT (component), "process-mode", value, NULL);

	return TRUE;
}

static void
gst_goo_videodec720p_class_init (GstGooVideoDec720pClass* klass)
{
 	GObjectClass* g_klass;
	GParamSpec* pspec;
	GstElementClass* gst_klass;

	/* GST GOO VIDEO_FILTER */
	GstGooVideoFilterClass* gst_c_klass = GST_GOO_VIDEO_FILTER_CLASS (klass);
	gst_c_klass->set_process_mode_func = GST_DEBUG_FUNCPTR (gst_goo_videodec720p_set_process_mode);
	gst_c_klass->transform_caps = GST_DEBUG_FUNCPTR (gst_goo_videodec720p_transform_caps);
	gst_c_klass->set_caps = GST_DEBUG_FUNCPTR (gst_goo_videodec720p_configure_caps);

	return;
}

static void
gst_goo_videodec720p_init (GstGooVideoDec720p* self, GstGooVideoDec720pClass* klass)
{
	return;
}

static void
parse_out_caps (GstCaps* caps, gint* format, gint* width, gint* height)
{
	GstStructure *structure;
	guint32 fourcc;

	structure = gst_caps_get_structure (caps, 0);
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

}

static void
parse_in_caps (GstCaps* caps, GstGooVideoDec720p* self)
{
	GstStructure *structure;
	guint32 fourcc;

	structure = gst_caps_get_structure (caps, 0);

	/* Get Header info */
	{
		const GValue *value;

		if ((value = gst_structure_get_value (structure, "codec_data")))
		{
			self->video_header = gst_value_get_buffer (value);
		}

	}

}

static GstCaps*
gst_goo_videodec720p_transform_caps (GstGooVideoFilter* filter,
				 GstPadDirection direction, GstCaps* caps)
{
	GstGooVideoDec720p* self = GST_GOO_VIDEODEC720P(filter);
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
	GST_DEBUG_OBJECT (self, "transformed caps %s", strcaps);			\
	g_free (strcaps);

	return result;

}


static gboolean
omx_sync (GstGooVideoDec720p* self, guint color_format, guint width, guint height)
{
	GST_DEBUG_OBJECT (self, "Synchronizing values to omx ports: color_format=%d, width=%d, height=%d", color_format, width, height);

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
	}

	/* outport */
	{
		OMX_PARAM_PORTDEFINITIONTYPE* param =
			GOO_PORT_GET_DEFINITION (GST_GOO_VIDEO_FILTER(self)->outport);

		param->format.video.nFrameWidth = width;
		param->format.video.nFrameHeight = height;
		param->format.video.eColorFormat = color_format;
	}

	return TRUE;
}

static gboolean
gst_goo_videodec720p_configure_caps (GstGooVideoFilter* filter,
				  GstCaps* in, GstCaps* out)
{
	GstGooVideoDec720p* self = GST_GOO_VIDEODEC720P (filter);

	gboolean ret = FALSE;
	guint color_format;
	guint width;
	guint height;


	GST_DEBUG_OBJECT (self, "Configuring caps");

	g_return_val_if_fail (GST_CAPS_IS_SIMPLE (in), FALSE);
	g_return_val_if_fail (GST_CAPS_IS_SIMPLE (out), FALSE);

	/* input capabilities */
	parse_in_caps (in, self);

	/* output capabilities */
GST_DEBUG ("out caps %" GST_PTR_FORMAT, out);
	parse_out_caps (out, &color_format, &width, &height);
	filter->src_caps = gst_caps_ref (out);

	if (goo_component_get_state (GST_GOO_VIDEO_FILTER(self)->component) == OMX_StateLoaded)
	{
		GST_OBJECT_LOCK (self);
		ret = omx_sync (self, color_format, width, height);
		GST_OBJECT_UNLOCK (self);
	}

	return ret;

}
