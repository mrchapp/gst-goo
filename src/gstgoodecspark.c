/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* =====================================================================
 *                Texas Instruments OMAP(TM) Platform Software
 *             Copyright (c) 2005 Texas Instruments, Incorporated
 *
 * Use of this software is controlled by the terms and conditions found
 * in the license agreement under which this software has been supplied.
 * ===================================================================== */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <goo-ti-sparkdec.h>
#include "gstgoodecspark.h"

GST_BOILERPLATE (GstGooDecSpark, gst_goo_decspark, GstGooVideoDec, GST_TYPE_GOO_VIDEODEC);

GST_DEBUG_CATEGORY_STATIC (gst_goo_decspark_debug);
#define GST_CAT_DEFAULT gst_goo_decspark_debug

#define DEFAULT_WIDTH 176
#define DEFAULT_HEIGHT 144
#define DEFAULT_FRAMERATE 25
#define DEFAULT_COLOR_FORMAT OMX_VIDEO_CodingUnused

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
						"OpenMAX SPARK decoder",
						"Codedc/Decoder/Video",
						"Decodes SPARK streams with OpenMAX",
						"Texas Instrument"
						);

static GstStaticPadTemplate sink_factory =
	GST_STATIC_PAD_TEMPLATE (
    						"sink",
							GST_PAD_SINK,
							GST_PAD_ALWAYS,
							GST_STATIC_CAPS("video/x-svq, "
											"width = (int) [16, 4096], "
											"height = (int) [16, 4096], "
											"framerate = (GstFraction) [0/1, 60/1];"
											"video/x-gst-fourcc-FLV1, "
											"width = (int) [16, 4096], "
											"height = (int) [16, 4096], "
											"framerate = (GstFraction) [0/1, 60/1]"));


static void
gst_goo_decspark_base_init (gpointer g_klass)
{
	GST_DEBUG_CATEGORY_INIT (gst_goo_decspark_debug, "goodecspark", 0,
							"OpenMAX SPARK decoder element");

	GstElementClass* e_klass = GST_ELEMENT_CLASS (g_klass);

	gst_element_class_add_pad_template (e_klass,
		gst_static_pad_template_get (&sink_factory));

	gst_element_class_set_details (e_klass, &details);

	return;
}

static GstBuffer*
gst_goo_decspark_codec_data_processing (GstGooVideoFilter *filter, GstBuffer *buffer)
{

	GstGooDecSpark *self = GST_GOO_DECSPARK (filter);

	if (GST_IS_BUFFER (GST_GOO_VIDEODEC(self)->video_header))
	{
		GST_DEBUG_OBJECT (self, "Adding SPARK header info to buffer");

		GstBuffer *new_buf = gst_buffer_merge (GST_BUFFER (GST_GOO_VIDEODEC(self)->video_header), GST_BUFFER (buffer));

		/* gst_buffer_merge() will end up putting video_header's timestamp on
		 * the new buffer, but actually we want buf's timestamp:
		 */
		GST_BUFFER_TIMESTAMP (new_buf) = GST_BUFFER_TIMESTAMP (buffer);
		buffer = new_buf;

		gst_buffer_unref (GST_GOO_VIDEODEC(self)->video_header);
	}

	return buffer;

}

static void
gst_goo_decspark_class_init (GstGooDecSparkClass* klass)
{
	GObjectClass* g_klass;
	GParamSpec* pspec;
	GstElementClass* gst_klass;

	/* GST GOO VIDEO_FILTER */
	GstGooVideoFilterClass* gst_c_klass = GST_GOO_VIDEO_FILTER_CLASS (klass);
	/*gst_c_klass->codec_data_processing_func = GST_DEBUG_FUNCPTR (gst_goo_decspark_codec_data_processing);*/

	return;
}

static void
gst_goo_decspark_init (GstGooDecSpark* self, GstGooDecSparkClass* klass)
{
	GST_DEBUG ("");

	GST_GOO_VIDEO_FILTER (self)->component = goo_component_factory_get_component
		(GST_GOO_VIDEO_FILTER (self)->factory, GOO_TI_SPARK_DECODER);

	GooComponent* component = GST_GOO_VIDEO_FILTER (self)->component;

	/* Select Stream mode operation as default */
	g_object_set (G_OBJECT (component), "process-mode", GOO_TI_VIDEO_DECODER_FRAMEMODE, NULL);

	/* input port */
	GST_GOO_VIDEO_FILTER (self)->inport = goo_component_get_port (component, "input0");
	g_assert (GST_GOO_VIDEO_FILTER (self)->inport != NULL);

	{
		GooPort* port = GST_GOO_VIDEO_FILTER (self)->inport;
		GOO_PORT_GET_DEFINITION (port)->format.video.nFrameWidth = DEFAULT_WIDTH;
		GOO_PORT_GET_DEFINITION (port)->format.video.nFrameHeight = DEFAULT_HEIGHT;
		/*GOO_PORT_GET_DEFINITION (port)->format.video.eColorFormat = DEFAULT_COLOR_FORMAT;*/
		/*GOO_PORT_GET_DEFINITION (port)->format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;*/
		GOO_PORT_GET_DEFINITION (port)->format.video.eColorFormat = OMX_COLOR_FormatCbYCrY;
	}

	GST_GOO_VIDEO_FILTER (self)->outport = goo_component_get_port (component, "output0");
	g_assert (GST_GOO_VIDEO_FILTER (self)->outport != NULL);

	{
		GooPort* port = GST_GOO_VIDEO_FILTER (self)->outport;
		GOO_PORT_GET_DEFINITION (port)->format.video.nFrameWidth = DEFAULT_WIDTH;
		GOO_PORT_GET_DEFINITION (port)->format.video.nFrameHeight = DEFAULT_HEIGHT;
		/*GOO_PORT_GET_DEFINITION (port)->format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;*/
		GOO_PORT_GET_DEFINITION (port)->format.video.eColorFormat = OMX_COLOR_FormatCbYCrY;

		/** Use the PARENT's callback function **/
		goo_port_set_process_buffer_function
			(port, gst_goo_video_filter_outport_buffer);

	}

	g_object_set_data (G_OBJECT (GST_GOO_VIDEO_FILTER (self)->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", GST_GOO_VIDEO_FILTER (self)->component);

        return;
}
