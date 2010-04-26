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

#include <goo-ti-nbamrdec.h>
#include <goo-component.h>
#include <goo-ti-audio-manager.h>

#if 1
#include <TIDspOmx.h>
#endif

#include "gstgoodecnbamr.h"

GST_BOILERPLATE (GstGooDecNbAmr, gst_goo_decnbamr, GstGooAudioFilter, GST_TYPE_GOO_AUDIO_FILTER);

GST_DEBUG_CATEGORY_STATIC (gst_goo_decnbamr_debug);
#define GST_CAT_DEFAULT gst_goo_decnbamr_debug

/* signals */
enum
{
	LAST_SIGNAL
};

/* args */
enum
{
	PROP_0,
	PROP_MIME,
	PROP_DTX_MODE,
	PROP_MIMO_MODE
};

static GType
gst_goo_decnbamr_dtxmode_get_type ()
{
	static GType gst_goo_decnbamr_dtxmode_type = 0;
	static GEnumValue gst_goo_decnbamr_dtxmode[] =
 	{
		{OMX_AUDIO_AMRDTXModeOff,	 "Off", "DTX Off"},
		{OMX_AUDIO_AMRDTXModeOnVAD1, "VAD1",
			"Voice Activity Detector 1"},
		{OMX_AUDIO_AMRDTXasEFR, "EFR",
			"DTX as EFR instead of AMR Standard"},
		{0, NULL, NULL},
	};

	gst_goo_decnbamr_dtxmode_type = g_enum_register_static ("GstGooNbAmrDecDtxMode", gst_goo_decnbamr_dtxmode);

	return gst_goo_decnbamr_dtxmode_type;
}

#define GST_GOO_DECNBAMR_DTXMODE_TYPE (gst_goo_decnbamr_dtxmode_get_type())

static gboolean gst_goo_decnbamr_src_setcaps (GstPad *pad, GstCaps *caps);
static gboolean gst_goo_decnbamr_sink_setcaps (GstPad *pad, GstCaps *caps);

#define GST_GOO_DECNBAMR_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_GOO_DECNBAMR, GstGooDecNbAmrPrivate))

struct _GstGooDecNbAmrPrivate
{
	guint incount;
	guint outcount;
};

/* default values */
#define CHANNELS_DEFAULT 1
#define INPUT_BUFFERSIZE 118
#define INPUT_BUFFERSIZE_EFR 120
#define INPUT_BUFFERSIZE_MIME 34
#define OUTPUT_BUFFERSIZE 320
#define DEFAULT_DTX_MODE OMX_AUDIO_AMRDTXModeOff
#define DEFAULT_FRAME_FORMAT OMX_AUDIO_AMRFrameFormatFSF
#define DEFAULT_BITRATE 8000
#define DEFAULT_MIME_MODE TRUE
#define DEFAULT_MIMO_MODE FALSE
#define NUM_INPUT_BUFFERS_DEFAULT 1
#define NUM_OUTPUT_BUFFERS_DEFAULT 1
#define DEFAULT_AMR_BANDMODE 1
#define DEFAULT_WIDTH 16
#define DEFAULT_DEPTH 16

static const GstElementDetails details =
		GST_ELEMENT_DETAILS (
				"OpenMAX NarrowBand AMR decoder",
				"Codec/Decoder/Audio",
				"Decodes NarrowBand AMR streams with OpenMAX",
				"Texas Instruments"
				);

static const GstStaticPadTemplate src_factory =
		GST_STATIC_PAD_TEMPLATE ("src",
				GST_PAD_SRC,
				GST_PAD_ALWAYS,
				GST_STATIC_CAPS ("audio/x-raw-int,"
						"rate = (int) [8000, 96000], "
						"channels = (int) [1, 2], "
						"width = (int) 16, "
						"depth = (int) 16 "
						));

static const GstStaticPadTemplate sink_factory =
		GST_STATIC_PAD_TEMPLATE ("sink",
				GST_PAD_SINK,
				GST_PAD_ALWAYS,
				GST_STATIC_CAPS ("audio/AMR, "
						"rate = (int) [8000, 48000], "
						"channels = (int) [1, 2] "
						));

/* For NBAMR MIMO mode must be set to
 * DATAPATH_MIMO_3 according to the OMX people.
 */

