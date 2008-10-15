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

#include <goo-ti-wbamrdec.h>
#include "gstgoodecwbamr.h"

GST_BOILERPLATE(GstGooDecWbAmr, gst_goo_decwbamr, GstGooAudioFilter,
        GST_TYPE_GOO_AUDIO_FILTER);

GST_DEBUG_CATEGORY_STATIC(gst_goo_decwbamr_debug);
#define GST_CAT_DEFAULT gst_goo_decwbamr_debug

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
    PROP_DTX_MODE
};

static GType
gst_goo_decnbamr_dtxmode_get_type ()
{
	static GType gst_goo_decnbamr_dtxmode_type = 0;
	static GEnumValue gst_goo_decnbamr_dtxmode[] =
	{
		{OMX_AUDIO_AMRDTXModeOff,	 "0", "DTX off"},
		{OMX_AUDIO_AMRDTXModeOnVAD1, "1", "Use Type 1 VAD"},
		{0, NULL, NULL},
	};

	if (!gst_goo_decnbamr_dtxmode_type)
	{
		gst_goo_decnbamr_dtxmode_type = g_enum_register_static ("GstGooNbAmrDecDtxMode", gst_goo_decnbamr_dtxmode);
	}

	return gst_goo_decnbamr_dtxmode_type;
}
#define GST_GOO_DECNBAMR_DTXMODE_TYPE (gst_goo_decnbamr_dtxmode_get_type())

static gboolean gst_goo_decwbamr_src_setcaps(GstPad *pad, GstCaps *caps);
static gboolean gst_goo_decwbamr_sink_setcaps(GstPad *pad, GstCaps *caps);

#define GST_GOO_DECWBAMR_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_GOO_DECWBAMR, GstGooDecWbAmrPrivate))

struct _GstGooDecWbAmrPrivate
{
        guint incount;
        guint outcount;
};

/* default values */
#define CHANNELS_DEFAULT 1
#define INPUT_BUFFERSIZE 116
#define OUTPUT_BUFFERSIZE 640
#define DEFAULT_DTX_MODE OMX_AUDIO_AMRDTXModeOff
#define DEFAULT_FRAME_FORMAT OMX_AUDIO_AMRFrameFormatFSF
#define DEFAULT_BITRATE 8000
#define DEFAULT_MIME_MODE TRUE
#define NUM_INPUT_BUFFERS_DEFAULT 1
#define NUM_OUTPUT_BUFFERS_DEFAULT 1
#define DEFAULT_WBAMR_BANDMODE 1
#define DEFAULT_WIDTH 16
#define DEFAULT_DEPTH 16

static const GstElementDetails details =
            GST_ELEMENT_DETAILS (
                    "OpenMAX WideBand AMR decoder",
                    "Codec/Decoder/Audio",
                    "Decodes WideBand AMR streams with OpenMAX",
                    "Texas Instruments");

static const GstStaticPadTemplate src_factory =GST_STATIC_PAD_TEMPLATE (
        "src",
        GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS (
                "audio/x-raw-int,"
                "rate = (int) [8000, 96000], "
                "channels = (int) [1, 2], "
                "width = (int) 16, "
                "depth = (int) 16 "));

static const GstStaticPadTemplate sink_factory =GST_STATIC_PAD_TEMPLATE (
        "sink",
        GST_PAD_SINK,
        GST_PAD_ALWAYS, GST_STATIC_CAPS (
                "audio/AMR-WB, "
                "rate = (int) 16000, "
                "channels = (int) 1 "));

static gboolean _goo_ti_wbamrdec_set_mime(GstGooDecWbAmr* self,
        gboolean mime)
{
    GST_DEBUG ("goo_ti_wbamrdec_set_mime %d", mime);
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
            GOO_PORT_GET_DEFINITION (port)->format.audio.cMIMEType = "NONMIME";
            GOO_TI_WBAMRDEC_GET_INPUT_PORT_PARAM (component)->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatConformance;
			GOO_TI_WBAMRDEC_GET_OUTPUT_PORT_PARAM (component)->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatConformance;
            break;
        case TRUE:
            GOO_PORT_GET_DEFINITION (port)->format.audio.cMIMEType = "MIME";
            GOO_TI_WBAMRDEC_GET_INPUT_PORT_PARAM (component)->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;
			GOO_TI_WBAMRDEC_GET_OUTPUT_PORT_PARAM (component)->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;
			GST_GOO_AUDIO_FILTER (self)->wbamr_mime = TRUE;
            break;
        default:
			GOO_PORT_GET_DEFINITION (port)->format.audio.cMIMEType = "MIME";
            GOO_TI_WBAMRDEC_GET_INPUT_PORT_PARAM (component)->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;
			GOO_TI_WBAMRDEC_GET_OUTPUT_PORT_PARAM (component)->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;
			GST_GOO_AUDIO_FILTER (self)->wbamr_mime = TRUE;
            break;
    }

    g_object_unref (iter);
    g_object_unref (port);

    return retval;
}

