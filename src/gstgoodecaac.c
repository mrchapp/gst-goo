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

#include <goo-ti-aacdec.h>
#include "gstgoodecaac.h"

GST_BOILERPLATE (GstGooDecAac, gst_goo_decaac, GstGooAudioFilter, GST_TYPE_GOO_AUDIO_FILTER);

GST_DEBUG_CATEGORY_STATIC (gst_goo_decaac_debug);
#define GST_CAT_DEFAULT gst_goo_decaac_debug

/* signals */
enum
{
        LAST_SIGNAL
};

/* args */
enum
{
	PROP_0,
	PROP_PROFILE,
	PROP_SBR,
	PROP_BITOUTPUT,
	PROP_PARAMETRICSTEREO
};

static gboolean gst_goo_decaac_src_setcaps (GstPad *pad, GstCaps *caps);
static gboolean gst_goo_decaac_sink_setcaps (GstPad *pad, GstCaps *caps);

#define GST_GOO_DECAAC_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_GOO_DECAAC, GstGooDecAacPrivate))

struct _GstGooDecAacPrivate
{
	guint incount;
	guint outcount;
};

/* default values */
#define BITPERSAMPLE_DEFAULT 16
#define CHANNELS_DEFAULT 2
#define PROFILE_DEFAULT OMX_AUDIO_AACObjectHE_PS
#define RAW_DEFAULT OMX_AUDIO_AACStreamFormatRAW;
#define SAMPLERATE_DEFAULT 44100
#define INPUT_BUFFERSIZE_DEFAULT 2000
#define OUTPUT_BUFFERSIZE_DEFAULT 8192
#define NUM_INPUT_BUFFERS_DEFAULT 1
#define NUM_OUTPUT_BUFFERS_DEFAULT 1
#define DEFAULT_WIDTH 16
#define DEFAULT_DEPTH 16