static gboolean
_goo_ti_nbamrdec_set_mimo_mode (GstGooDecNbAmr* self)
{
    g_assert (self != NULL);
    GooComponent * component = GST_GOO_AUDIO_FILTER(self)->component;
    TI_OMX_DATAPATH datapath;

    gboolean retval = TRUE;

    switch (self->mimo_mode)
    {
        case TRUE:
            datapath = DATAPATH_MIMO_3;
            break;
        default:
            datapath = DATAPATH_APPLICATION;
            break;
    }

    goo_component_set_config_by_name (component,
            GOO_TI_AUDIO_COMPONENT (component)->datapath_param_name,
            &datapath);

    return retval;
}

static gboolean
_goo_ti_nbamrdec_set_mime (GstGooDecNbAmr* self, gboolean mime)
{
	GST_DEBUG ("\ngoo_ti_nbamrdec_set_mime %d\n", mime);
	g_assert (self != NULL);
	GooComponent* component = GST_GOO_AUDIO_FILTER (self)->component;
	GooIterator *iter;
	GooPort *port;

	gboolean retval = TRUE;

	self->mime = mime;

	/* set input port */
	iter = goo_component_iterate_input_ports (component);
	goo_iterator_nth (iter, 0);
	port = GOO_PORT (goo_iterator_get_current (iter));
	g_assert (port != NULL);

	switch (self->mime)
	{
		case FALSE:
			GOO_PORT_GET_DEFINITION (port)->nBufferSize = INPUT_BUFFERSIZE;
			GOO_TI_NBAMRDEC_GET_INPUT_PORT_PARAM (component)->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatConformance;
			GOO_TI_NBAMRDEC_GET_OUTPUT_PORT_PARAM (component)->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatConformance;
		break;
		case TRUE:
			GOO_PORT_GET_DEFINITION (port)->nBufferSize = INPUT_BUFFERSIZE_MIME;
			GOO_TI_NBAMRDEC_GET_INPUT_PORT_PARAM (component)->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;
			GOO_TI_NBAMRDEC_GET_OUTPUT_PORT_PARAM (component)->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;
			GST_GOO_AUDIO_FILTER (self)->nbamr_mime = TRUE;
		break;
		default:
			GOO_PORT_GET_DEFINITION (port)->nBufferSize = INPUT_BUFFERSIZE_MIME;
			GOO_TI_NBAMRDEC_GET_INPUT_PORT_PARAM (component)->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;
			GOO_TI_NBAMRDEC_GET_OUTPUT_PORT_PARAM (component)->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;
			GST_GOO_AUDIO_FILTER (self)->nbamr_mime = TRUE;
		break;
	}

	g_object_unref (iter);
	g_object_unref (port);

	return retval;
}

static gboolean
_goo_ti_nbamrdec_set_dtx_mode (GstGooDecNbAmr* self, guint dtx_mode)
{
	g_assert (self != NULL);
	GooComponent* component = GST_GOO_AUDIO_FILTER (self)->component;
	GooIterator	*iter;
	GooPort	*port;

	gboolean retval = TRUE;

	self->dtx_mode = dtx_mode;

	/* set input port */
	iter = goo_component_iterate_input_ports (component);
	goo_iterator_nth (iter, 0);
	port = GOO_PORT (goo_iterator_get_current (iter));
	g_assert (port != NULL);

	switch (self->dtx_mode)
	{
		case 0:
			GOO_TI_NBAMRDEC_GET_INPUT_PORT_PARAM (component)->eAMRDTXMode = OMX_AUDIO_AMRDTXModeOff;
		break;
		case 1:
			GOO_TI_NBAMRDEC_GET_INPUT_PORT_PARAM (component)->eAMRDTXMode = OMX_AUDIO_AMRDTXModeOnVAD1;
		break;
		case 4:
			GOO_PORT_GET_DEFINITION (port)->nBufferSize = INPUT_BUFFERSIZE_EFR;
			GOO_TI_NBAMRDEC_GET_INPUT_PORT_PARAM (component)->eAMRDTXMode = OMX_AUDIO_AMRDTXasEFR;
		default:
			GOO_TI_NBAMRDEC_GET_INPUT_PORT_PARAM (component)->eAMRDTXMode = OMX_AUDIO_AMRDTXModeOff;
		break;
	}

	g_object_unref (iter);
	g_object_unref (port);

	return retval;
}

static gboolean
gst_goo_decnbamr_process_mode_default (GstGooAudioFilter *self, guint value)
{
	GooComponent *component = GST_GOO_AUDIO_FILTER (self)->component;

	GST_DEBUG ("gst_goo_decnbamr_process_mode_default %d", value);
	g_object_set (G_OBJECT (component), "frame-mode", value ? TRUE : FALSE, NULL);

	return TRUE;
}

