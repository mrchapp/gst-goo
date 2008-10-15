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

#include "gstgoodecmpeg4.h"

GST_BOILERPLATE (GstGooDecMpeg4, gst_goo_decmpeg4, GstGooVideoDec, GST_TYPE_GOO_VIDEODEC);

GST_DEBUG_CATEGORY_STATIC (gst_goo_decmpeg4_debug);
#define GST_CAT_DEFAULT gst_goo_decmpeg4_debug

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

static const GstElementDetails details =
	GST_ELEMENT_DETAILS (
						"OpenMAX MPEG4 decoder",
						"Codedc/Decoder/Video",
						"Decodes MPEG4 streams with OpenMAX",
						"Texas Instrument"
						);

/* FIXME: make three caps, for mpegversion 1, 2 and 2.5 */
static GstStaticPadTemplate sink_factory =
	GST_STATIC_PAD_TEMPLATE (
    						"sink",
							GST_PAD_SINK,
							GST_PAD_ALWAYS,
							GST_STATIC_CAPS ("video/mpeg, "
											"mpegversion = (int) 4, "
											"systemstream = (boolean) FALSE,"
											"width = (int) [16, 4096], "
											"height = (int) [16, 4096], "
											"framerate = (GstFraction) [0/1, 60/1]"));


static void
gst_goo_decmpeg4_base_init (gpointer g_klass)
{
	GST_DEBUG_CATEGORY_INIT (gst_goo_decmpeg4_debug, "goodecmpeg4", 0,
							"OpenMAX MPEG4 decoder element");

	GstElementClass* e_klass = GST_ELEMENT_CLASS (g_klass);

	gst_element_class_add_pad_template (e_klass,
		gst_static_pad_template_get (&sink_factory));

	gst_element_class_set_details (e_klass, &details);

	return;
}

static GstBuffer*
gst_goo_decmpeg4_codec_data_processing (GstGooVideoFilter *filter, GstBuffer *buffer)
{

	GstGooDecMpeg4 *self = GST_GOO_DECMPEG4 (filter);

	if (GST_IS_BUFFER (GST_GOO_VIDEODEC(self)->video_header))
	{
		GST_DEBUG_OBJECT (self, "Adding MPEG-4 header info to buffer");

		buffer = gst_buffer_merge (GST_BUFFER (GST_GOO_VIDEODEC(self)->video_header), GST_BUFFER (buffer));
		gst_buffer_unref (GST_GOO_VIDEODEC(self)->video_header);

	}

	return buffer;

}

static void
gst_goo_decmpeg4_class_init (GstGooDecMpeg4Class* klass)
{
	GObjectClass* g_klass;
	GParamSpec* pspec;
	GstElementClass* gst_klass;

	/* GST GOO VIDEO_FILTER */
	GstGooVideoFilterClass* gst_c_klass = GST_GOO_VIDEO_FILTER_CLASS (klass);
	gst_c_klass->codec_data_processing_func = GST_DEBUG_FUNCPTR (gst_goo_decmpeg4_codec_data_processing);

	return;
}

static void
gst_goo_decmpeg4_init (GstGooDecMpeg4* self, GstGooDecMpeg4Class* klass)
{
	GST_DEBUG ("");

	GST_GOO_VIDEO_FILTER (self)->component = goo_component_factory_get_component
		(GST_GOO_VIDEO_FILTER (self)->factory, GOO_TI_MPEG4_DECODER);

	GooComponent* component = GST_GOO_VIDEO_FILTER (self)->component;

	/* Select Stream mode operation as default */
	g_object_set (G_OBJECT (component), "process-mode", GOO_TI_VIDEO_DECODER_FRAMEMODE, NULL);

	/* input port */
	GST_GOO_VIDEO_FILTER (self)->inport = goo_component_get_port (component, "input0");
	g_assert (GST_GOO_VIDEO_FILTER (self)->inport != NULL);

	GST_GOO_VIDEO_FILTER (self)->outport = goo_component_get_port (component, "output0");
	g_assert (GST_GOO_VIDEO_FILTER (self)->outport != NULL);

	/** Use the PARENT's callback function **/
	goo_port_set_process_buffer_function
		(GST_GOO_VIDEO_FILTER (self)->outport, gst_goo_video_filter_outport_buffer);

	g_object_set_data (G_OBJECT (GST_GOO_VIDEO_FILTER (self)->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", GST_GOO_VIDEO_FILTER (self)->component);

        return;
}