static const GstElementDetails details =
        GST_ELEMENT_DETAILS (
                "OpenMAX AAC decoder",
                "Codec/Decoder/Audio",
                "Decodes Advanced Audio Coding streams with OpenMAX",
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


/* FIXME: make three caps, for mpegversion 1, 2 and 2.5 */
static GstStaticPadTemplate sink_factory =
        GST_STATIC_PAD_TEMPLATE (
                "sink",
                GST_PAD_SINK,
                GST_PAD_ALWAYS,
                GST_STATIC_CAPS ("audio/mpeg, "
				"mpegversion = (int) 4, "
				"rate = (int) [8000, 96000],"
				"channels = (int) [1, 8] "
				));

static gboolean
_goo_ti_aacdec_set_profile (GstGooDecAac* self, guint profile)
{
	g_assert (self != NULL);
	GooComponent * component = GST_GOO_AUDIO_FILTER(self)->component;


	gboolean retval = TRUE;


	self->profile = profile;


	switch (self->profile)
	{
		case 0:
			GOO_TI_AACDEC_GET_INPUT_PORT_PARAM (component)->eAACProfile = OMX_AUDIO_AACObjectMain;
			break;
		case 1:
			GOO_TI_AACDEC_GET_INPUT_PORT_PARAM (component)->eAACProfile = OMX_AUDIO_AACObjectLC;
			break;
		case 2:
			GOO_TI_AACDEC_GET_INPUT_PORT_PARAM (component)->eAACProfile = OMX_AUDIO_AACObjectSSR;
			break;
		case 3:
			GOO_TI_AACDEC_GET_INPUT_PORT_PARAM (component)->eAACProfile = OMX_AUDIO_AACObjectLTP;
			break;
		default:
			GOO_TI_AACDEC_GET_INPUT_PORT_PARAM (component)->eAACProfile = OMX_AUDIO_AACObjectMain;
			break;
	}


	return retval;

}

static gboolean
_goo_ti_aacdec_set_sbr (GstGooDecAac* self, gboolean sbr)
{
	g_assert (self != NULL);
	GooComponent* component = GST_GOO_AUDIO_FILTER (self)->component;

	gboolean retval = TRUE;

	self->sbr = sbr;

	if (sbr == TRUE)
	{
		GOO_TI_AACDEC_GET_INPUT_PORT_PARAM (component)->eAACProfile = OMX_AUDIO_AACObjectHE;
	}

	return retval;

}

static gboolean
_goo_ti_aacdec_set_parametric_stereo (GstGooDecAac* self, gboolean parametric_stereo)
{
	g_assert (self != NULL);
	GooComponent* component = GST_GOO_AUDIO_FILTER (self)->component;

	gboolean retval = TRUE;

	self->parametric_stereo = parametric_stereo;

	if (parametric_stereo == TRUE)
	{
		GOO_TI_AACDEC_GET_INPUT_PORT_PARAM (component)->eAACProfile = OMX_AUDIO_AACObjectHE_PS;
	}

	return retval;

}

static gboolean
_goo_ti_aacdec_set_bit_output (GstGooDecAac* self, gboolean bit_output)
{
	g_assert (self != NULL);

	GooComponent * component = GST_GOO_AUDIO_FILTER (self)->component;
	gboolean retval = TRUE;


	self->bit_output = bit_output;


	switch (self->bit_output)
	{
		case 0:
			GOO_TI_AACDEC_GET_OUTPUT_PORT_PARAM (component)->nBitPerSample = 16;
			break;
		case 1:
			GOO_TI_AACDEC_GET_OUTPUT_PORT_PARAM (component)->nBitPerSample = 24;
			break;
		default:
			GOO_TI_AACDEC_GET_OUTPUT_PORT_PARAM (component)->nBitPerSample = 16;
			break;
	}


	return retval;

}
static gboolean
gst_goo_decaac_process_mode_default (GstGooAudioFilter *self, guint value)
{
	GooComponent *component = GST_GOO_AUDIO_FILTER (self)->component;

	g_object_set (G_OBJECT (component), "frame-mode", value ? TRUE : FALSE, NULL);

	return TRUE;
}

static void
gst_goo_decaac_set_property (GObject* object, guint prop_id,
				const GValue* value, GParamSpec* pspec)
{
	g_assert (GST_IS_GOO_DECAAC (object));
	GstGooDecAac* self = GST_GOO_DECAAC (object);


	switch (prop_id)
	{
	case PROP_PROFILE:
		_goo_ti_aacdec_set_profile (self,
		g_value_get_uint (value));
		break;
	case PROP_SBR:
		_goo_ti_aacdec_set_sbr (self,
		g_value_get_boolean (value));
		break;
	case PROP_BITOUTPUT:
		_goo_ti_aacdec_set_bit_output (self,
		g_value_get_boolean (value));
		break;
	case PROP_PARAMETRICSTEREO:
		_goo_ti_aacdec_set_parametric_stereo (self,
		g_value_get_boolean (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_decaac_get_property (GObject* object, guint prop_id,
				GValue* value, GParamSpec* pspec)
{
	g_assert (GST_IS_GOO_DECAAC (object));
	GstGooDecAac* self = GST_GOO_DECAAC (object);

	switch (prop_id)
	{
		case PROP_PROFILE:
			g_value_set_uint (value, self->profile);
			break;
		case PROP_SBR:
			g_value_set_boolean (value, self->sbr);
			break;
		case PROP_BITOUTPUT:
			g_value_set_boolean (value, self->bit_output);
			break;
		case PROP_PARAMETRICSTEREO:
			g_value_set_boolean (value, self->parametric_stereo);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}

	return;
}

static void
gst_goo_decaac_dispose (GObject* object)
{
        G_OBJECT_CLASS (parent_class)->dispose (object);

        return;
}

static void
gst_goo_decaac_base_init (gpointer g_klass)
{
        GST_DEBUG_CATEGORY_INIT (gst_goo_decaac_debug, "goodecaac", 0,
                                 "OpenMAX Advanced Audio Coding decoder element");

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

static gboolean
gst_goo_decaac_check_fixed_src_caps (GstGooAudioFilter* filter)
{
	GstGooDecAac *self = GST_GOO_DECAAC (filter);
	OMX_AUDIO_PARAM_PCMMODETYPE *param;
	GstCaps *src_caps;

	param = GOO_TI_AACDEC_GET_OUTPUT_PORT_PARAM (GST_GOO_AUDIO_FILTER (self)->component);

	src_caps = gst_caps_new_simple (
								"audio/x-raw-int",
								"endianness", G_TYPE_INT, G_BYTE_ORDER,
								"signed", G_TYPE_BOOLEAN, TRUE,
								"width", G_TYPE_INT, 16,
								"depth", G_TYPE_INT, 16,
								"rate", G_TYPE_INT, param->nSamplingRate,
								"channels", G_TYPE_INT, param->nChannels,
								NULL
								);

	filter->src_caps = gst_caps_ref (src_caps);
	gst_pad_set_caps (GST_GOO_AUDIO_FILTER (self)->srcpad, src_caps);
	gst_caps_unref (src_caps);

	return TRUE;
}

static void
gst_goo_decaac_class_init (GstGooDecAacClass* klass)
{
	GObjectClass* g_klass = G_OBJECT_CLASS (klass);
	GParamSpec* pspec;
	GstElementClass* gst_klass;

	/* gobject */

	g_type_class_add_private (klass, sizeof (GstGooDecAacPrivate));

	g_klass->set_property =
		GST_DEBUG_FUNCPTR (gst_goo_decaac_set_property);
	g_klass->get_property =
		GST_DEBUG_FUNCPTR (gst_goo_decaac_get_property);
	g_klass->dispose = GST_DEBUG_FUNCPTR (gst_goo_decaac_dispose);

	GParamSpec* spec;

	spec = g_param_spec_uint ("profile", "Profile",
							  "The EProfile",
							  0, 3, 0, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_PROFILE, spec);

	spec = g_param_spec_boolean ("SBR", "Sbr",
							  "HE-AAC format",
							  FALSE, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_SBR, spec);

	spec = g_param_spec_boolean ("bit-output", "Bit output",
							  "Setting Profiles",
							  FALSE, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_BITOUTPUT, spec);

	spec = g_param_spec_boolean ("parametric-stereo", "Parametric Stereo",
							  "eAAC+ format",
							  FALSE, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_PARAMETRICSTEREO, spec);

	/* GST GOO FILTER */
	GstGooAudioFilterClass* gst_c_klass = GST_GOO_AUDIO_FILTER_CLASS (klass);
	gst_c_klass->check_fixed_src_caps_func = GST_DEBUG_FUNCPTR (gst_goo_decaac_check_fixed_src_caps);
    gst_c_klass->set_process_mode_func = GST_DEBUG_FUNCPTR (gst_goo_decaac_process_mode_default);

	return;
}

static void
gst_goo_decaac_init (GstGooDecAac* self, GstGooDecAacClass* klass)
{
	GST_DEBUG ("");

	GST_GOO_AUDIO_FILTER (self)->component = goo_component_factory_get_component
		(GST_GOO_AUDIO_FILTER (self)->factory, GOO_TI_AAC_DECODER);

	GooComponent* component = GST_GOO_AUDIO_FILTER (self)->component;

GST_DEBUG ("component=0x%08x\n", component);

	/* Select Stream mode operation as default */
	g_object_set (G_OBJECT (component), "dasf-mode", FALSE,
				  "frame-mode", FALSE,
				  NULL);

	/* component default parameters */
	{
		/* input port */
		GOO_TI_AACDEC_GET_INPUT_PORT_PARAM (component)->nChannels = CHANNELS_DEFAULT;
		GOO_TI_AACDEC_GET_INPUT_PORT_PARAM (component)->nSampleRate = SAMPLERATE_DEFAULT;
		GOO_TI_AACDEC_GET_INPUT_PORT_PARAM (component)->eAACProfile = PROFILE_DEFAULT;
		GOO_TI_AACDEC_GET_INPUT_PORT_PARAM (component)->eAACStreamFormat = RAW_DEFAULT;
		/* output port */
		GOO_TI_AACDEC_GET_OUTPUT_PORT_PARAM (component)->nBitPerSample = BITPERSAMPLE_DEFAULT;

	}

	/* input port */
	GST_GOO_AUDIO_FILTER (self)->inport = goo_component_get_port (component, "input0");
	g_assert (GST_GOO_AUDIO_FILTER (self)->inport != NULL);

	{
		GooPort* port = GST_GOO_AUDIO_FILTER (self)->inport;
		GOO_PORT_GET_DEFINITION (port)-> nBufferSize = INPUT_BUFFERSIZE_DEFAULT;
		GOO_PORT_GET_DEFINITION (port)-> format.audio.eEncoding = OMX_AUDIO_CodingAAC;
	}

	GST_GOO_AUDIO_FILTER (self)->outport = goo_component_get_port (component, "output0");
	g_assert (GST_GOO_AUDIO_FILTER (self)->outport != NULL);

	/* output port */
	{
		GooPort* port = GST_GOO_AUDIO_FILTER (self)->outport;

		GOO_PORT_GET_DEFINITION (port)-> nBufferSize = OUTPUT_BUFFERSIZE_DEFAULT;

		/** Use the PARENT's callback function **/
		goo_port_set_process_buffer_function (port, gst_goo_audio_filter_outport_buffer);

	}

	GstGooDecAacPrivate* priv = GST_GOO_DECAAC_GET_PRIVATE (self);
	priv->incount = 0;
	priv->outcount = 0;

	/* Setcaps functions */
	gst_pad_set_setcaps_function (GST_GOO_AUDIO_FILTER(self)->sinkpad, gst_goo_decaac_sink_setcaps);
	gst_pad_set_setcaps_function (GST_GOO_AUDIO_FILTER(self)->srcpad, gst_goo_decaac_src_setcaps);

	g_object_set_data (G_OBJECT (GST_GOO_AUDIO_FILTER (self)->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", GST_GOO_AUDIO_FILTER (self)->component);

        return;
}

static gboolean
gst_goo_decaac_sink_setcaps (GstPad *pad, GstCaps *caps)
{
	GstStructure *structure;
	GstGooDecAac *self;
	GooComponent *component;
	GstPad *peer;
	GstBuffer *audio_header;
	GstElement *prev_element;
	gchar *str_caps;
	gchar *str_peer;
	gchar compare[] = "queue";
	gint comp_res;
	guint sample_rate = SAMPLERATE_DEFAULT;
	guint channels = CHANNELS_DEFAULT;
	const GValue *value;
	static gint sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000};

	self = GST_GOO_DECAAC (GST_PAD_PARENT (pad));
	GST_DEBUG_OBJECT (self, "sink_setcaps");
	component = (GooComponent *) GST_GOO_AUDIO_FILTER (self)->component;
	peer = gst_pad_get_peer (pad);
	prev_element = GST_ELEMENT (gst_pad_get_parent (peer));
	str_peer = gst_element_get_name (prev_element);
	comp_res = strncmp (compare, str_peer, 5);

	g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE);

	structure = gst_caps_get_structure (caps, 0);
	str_caps = gst_structure_to_string (structure);
	GST_DEBUG_OBJECT (self, "sink caps: %s", str_caps);
	g_free (str_caps);

	if (gst_structure_has_field (structure, "codec_data"))
	{
		gint iIndex = 0;

		value = gst_structure_get_value (structure, "codec_data");
		audio_header = gst_value_get_buffer (value);

		/* Lets see if we can get some information out of this crap */
		iIndex = (((GST_BUFFER_DATA (audio_header)[0] & 0x7) << 1) |
				((GST_BUFFER_DATA (audio_header)[1] & 0x80) >> 7));
		channels = (GST_BUFFER_DATA (audio_header)[1] & 0x78) >> 3;

		GOO_TI_AACDEC_GET_INPUT_PORT_PARAM (component)->nSampleRate = sample_rates[iIndex];
		GOO_TI_AACDEC_GET_INPUT_PORT_PARAM (component)->nChannels = channels;
		GOO_TI_AACDEC_GET_INPUT_PORT_PARAM (component)->eAACStreamFormat = OMX_AUDIO_AACStreamFormatRAW;
	}
	else
	{
		gst_structure_get_int (structure, "rate", &sample_rate);
		gst_structure_get_int (structure, "channels", &channels);
		GOO_TI_AACDEC_GET_INPUT_PORT_PARAM (component)->nChannels = channels;
		if ((comp_res == 0) || ((self->sbr == FALSE) && (self->parametric_stereo == FALSE)))
		{
			GOO_TI_AACDEC_GET_INPUT_PORT_PARAM (component)->nSampleRate = sample_rate;
		}
		else
		{
			GOO_TI_AACDEC_GET_INPUT_PORT_PARAM (component)->nSampleRate = sample_rate / 2;
		}
		GOO_TI_AACDEC_GET_INPUT_PORT_PARAM (component)->eAACStreamFormat = OMX_AUDIO_AACStreamFormatMax;
	}

	gst_object_unref (prev_element);
	gst_object_unref (peer);

	return gst_pad_set_caps (pad,caps);
}

static gboolean
gst_goo_decaac_src_setcaps (GstPad *pad, GstCaps *caps)
{
	GstStructure *structure;
	GstGooDecAac *self;
	GooPort *outport;
	guint width = DEFAULT_WIDTH;
	guint depth = DEFAULT_DEPTH;
	gchar *str_caps;

	self = GST_GOO_DECAAC (GST_PAD_PARENT (pad));
	outport = (GooPort *) GST_GOO_AUDIO_FILTER (self)->outport;

	GST_DEBUG_OBJECT (self, "src_setcaps");
	GST_LOG ("caps are %" GST_PTR_FORMAT, caps);
	g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE);

	structure = gst_caps_get_structure (caps, 0);

	gst_structure_get_int (structure, "width", &width);
	gst_structure_get_int (structure, "depth", &depth);

	return gst_pad_set_caps (pad, caps);
}