static void
gst_goo_decnbamr_set_property (GObject* object, guint prop_id,
				const GValue* value, GParamSpec* pspec)
{
	GST_DEBUG ("gst_goo_decnbamr_set_property %d", prop_id);
	g_assert (GST_IS_GOO_DECNBAMR (object));
	GstGooDecNbAmr* self = GST_GOO_DECNBAMR (object);

	switch (prop_id)
	{
		case PROP_MIME:
			_goo_ti_nbamrdec_set_mime (self, g_value_get_boolean (value));
		break;
		case PROP_DTX_MODE:
			_goo_ti_nbamrdec_set_dtx_mode (self, g_value_get_enum (value));
		break;
		case PROP_MIMO_MODE:
			self->mimo_mode = g_value_get_boolean (value);
			_goo_ti_nbamrdec_set_mimo_mode (self);
		    break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_decnbamr_get_property (GObject* object, guint prop_id,
					GValue* value, GParamSpec* pspec)
{
	GST_DEBUG ("gst_goo_decnbamr_get_property");
	g_assert (GST_IS_GOO_DECNBAMR (object));
	GstGooDecNbAmr* self = GST_GOO_DECNBAMR (object);

	switch (prop_id)
	{
		case PROP_MIME:
			g_value_set_boolean (value, self->mime);
		break;
		case PROP_DTX_MODE:
			g_value_set_enum (value, self->dtx_mode);
		break;
		case PROP_MIMO_MODE:
		    g_value_set_boolean (value, self->mimo_mode);
		    break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static GstStateChangeReturn
gst_goo_decnbamr_change_state (GstElement* element, GstStateChange transition)
{
    GST_LOG ("");

    GstGooAudioFilter* self = GST_GOO_AUDIO_FILTER (element);
	GstGooDecNbAmr* gNbAmr = GST_GOO_DECNBAMR (self);
    GstStateChangeReturn result;

    g_assert (self->component != NULL);
    g_assert (self->inport != NULL);
    g_assert (self->outport != NULL);

    switch (transition)
    {
    case GST_STATE_CHANGE_NULL_TO_READY:
        break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        if ((GOO_COMPONENT (self->component)->cur_state == OMX_StateIdle) &&
		(gNbAmr->mimo_mode == TRUE))
        {
            GooTiAudioManager* gAm = NULL;
            gint ret;
            gAm =  GOO_TI_AUDIO_COMPONENT (self->component)->manager;
            g_assert (gAm != NULL);
            gAm->cmd->AM_Cmd = AM_CommandWarnSampleFreqChange;
            gAm->cmd->param1 = 8000;
            gAm->cmd->param2 = 6;
            ret = write (gAm->fdwrite, gAm->cmd, sizeof (AM_COMMANDDATATYPE));
            g_assert (ret ==  sizeof (AM_COMMANDDATATYPE));
        }
        break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        break;
    default:
        break;
    }

    gboolean avoid_double_idle = FALSE;

    if (transition != GST_STATE_CHANGE_PAUSED_TO_READY)
    {
        result = GST_ELEMENT_CLASS (parent_class)->change_state (element,
                                 transition);
    }
    else
    {
        result = GST_STATE_CHANGE_SUCCESS;
    }


    switch (transition)
    {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
        /* goo_component_set_state_paused (self->component); */
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        if(GOO_COMPONENT (self->component)->cur_state != OMX_StateIdle)
        {
            GST_INFO ("going to idle");
            goo_component_set_state_idle (self->component);
        }
        GST_INFO ("going to loaded");
        goo_component_set_state_loaded (self->component);
        break;
    case GST_STATE_CHANGE_READY_TO_NULL:
        break;
    default:
        break;
    }

    return result;
}

static void
gst_goo_decnbamr_dispose (GObject* object)
{
	G_OBJECT_CLASS (parent_class)->dispose (object);

	return;
}

static void
gst_goo_decnbamr_base_init (gpointer g_klass)
{
	GST_DEBUG_CATEGORY_INIT (gst_goo_decnbamr_debug, "goodecnbamr", 0,
							 "OpenMAX NarrowBand AMR Coding decoder element");

	GstElementClass* e_klass = GST_ELEMENT_CLASS (g_klass);

	gst_element_class_add_pad_template (e_klass,
										gst_static_pad_template_get
										((GstStaticPadTemplate *)&sink_factory));

	gst_element_class_add_pad_template (e_klass,
										gst_static_pad_template_get
										((GstStaticPadTemplate *)&src_factory));

	gst_element_class_set_details (e_klass, &details);

	return;
}

static gboolean
gst_goo_decnbamr_check_fixed_src_caps (GstGooAudioFilter *filter)
{
	GstGooDecNbAmr* self = GST_GOO_DECNBAMR (filter);
	OMX_AUDIO_PARAM_AMRTYPE *param;
	GstCaps *src_caps;

	param = GOO_TI_NBAMRDEC_GET_OUTPUT_PORT_PARAM (GST_GOO_AUDIO_FILTER (self)->component);

	src_caps = gst_caps_new_simple (
								"audio/x-raw-int",
								"endianness", G_TYPE_INT, G_BYTE_ORDER,
								"signed", G_TYPE_BOOLEAN, TRUE,
								"width", G_TYPE_INT, 16,
								"depth", G_TYPE_INT, 16,
								"rate", G_TYPE_INT, param->nBitRate,
								"channels", G_TYPE_INT, param->nChannels,
								NULL
								);
	filter->src_caps = gst_caps_ref (src_caps);

	gst_pad_set_caps (GST_GOO_AUDIO_FILTER (self)->srcpad, src_caps);
	gst_caps_unref (src_caps);

	return TRUE;
}

static void
gst_goo_decnbamr_class_init (GstGooDecNbAmrClass* klass)
{
	GObjectClass* g_klass = G_OBJECT_CLASS (klass);
	GstElementClass* gst_klass;

	/* gobject */
	g_type_class_add_private (klass, sizeof (GstGooDecNbAmrPrivate));

	g_klass->set_property = GST_DEBUG_FUNCPTR (gst_goo_decnbamr_set_property);
	g_klass->get_property = GST_DEBUG_FUNCPTR (gst_goo_decnbamr_get_property);
	g_klass->dispose = GST_DEBUG_FUNCPTR (gst_goo_decnbamr_dispose);

	GParamSpec* spec;

	spec = g_param_spec_boolean ("mime", "MIME",
							  "Enable using MIME headers",
							  DEFAULT_MIME_MODE, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_MIME, spec);

	spec = g_param_spec_enum ("dtx-mode", "DTX Mode",
							  "Specifies the decoding DTX settings",
							  GST_GOO_DECNBAMR_DTXMODE_TYPE,
							  DEFAULT_DTX_MODE,
							  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_DTX_MODE, spec);
	spec = g_param_spec_boolean ("mimo-mode", "MIMO Mixer Mode",
	                          "Activates MIMO Operation",
	                          DEFAULT_MIMO_MODE, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_MIMO_MODE, spec);

	/* GST GOO FILTER */
	GstGooAudioFilterClass* gst_c_klass = GST_GOO_AUDIO_FILTER_CLASS (klass);
	gst_c_klass->check_fixed_src_caps_func = GST_DEBUG_FUNCPTR (gst_goo_decnbamr_check_fixed_src_caps);
	gst_c_klass->set_process_mode_func = GST_DEBUG_FUNCPTR (gst_goo_decnbamr_process_mode_default);

	gst_klass = GST_ELEMENT_CLASS (klass);

	/* Setting output device for MIMO mode must be done before component
	 * goes to executing state. Therefore, overloading change_state is
	 * a good option.
	 */
	gst_klass->change_state = GST_DEBUG_FUNCPTR (gst_goo_decnbamr_change_state);

	return;
}

static void
gst_goo_decnbamr_init (GstGooDecNbAmr *self, GstGooDecNbAmrClass *klass)
{
	GST_DEBUG ("gst_goo_decnbamr_init");

	GST_GOO_AUDIO_FILTER (self)->component = goo_component_factory_get_component (GST_GOO_AUDIO_FILTER (self)->factory, GOO_TI_NBAMR_DECODER);

	GooComponent* component = GST_GOO_AUDIO_FILTER (self)->component;

	/* component default parameters */
	{
		/* input port */
		GOO_TI_NBAMRDEC_GET_INPUT_PORT_PARAM (component)->nChannels = CHANNELS_DEFAULT;
		GOO_TI_NBAMRDEC_GET_INPUT_PORT_PARAM (component)->nBitRate = DEFAULT_BITRATE;
		GOO_TI_NBAMRDEC_GET_INPUT_PORT_PARAM (component)->eAMRBandMode = DEFAULT_AMR_BANDMODE;
		GOO_TI_NBAMRDEC_GET_INPUT_PORT_PARAM (component)->eAMRDTXMode = DEFAULT_DTX_MODE;
		//GOO_TI_NBAMRDEC_GET_INPUT_PORT_PARAM (component)->eAMRFrameFormat = DEFAULT_FRAME_FORMAT;

		/* output port */
		GOO_TI_NBAMRDEC_GET_OUTPUT_PORT_PARAM (component)->nChannels = CHANNELS_DEFAULT;
		GOO_TI_NBAMRDEC_GET_OUTPUT_PORT_PARAM (component)->nBitRate = DEFAULT_BITRATE;
	}

	/* input port */
	GST_GOO_AUDIO_FILTER (self)->inport = goo_component_get_port (component, "input0");
	g_assert (GST_GOO_AUDIO_FILTER (self)->inport != NULL);

	{
		GooPort* port = GST_GOO_AUDIO_FILTER (self)->inport;
		//GOO_PORT_GET_DEFINITION (port)->nBufferSize = INPUT_BUFFERSIZE;
		GOO_PORT_GET_DEFINITION (port)->format.audio.eEncoding = OMX_AUDIO_CodingAMR;
	}

	/* output port */
	GST_GOO_AUDIO_FILTER (self)->outport = goo_component_get_port (component, "output0");
	g_assert (GST_GOO_AUDIO_FILTER (self)->outport != NULL);

	{
		GooPort* port = GST_GOO_AUDIO_FILTER (self)->outport;

		GOO_PORT_GET_DEFINITION (port)->nBufferSize = OUTPUT_BUFFERSIZE;

		/* USE THE PARENT'S CALLBACK FUNCTION */
		goo_port_set_process_buffer_function (port, gst_goo_audio_filter_outport_buffer);
	}

g_object_set(self, "mime", 1, NULL);

	GstGooDecNbAmrPrivate* priv = GST_GOO_DECNBAMR_GET_PRIVATE (self);
	priv->incount = 0;
	priv->outcount = 0;


	/* Setcaps functions */
	gst_pad_set_setcaps_function (GST_GOO_AUDIO_FILTER (self)->sinkpad, gst_goo_decnbamr_sink_setcaps);
	gst_pad_set_setcaps_function (GST_GOO_AUDIO_FILTER (self)->srcpad, gst_goo_decnbamr_src_setcaps);

	g_object_set_data (G_OBJECT (GST_GOO_AUDIO_FILTER (self)->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", GST_GOO_AUDIO_FILTER (self)->component);

	return;
}

static gboolean
gst_goo_decnbamr_sink_setcaps (GstPad *pad, GstCaps *caps)
{
	GstStructure *structure;
	GstGooDecNbAmr *self;
	GooComponent *component;
	gchar *str_caps;
	guint bit_rate = DEFAULT_BITRATE;
	guint channels = CHANNELS_DEFAULT;
	guint width = DEFAULT_WIDTH;
	guint depth = DEFAULT_DEPTH;

	self = GST_GOO_DECNBAMR (GST_PAD_PARENT (pad));
	component = (GooComponent *) GST_GOO_AUDIO_FILTER (self)->component;

	GST_DEBUG_OBJECT (self, "sink_setcaps");

	g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE);

	structure = gst_caps_get_structure (caps, 0);
	str_caps = gst_structure_to_string (structure);
	GST_DEBUG_OBJECT (self, "sink caps: %s", str_caps);
	g_free (str_caps);

	gst_structure_get_int (structure, "rate", &bit_rate);
	gst_structure_get_int (structure, "channels", &channels);

	GOO_TI_NBAMRDEC_GET_INPUT_PORT_PARAM (component)->nChannels = channels;
	GOO_TI_NBAMRDEC_GET_INPUT_PORT_PARAM (component)->nBitRate = bit_rate;

	return gst_pad_set_caps (pad, caps);
}

static gboolean
gst_goo_decnbamr_src_setcaps (GstPad *pad, GstCaps *caps)
{
	GstStructure *structure;
	GstGooDecNbAmr *self;
	GooPort *outport;
	guint width = DEFAULT_WIDTH;
	guint depth = DEFAULT_DEPTH;
	gchar *stc_caps;

	self= GST_GOO_DECNBAMR (GST_PAD_PARENT (pad));
	outport = (GooPort *) GST_GOO_AUDIO_FILTER (self)->outport;

	GST_DEBUG_OBJECT (self, "src_setcaps");
	GST_LOG ("caps are %" GST_PTR_FORMAT, caps);
	g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE);

	structure = gst_caps_get_structure (caps, 0);

	gst_structure_get_int (structure, "width", &width);
	gst_structure_get_int (structure, "depth", &depth);

	return gst_pad_set_caps (pad, caps);
}

