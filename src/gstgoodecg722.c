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

#include <goo-ti-g722dec.h>
#include "gstgoodecg722.h"
#define OMX 0x0101

GST_BOILERPLATE (GstGooDecG722, gst_goo_decg722, GstGooAudioFilter, GST_TYPE_GOO_AUDIO_FILTER);

GST_DEBUG_CATEGORY_STATIC (gst_goo_decg722_debug);
#define GST_CAT_DEFAULT gst_goo_decg722_debug

/* signals */
enum
{
        LAST_SIGNAL
};

/* args */
enum
{
	PROP_0,
};

static gboolean gst_goo_decg722_sink_setcaps (GstPad *pad, GstCaps *caps);

/* default values */
#define CHANNELS_DEFAULT 1
#define SAMPLERATE_DEFAULT 48000
#define INPUT_BUFFERSIZE_DEFAULT 1024
#define NUM_INPUT_BUFFERS_DEFAULT 1
#define DEFAULT_WIDTH 16
#define DEFAULT_DEPTH 16

static const GstElementDetails details =
        GST_ELEMENT_DETAILS (
                "OpenMAX G722 decoder",
                "Codec/Decoder/Audio",
                "Decodes G722 streams with OpenMAX",
                "Texas Instrument"
                );

static GstStaticPadTemplate src_factory =
GST_STATIC_PAD_TEMPLATE ("src",
		GST_PAD_SRC,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS ("audio/x-raw-int, "
				"rate = (int) [8000, 192000], "
				"channels = (int) [1, 8], "
				"width = (int) [8, 4096], "
				"depth = (int) [8, 4096] "
				));

static GstStaticPadTemplate sink_factory =
        GST_STATIC_PAD_TEMPLATE (
                "sink",
                GST_PAD_SINK,
                GST_PAD_ALWAYS,
		GST_STATIC_CAPS ("audio/x-adpcm, "
				"rate = (int) [8000, 192000], "
				"layout = (string) {g722}, "
				"channels = (int) [1, 2]"
				));

static void
gst_goo_decg722_dispose (GObject* object)
{
        G_OBJECT_CLASS (parent_class)->dispose (object);

        return;
}

static void
gst_goo_decg722_base_init (gpointer g_klass)
{
        GST_DEBUG_CATEGORY_INIT (gst_goo_decg722_debug, "goodecg722", 0,
                                 "OpenMAX G722 decoder element");

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
gst_goo_decg722_class_init (GstGooDecG722Class* klass)
{
	GObjectClass* g_klass = G_OBJECT_CLASS (klass);
	GstElementClass* gst_klass;

	/* gobject */

	g_klass->dispose = GST_DEBUG_FUNCPTR (gst_goo_decg722_dispose);

	/* GST GOO FILTER */
	GstGooAudioFilterClass* gst_c_klass = GST_GOO_AUDIO_FILTER_CLASS (klass);
	return;
}

static void
gst_goo_decg722_init (GstGooDecG722* self, GstGooDecG722Class* klass)
{
	GST_DEBUG ("");

	GST_GOO_AUDIO_FILTER (self)->component = goo_component_factory_get_component
		(GST_GOO_AUDIO_FILTER (self)->factory, GOO_TI_G722_DECODER);

	GooComponent* component = GST_GOO_AUDIO_FILTER (self)->component;

	/* component default parameters */
	{
		/* input port */
                GOO_TI_G722DEC_GET_INPUT_PARAM (component)->nChannels = CHANNELS_DEFAULT;
	}

	/* input port */
	GST_GOO_AUDIO_FILTER (self)->inport = goo_component_get_port (component, "input0");
	g_assert (GST_GOO_AUDIO_FILTER (self)->inport != NULL);

	{
		GooPort* port = GST_GOO_AUDIO_FILTER (self)->inport;
		GOO_PORT_GET_DEFINITION (port)-> nBufferSize = INPUT_BUFFERSIZE_DEFAULT;
	}

	GST_GOO_AUDIO_FILTER (self)->outport = goo_component_get_port (component, "output0");
	g_assert (GST_GOO_AUDIO_FILTER (self)->outport != NULL);


	/* Setcaps functions */
	gst_pad_set_setcaps_function (GST_GOO_AUDIO_FILTER(self)->sinkpad, gst_goo_decg722_sink_setcaps);

	g_object_set_data (G_OBJECT (GST_GOO_AUDIO_FILTER (self)->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", GST_GOO_AUDIO_FILTER (self)->component);

        return;
}

static gboolean
gst_goo_decg722_sink_setcaps (GstPad *pad, GstCaps *caps)
{
	GstStructure *structure;
	GstGooDecG722 *self;
	GooComponent *component;
	gchar *str_caps;
	guint sample_rate = SAMPLERATE_DEFAULT;
	guint channels = CHANNELS_DEFAULT;

	self = GST_GOO_DECG722 (GST_PAD_PARENT (pad));
	component = (GooComponent *) GST_GOO_AUDIO_FILTER (self)->component;

	GST_DEBUG_OBJECT (self, "sink_setcaps");

	g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE );

	structure = gst_caps_get_structure (caps, 0);
	str_caps = gst_structure_to_string (structure);
	GST_DEBUG_OBJECT (self, "sink caps: %s", str_caps);
	g_free (str_caps);

	gst_structure_get_int (structure, "rate", &sample_rate);
	gst_structure_get_int (structure, "channels", &channels);

	if (gst_goo_util_structure_is_parsed (structure))
		g_object_set (G_OBJECT (self), "process-mode", 0, NULL);

	GOO_TI_G722DEC_GET_INPUT_PARAM (component)->nSamplingRate = sample_rate;
	GOO_TI_G722DEC_GET_INPUT_PARAM (component)->nChannels = channels;

	return gst_pad_set_caps (pad, caps);
}

