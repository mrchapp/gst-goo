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

#include <goo-ti-imaadpcmdec.h>
#include "gstgoodecimaadpcm.h"
#define OMX 0x0101

GST_BOILERPLATE (GstGooDecImaAdPcm, gst_goo_decimaadpcm, GstGooAudioFilter, GST_TYPE_GOO_AUDIO_FILTER);

GST_DEBUG_CATEGORY_STATIC (gst_goo_decimaadpcm_debug);
#define GST_CAT_DEFAULT gst_goo_decimaadpcm_debug

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

static gboolean gst_goo_decimaadpcm_sink_setcaps (GstPad *pad, GstCaps *caps);

/* default values */
#define ALIGN_DEFAULT 256
#define CHANNELS_DEFAULT 1
#define SAMPLERATE_DEFAULT 8000
#define INPUT_BUFFERSIZE_DEFAULT 256
#define OUTPUT_BUFFERSIZE_DEFAULT 1010
#define DEFAULT_WIDTH 16
#define DEFAULT_DEPTH 16

static const GstElementDetails details =
        GST_ELEMENT_DETAILS (
                "OpenMAX IMA ADPCM decoder",
                "Codec/Decoder/Audio",
                "Decodes IMA ADPCM streams with OpenMAX",
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
				"rate = (int) [1000, 96000], "
				"channels = (int) [1, 2], "
				"block_align = (int) [1, 2147483647] "
				));

static gboolean
gst_goo_decimaadpcm_process_mode_default (GstGooAudioFilter *self, guint value)
{
	GooComponent *component = self->component;

	g_object_set (G_OBJECT (component), "frame-mode", value ? TRUE : FALSE, NULL);

	return TRUE;
}

static void
gst_goo_decimaadpcm_dispose (GObject* object)
{
        G_OBJECT_CLASS (parent_class)->dispose (object);

        return;
}

static void
gst_goo_decimaadpcm_base_init (gpointer g_klass)
{
        GST_DEBUG_CATEGORY_INIT (gst_goo_decimaadpcm_debug, "goodecimaadpcm", 0,
                                 "OpenMAX IMA ADPCM decoder element");

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
gst_goo_decimaadpcm_class_init (GstGooDecImaAdPcmClass* klass)
{
	GObjectClass* g_klass = G_OBJECT_CLASS (klass);
	GstElementClass* gst_klass;

	/* gobject */

	g_klass->dispose = GST_DEBUG_FUNCPTR (gst_goo_decimaadpcm_dispose);

	/* GST GOO FILTER */
	GstGooAudioFilterClass* gst_c_klass = GST_GOO_AUDIO_FILTER_CLASS (klass);
	gst_c_klass->set_process_mode_func = GST_DEBUG_FUNCPTR (gst_goo_decimaadpcm_process_mode_default);
	return;
}

static void
gst_goo_decimaadpcm_init (GstGooDecImaAdPcm* self, GstGooDecImaAdPcmClass* klass)
{
	GST_DEBUG ("");

	GST_GOO_AUDIO_FILTER (self)->component = goo_component_factory_get_component
		(GST_GOO_AUDIO_FILTER (self)->factory, GOO_TI_IMAADPCM_DECODER);

	GooComponent* component = GST_GOO_AUDIO_FILTER (self)->component;

	/* component default parameters */
	{
		GOO_TI_IMAADPCMDEC_GET_INPUT_PARAM (component)->nChannels = CHANNELS_DEFAULT;
		GOO_TI_IMAADPCMDEC_GET_OUTPUT_PARAM (component)->nChannels = CHANNELS_DEFAULT;
	}

	/* input port */
	GST_GOO_AUDIO_FILTER (self)->inport = goo_component_get_port (component, "input0");
	g_assert (GST_GOO_AUDIO_FILTER (self)->inport != NULL);

	{
		GooPort* port = GST_GOO_AUDIO_FILTER (self)->inport;
		GOO_PORT_GET_DEFINITION (port)-> nBufferSize = INPUT_BUFFERSIZE_DEFAULT;
		GOO_PORT_GET_DEFINITION (port)-> format.audio.eEncoding = OMX_AUDIO_CodingADPCM;
	}

	/* output port */
	GST_GOO_AUDIO_FILTER (self)->outport = goo_component_get_port (component, "output0");
	g_assert (GST_GOO_AUDIO_FILTER (self)->outport != NULL);

	{
		GooPort* port = GST_GOO_AUDIO_FILTER (self)->outport;
		GOO_PORT_GET_DEFINITION (port)-> nBufferSize = OUTPUT_BUFFERSIZE_DEFAULT;
		GOO_PORT_GET_DEFINITION (port)-> format.audio.eEncoding = OMX_AUDIO_CodingADPCM;
	}

	/* Setcaps functions */
	gst_pad_set_setcaps_function (GST_GOO_AUDIO_FILTER(self)->sinkpad, gst_goo_decimaadpcm_sink_setcaps);

	g_object_set_data (G_OBJECT (GST_GOO_AUDIO_FILTER (self)->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", GST_GOO_AUDIO_FILTER (self)->component);
	g_object_set(G_OBJECT (self), "process-mode", 1, NULL);

    return;
}

static gboolean
gst_goo_decimaadpcm_sink_setcaps (GstPad *pad, GstCaps *caps)
{
	GstStructure *structure;
	GstGooDecImaAdPcm *self;
	GooComponent *component;
	gchar *str_caps;
	guint block_align = ALIGN_DEFAULT;
	guint sample_rate = SAMPLERATE_DEFAULT;
	guint channels = CHANNELS_DEFAULT;

	self = GST_GOO_DECIMAADPCM (GST_PAD_PARENT (pad));
	component = (GooComponent *) GST_GOO_AUDIO_FILTER (self)->component;

	GST_DEBUG_OBJECT (self, "sink_setcaps");

	g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE );

	structure = gst_caps_get_structure (caps, 0);
	str_caps = gst_structure_to_string (structure);
	GST_DEBUG_OBJECT (self, "sink caps: %s", str_caps);
	g_free (str_caps);

	gst_structure_get_int (structure, "rate", &sample_rate);
	gst_structure_get_int (structure, "channels", &channels);
	gst_structure_get_int (structure, "block_align", &block_align);

	GOO_TI_IMAADPCMDEC_GET_INPUT_PARAM (component)->nSamplingRate = sample_rate;
	GOO_TI_IMAADPCMDEC_GET_INPUT_PARAM (component)->nBitPerSample = block_align;
	GOO_TI_IMAADPCMDEC_GET_INPUT_PARAM (component)->nChannels = channels;
	GOO_TI_IMAADPCMDEC_GET_OUTPUT_PARAM (component)->nSamplingRate = sample_rate;
	GOO_TI_IMAADPCMDEC_GET_OUTPUT_PARAM (component)->nBitPerSample = block_align;
	GOO_TI_IMAADPCMDEC_GET_OUTPUT_PARAM (component)->nChannels = channels;

	return gst_pad_set_caps (pad, caps);
}
