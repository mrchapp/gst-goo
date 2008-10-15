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

#include "gstgooench264.h"

GST_BOILERPLATE (GstGooEncH264, gst_goo_ench264, GstGooVideoEnc, GST_TYPE_GOO_VIDEOENC);

GST_DEBUG_CATEGORY_STATIC (gst_goo_ench264_debug);
#define GST_CAT_DEFAULT gst_goo_ench264_debug

/* signals */
enum
{
        LAST_SIGNAL
};

/* args */
enum
{
	PROP_0,
	PROP_LEVEL
};

/* default values */
#define DEFAULT_BITRATE 768000
#define DEFAULT_CONTROLRATE GOO_TI_VIDEO_ENCODER_CR_VARIABLE
#define DEFAULT_LEVEL OMX_VIDEO_AVCLevel3

static const GstElementDetails details =
        GST_ELEMENT_DETAILS (
                "OpenMAX H264 encoder",
                "Codedc/Encoder/Video",
                "Decodes H264 streams with OpenMAX",
                "Texas Instrument"
                );

static GstStaticPadTemplate src_factory =
GST_STATIC_PAD_TEMPLATE ("src",
		GST_PAD_SRC,
		GST_PAD_ALWAYS,
                GST_STATIC_CAPS ("video/x-h264, "
				"width = (int) [16, 4096], "
				"height = (int) [16, 4096], "
				"framerate = (GstFraction) [1/1, 60/1]"));

static void
gst_goo_ench264_set_property (GObject* object, guint prop_id,
				const GValue* value, GParamSpec* pspec)
{
	GstGooEncH264* self = GST_GOO_ENCH264 (object);
	GooComponent* component = GST_GOO_VIDEO_FILTER (self)->component;

	switch (prop_id)
	{
		case PROP_LEVEL:
		{
			g_object_set (G_OBJECT (component), "level", g_value_get_enum (value), NULL);
			break;
		}
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}

	return;
}

static void
gst_goo_ench264_get_property (GObject* object, guint prop_id,
				GValue* value, GParamSpec* pspec)
{
	GstGooEncH264* self = GST_GOO_ENCH264 (object);
	GooComponent* component = GST_GOO_VIDEO_FILTER (self)->component;

	switch (prop_id)
	{
		case PROP_LEVEL:
		{
			gint level;
			g_object_get (G_OBJECT (component), "level", &level, NULL);
			g_value_set_enum (value, level);
			break;
		}
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}

	return;
}

static void
gst_goo_ench264_base_init (gpointer g_klass)
{
	GST_DEBUG_CATEGORY_INIT (gst_goo_ench264_debug, "gooench264", 0,
		"OpenMAX H264 encoder element");

	GstElementClass* e_klass = GST_ELEMENT_CLASS (g_klass);

	gst_element_class_add_pad_template (e_klass,
		gst_static_pad_template_get
		(&src_factory));

	gst_element_class_set_details (e_klass, &details);

	return;
}

static void
gst_goo_ench264_class_init (GstGooEncH264Class* klass)
{
	GObjectClass* g_klass;
	GParamSpec* pspec;
	GstElementClass* gst_klass;

	/* gobject */
	g_klass = G_OBJECT_CLASS (klass);

	g_klass->set_property =
		GST_DEBUG_FUNCPTR (gst_goo_ench264_set_property);
	g_klass->get_property =
		GST_DEBUG_FUNCPTR (gst_goo_ench264_get_property);

	GParamSpec* spec;

	spec = g_param_spec_enum ("level", "Encoding Level",
			"Specifies the compression encoding level",
			GOO_TI_H264ENC_LEVEL,
			DEFAULT_LEVEL, G_PARAM_READWRITE);

	g_object_class_install_property (g_klass, PROP_LEVEL, spec);

	return;
}

static void
gst_goo_ench264_init (GstGooEncH264* self, GstGooEncH264Class* klass)
{
	GST_DEBUG ("");

	GST_GOO_VIDEO_FILTER (self)->component = goo_component_factory_get_component
		(GST_GOO_VIDEO_FILTER (self)->factory, GOO_TI_H264_ENCODER);

	GooComponent* component = GST_GOO_VIDEO_FILTER (self)->component;

	/* Select default properties */
	g_object_set (G_OBJECT (component),
		"level", DEFAULT_LEVEL,
		"control-rate", DEFAULT_CONTROLRATE,
		NULL);


	/* input port */
	GST_GOO_VIDEO_FILTER (self)->inport = goo_component_get_port (component, "input0");
	g_assert (GST_GOO_VIDEO_FILTER (self)->inport != NULL);

	{
		GooPort* port = GST_GOO_VIDEO_FILTER (self)->inport;
		GOO_PORT_GET_DEFINITION (port)->format.video.cMIMEType = "yuv";
	}

	GST_GOO_VIDEO_FILTER (self)->outport = goo_component_get_port (component, "output0");
	g_assert (GST_GOO_VIDEO_FILTER (self)->outport != NULL);

	/* output port */
	{
		GooPort* port = GST_GOO_VIDEO_FILTER (self)->outport;
		GOO_PORT_GET_DEFINITION (port)->format.video.cMIMEType = "264";
		GOO_PORT_GET_DEFINITION (port)->format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
		GOO_PORT_GET_DEFINITION (port)->format.video.nBitrate = DEFAULT_BITRATE;

		/** Use the PARENT's callback function **/
		goo_port_set_process_buffer_function
		(port, gst_goo_video_filter_outport_buffer);

	}

	g_object_set_data (G_OBJECT (GST_GOO_VIDEO_FILTER (self)->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", GST_GOO_VIDEO_FILTER (self)->component);

        return;
}