static gboolean _goo_ti_wbamrdec_set_dtx_mode(GstGooDecWbAmr* self,
        guint dtx_mode)
{
    g_assert (self != NULL);
    GooComponent* component = GST_GOO_AUDIO_FILTER (self)->component;

    gboolean retval = TRUE;

    self->dtx_mode = dtx_mode;

    switch (self->dtx_mode)
    {
        case 0:
            GOO_TI_WBAMRDEC_GET_INPUT_PORT_PARAM (component)->eAMRDTXMode = OMX_AUDIO_AMRDTXModeOff;
            break;
        case 1:
            GOO_TI_WBAMRDEC_GET_INPUT_PORT_PARAM (component)->eAMRDTXMode = OMX_AUDIO_AMRDTXModeOnVAD1;
            break;
        default:
            GOO_TI_WBAMRDEC_GET_INPUT_PORT_PARAM (component)->eAMRDTXMode = OMX_AUDIO_AMRDTXModeOff;
            break;
    }

    return retval;
}

static gboolean gst_goo_decwbamr_process_mode_default(GstGooAudioFilter *self,
        guint value)
{
    GooComponent *component = GST_GOO_AUDIO_FILTER (self)->component;

    GST_DEBUG ("gst_goo_decwbamr_process_mode_default %d", value);
    g_object_set (G_OBJECT (component), "frame-mode", value ? TRUE : FALSE,
            NULL);

    return TRUE;
}

