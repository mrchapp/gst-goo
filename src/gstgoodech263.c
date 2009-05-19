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

#include "gstgoodech263.h"

GST_BOILERPLATE (GstGooDecH263, gst_goo_dech263, GstGooVideoDec, GST_TYPE_GOO_VIDEODEC);

GST_DEBUG_CATEGORY_STATIC (gst_goo_dech263_debug);
#define GST_CAT_DEFAULT gst_goo_dech263_debug

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

#define GST_GOO_DECH263_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_GOO_DECH263, GstGooDecH263Private))

static const GstElementDetails details =
	GST_ELEMENT_DETAILS (
		"OpenMAX H263 decoder",
		"Codedc/Decoder/Video",
		"Decodes H263 streams with OpenMAX",
		"Texas Instrument"
		);

static GstStaticPadTemplate sink_factory =
	GST_STATIC_PAD_TEMPLATE ("sink",
		GST_PAD_SINK,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS ("video/x-h263,"
			"width = (int) [16, 4096], "
			"height = (int) [16, 4096], "
			"framerate = (GstFraction) [1/1, 120/1]"));


static void
gst_goo_dech263_base_init (gpointer g_klass)
{
	GST_DEBUG_CATEGORY_INIT (gst_goo_dech263_debug, "goodech263", 0,
		"OpenMAX H263 decoder element");

	GstElementClass* e_klass = GST_ELEMENT_CLASS (g_klass);

	gst_element_class_add_pad_template (e_klass,
		gst_static_pad_template_get (&sink_factory));

	gst_element_class_set_details (e_klass, &details);

	return;
}

static GstBuffer*
gst_goo_dech263_codec_data_processing (GstGooVideoFilter *filter, GstBuffer *buffer)
{

	GstGooDecH263 *self = GST_GOO_DECH263 (filter);
	GooPort* inport;

	inport = (GooPort *) GST_GOO_VIDEO_FILTER (self)->inport;

	GST_DEBUG_OBJECT (self, "Getting width and height from the first buffer");

	/* Get width and height from the code_data extra info */
	{
		guint width, height, format;

		/* The format is obtain from the 3 bits starting in offset 35 of the video header */
		format = (GST_BUFFER_DATA (buffer)[4] & 0x1C) >> 2;

		GST_DEBUG_OBJECT (self, "Codec data: format = %d", format);

		if (format != 7 && format != 6)
		{
			/* Used to multiplex from the format in the codec_data extra information */
			int h263_format[8][2] = {
				{ 0, 0 },
				{ 128, 96 },
				{ 176, 144 },
				{ 352, 288 },
				{ 704, 576 },
				{ 1408, 1152 },
			};

			width = (guint) h263_format[format][0];
			height = (guint) h263_format[format][1];

			GST_DEBUG_OBJECT (self, "Codec data: width = %d, height = %d", width, height);

			GOO_PORT_GET_DEFINITION (inport)->format.video.nFrameWidth = width;
			GOO_PORT_GET_DEFINITION (inport)->format.video.nFrameHeight = height;

		}

	}

	return buffer;

}

static void
gst_goo_dech263_class_init (GstGooDecH263Class* klass)
{
 	GObjectClass* g_klass;
	GParamSpec* pspec;
	GstElementClass* gst_klass;

	/* GST GOO VIDEO FILTER*/
	GstGooVideoFilterClass* gst_c_klass = GST_GOO_VIDEO_FILTER_CLASS (klass);
	gst_c_klass->codec_data_processing_func = GST_DEBUG_FUNCPTR (gst_goo_dech263_codec_data_processing);

	return;
}

static void
gst_goo_dech263_init (GstGooDecH263* self, GstGooDecH263Class* klass)
{
	GST_DEBUG ("");

	GST_GOO_VIDEO_FILTER (self)->component = goo_component_factory_get_component
	(GST_GOO_VIDEO_FILTER (self)->factory, GOO_TI_H263_DECODER);

	GooComponent* component = GST_GOO_VIDEO_FILTER (self)->component;

	/* Select Stream mode operation as default */
	g_object_set (G_OBJECT (component), "process-mode", GOO_TI_VIDEO_DECODER_FRAMEMODE, NULL);

	GST_GOO_VIDEO_FILTER (self)->inport = goo_component_get_port (component, "input0");
	g_assert (GST_GOO_VIDEO_FILTER (self)->inport != NULL);

	GST_GOO_VIDEO_FILTER (self)->outport = goo_component_get_port (component, "output0");
	g_assert (GST_GOO_VIDEO_FILTER (self)->outport != NULL);

	GooPort* port = GST_GOO_VIDEO_FILTER (self)->outport;

	/** Use the PARENT's callback function **/
	goo_port_set_process_buffer_function
		(port, gst_goo_video_filter_outport_buffer);

	g_object_set_data (G_OBJECT (GST_GOO_VIDEO_FILTER (self)->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", GST_GOO_VIDEO_FILTER (self)->component);

        return;
}