static void gst_goo_decwbamr_set_property(GObject* object, guint prop_id,
        const GValue* value, GParamSpec* pspec)
{
    GST_DEBUG ("gst_goo_decwbamr_set_property %d", prop_id);
    g_assert (GST_IS_GOO_DECWBAMR(object));
    GstGooDecWbAmr* self = GST_GOO_DECWBAMR (object);

    switch (prop_id)
    {
        case PROP_MIME:
            _goo_ti_wbamrdec_set_mime (self, g_value_get_boolean (value));
            break;
        case PROP_DTX_MODE:
            _goo_ti_wbamrdec_set_dtx_mode (self, g_value_get_uint (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }

    return;
}

static void gst_goo_decwbamr_get_property(GObject* object, guint prop_id,
        GValue* value, GParamSpec* pspec)
{
    GST_DEBUG ("gst_goo_decwbamr_get_property");
    g_assert (GST_IS_GOO_DECWBAMR(object));
    GstGooDecWbAmr* self = GST_GOO_DECWBAMR (object);

    switch (prop_id)
    {
        case PROP_MIME:
            g_value_set_boolean (value, self->mime);
            break;
        case PROP_DTX_MODE:
            g_value_set_uint (value, self->dtx_mode);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }

    return;
}

static void gst_goo_decwbamr_dispose(GObject* object)
{
    G_OBJECT_CLASS (parent_class)->dispose(object);

    return;
}

static void gst_goo_decwbamr_base_init(gpointer g_klass)
{
    GST_DEBUG_CATEGORY_INIT (gst_goo_decwbamr_debug, "goodecwbamr", 0,
            "OpenMAX WideBand AMR Coding decoder element");

    GstElementClass* e_klass = GST_ELEMENT_CLASS (g_klass);

    gst_element_class_add_pad_template (e_klass,
            gst_static_pad_template_get((GstStaticPadTemplate *)&sink_factory));

    gst_element_class_add_pad_template (e_klass,
            gst_static_pad_template_get((GstStaticPadTemplate *)&src_factory));

    gst_element_class_set_details (e_klass, &details);

    return;
}

static gboolean gst_goo_decwbamr_check_fixed_src_caps(GstGooAudioFilter *filter)
{
    GstGooDecWbAmr* self = GST_GOO_DECWBAMR (filter);
    OMX_AUDIO_PARAM_AMRTYPE *param;
    GstCaps *src_caps;

    param = GOO_TI_WBAMRDEC_GET_OUTPUT_PORT_PARAM (GST_GOO_AUDIO_FILTER (self)->component);

    src_caps = gst_caps_new_simple (
            "audio/x-raw-int",
            "width", G_TYPE_INT, 16,
            "depth", G_TYPE_INT, 16,
            "rate", G_TYPE_INT, param->nBitRate,
            "channels", G_TYPE_INT, param->nChannels,
            NULL);

    gst_pad_set_caps (GST_GOO_AUDIO_FILTER (self)->srcpad, src_caps);
    gst_caps_unref (src_caps);

    return TRUE;
}

static void gst_goo_decwbamr_class_init(GstGooDecWbAmrClass* klass)
{
    GObjectClass* g_klass = G_OBJECT_CLASS (klass);
    GParamSpec* pspec;
    GstElementClass* gst_klass;

    /* gobject */
    g_type_class_add_private (klass, sizeof(GstGooDecWbAmrPrivate));

    g_klass->set_property = GST_DEBUG_FUNCPTR (gst_goo_decwbamr_set_property);
    g_klass->get_property = GST_DEBUG_FUNCPTR (gst_goo_decwbamr_get_property);
    g_klass->dispose = GST_DEBUG_FUNCPTR (gst_goo_decwbamr_dispose);

    GParamSpec* spec;

    spec = g_param_spec_boolean ("mime", "MIME", "Enable using MIME headers",
    		DEFAULT_MIME_MODE, G_PARAM_READWRITE);
    g_object_class_install_property (g_klass, PROP_MIME, spec);

    spec = g_param_spec_uint ("dtx-mode", "DTX Mode",
            "Specifies the decoding DTX settings", 0, 1, DEFAULT_DTX_MODE,
            G_PARAM_READWRITE);
    g_object_class_install_property (g_klass, PROP_DTX_MODE, spec);

    /* GST GOO FILTER */
    GstGooAudioFilterClass* gst_c_klass = GST_GOO_AUDIO_FILTER_CLASS (klass);
    gst_c_klass->check_fixed_src_caps_func
            = GST_DEBUG_FUNCPTR (gst_goo_decwbamr_check_fixed_src_caps);
    gst_c_klass->set_process_mode_func
            = GST_DEBUG_FUNCPTR (gst_goo_decwbamr_process_mode_default);

    return;
}

static void gst_goo_decwbamr_init(GstGooDecWbAmr *self,
        GstGooDecWbAmrClass *klass)
{
    GST_DEBUG ("gst_goo_decwbamr_init");

    GST_GOO_AUDIO_FILTER (self)->component = goo_component_factory_get_component (
            GST_GOO_AUDIO_FILTER (self)->factory, GOO_TI_WBAMR_DECODER);

    GooComponent* component = GST_GOO_AUDIO_FILTER (self)->component;

    /* component default parameters */
    {
        /* input port */
        GOO_TI_WBAMRDEC_GET_INPUT_PORT_PARAM (component)->nChannels = CHANNELS_DEFAULT;
        GOO_TI_WBAMRDEC_GET_INPUT_PORT_PARAM (component)->nBitRate = DEFAULT_BITRATE;
        GOO_TI_WBAMRDEC_GET_INPUT_PORT_PARAM (component)->eAMRBandMode = DEFAULT_WBAMR_BANDMODE;
        GOO_TI_WBAMRDEC_GET_INPUT_PORT_PARAM (component)->eAMRDTXMode = DEFAULT_DTX_MODE;
        GOO_TI_WBAMRDEC_GET_INPUT_PORT_PARAM (component)->eAMRFrameFormat = DEFAULT_FRAME_FORMAT;

        /* output port */
        GOO_TI_WBAMRDEC_GET_OUTPUT_PORT_PARAM (component)->nChannels = CHANNELS_DEFAULT;
        GOO_TI_WBAMRDEC_GET_OUTPUT_PORT_PARAM (component)->nBitRate = DEFAULT_BITRATE;
    }

    /* input port */
    GST_GOO_AUDIO_FILTER (self)->inport = goo_component_get_port (component, "input0");
    g_assert (GST_GOO_AUDIO_FILTER (self)->inport != NULL);

    {
        GooPort* port = GST_GOO_AUDIO_FILTER (self)->inport;
        GOO_PORT_GET_DEFINITION (port)->nBufferCountActual = NUM_INPUT_BUFFERS_DEFAULT;
        GOO_PORT_GET_DEFINITION (port)->nBufferSize = INPUT_BUFFERSIZE;
        GOO_PORT_GET_DEFINITION (port)->eDomain = OMX_PortDomainAudio;
        GOO_PORT_GET_DEFINITION (port)->format.audio.eEncoding = OMX_AUDIO_CodingAMR;
    }

    /* output port */
    GST_GOO_AUDIO_FILTER (self)->outport = goo_component_get_port (component, "output0");
    g_assert (GST_GOO_AUDIO_FILTER (self)->outport != NULL);

    {
        GooPort* port = GST_GOO_AUDIO_FILTER (self)->outport;

        GOO_PORT_GET_DEFINITION (port)->nBufferCountActual = NUM_OUTPUT_BUFFERS_DEFAULT;
        GOO_PORT_GET_DEFINITION (port)->nBufferSize = OUTPUT_BUFFERSIZE;
        GOO_PORT_GET_DEFINITION (port)->eDomain = OMX_PortDomainAudio;

        /* USE THE PARENT'S CALLBACK FUNCTION */
        goo_port_set_process_buffer_function (port, gst_goo_audio_filter_outport_buffer);
    }

    g_object_set(self, "mime", 1, NULL);

    GstGooDecWbAmrPrivate* priv = GST_GOO_DECWBAMR_GET_PRIVATE (self);
    priv->incount = 0;
    priv->outcount = 0;

    /* Setcaps functions */
    gst_pad_set_setcaps_function (GST_GOO_AUDIO_FILTER (self)->sinkpad, gst_goo_decwbamr_sink_setcaps);
    gst_pad_set_setcaps_function (GST_GOO_AUDIO_FILTER (self)->srcpad, gst_goo_decwbamr_src_setcaps);

    g_object_set_data (G_OBJECT (GST_GOO_AUDIO_FILTER (self)->component), "gst", self);
    g_object_set_data (G_OBJECT (self), "goo", GST_GOO_AUDIO_FILTER (self)->component);

    return;
}

static gboolean gst_goo_decwbamr_sink_setcaps(GstPad *pad, GstCaps *caps)
{
    GstStructure *structure;
    GstGooDecWbAmr *self;
    GooComponent *component;
    gchar *str_caps;
    guint bit_rate= DEFAULT_BITRATE;
    guint channels= CHANNELS_DEFAULT;
    guint width= DEFAULT_WIDTH;
    guint depth= DEFAULT_DEPTH;

    self = GST_GOO_DECWBAMR (GST_PAD_PARENT (pad));
    component = (GooComponent *) GST_GOO_AUDIO_FILTER (self)->component;

    GST_DEBUG_OBJECT (self, "sink_setcaps");

    g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE);

    structure = gst_caps_get_structure (caps, 0);
    str_caps = gst_structure_to_string (structure);
    GST_DEBUG_OBJECT (self, "sink caps: %s", str_caps);
    g_free (str_caps);

    gst_structure_get_int (structure, "rate", &bit_rate);
    gst_structure_get_int (structure, "channels", &channels);

    GOO_TI_WBAMRDEC_GET_INPUT_PORT_PARAM (component)->nChannels = channels;
    GOO_TI_WBAMRDEC_GET_INPUT_PORT_PARAM (component)->nBitRate = bit_rate;

    return gst_pad_set_caps (pad, caps);
}

static gboolean gst_goo_decwbamr_src_setcaps(GstPad *pad, GstCaps *caps)
{
    GstStructure *structure;
    GstGooDecWbAmr *self;
    GooPort *outport;
    guint width= DEFAULT_WIDTH;
    guint depth= DEFAULT_DEPTH;
    gchar *stc_caps;

    self= GST_GOO_DECWBAMR (GST_PAD_PARENT (pad));
    outport = (GooPort *) GST_GOO_AUDIO_FILTER (self)->outport;

    GST_DEBUG_OBJECT (self, "src_setcaps");
    GST_LOG ("caps are %" GST_PTR_FORMAT, caps);
    g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE);

    structure = gst_caps_get_structure (caps, 0);

    gst_structure_get_int (structure, "width", &width);
    gst_structure_get_int (structure, "depth", &depth);

    return gst_pad_set_caps (pad, caps);
}

