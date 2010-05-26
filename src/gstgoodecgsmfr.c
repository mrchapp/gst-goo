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

#include <goo-ti-gsmfrdec.h>
#include "gstgoodecgsmfr.h"

GST_BOILERPLATE (GstGooDecGsmFr, gst_goo_decgsmfr, GstGooAudioFilter, GST_TYPE_GOO_AUDIO_FILTER);

GST_DEBUG_CATEGORY_STATIC (gst_goo_decgsmfr_debug);
#define GST_CAT_DEFAULT gst_goo_decgsmfr_debug

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

static gboolean gst_goo_decgsmfr_sink_setcaps (GstPad *pad, GstCaps *caps);
static gboolean gst_goo_decgsmfr_src_setcaps (GstPad *pad, GstCaps *caps);

#define GST_GOO_DECGSMFR_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_GOO_DECGSMFR, GstGooDecGsmFrPrivate))

struct _GstGooDecGsmFrPrivate
{
    guint incount;
    guint outcount;
};

/* default values */
#define CHANNELS_DEFAULT 1
#define INPUT_BUFFERSIZE 102
#define OUTPUT_BUFFERSIZE 320
#define DEFAULT_BITRATE 8000
#define NUM_INPUT_BUFFERS_DEFAULT 1
#define NUM_OUTPUT_BUFFERS_DEFAULT 1
#define DEFAULT_WIDTH 16
#define DEFAULT_DEPTH 16

static const GstElementDetails details =
        GST_ELEMENT_DETAILS (
                "OpenMAX GSM Full Rate decoder",
                "Codec/Decoder/Audio",
                "Decodes GSM Full Rate streams with OpenMAX",
                "Texas Instruments"
                );

static GstStaticPadTemplate src_factory =
GST_STATIC_PAD_TEMPLATE ("src",
        GST_PAD_SRC,
        GST_PAD_ALWAYS,
        GST_STATIC_CAPS ("audio/x-raw-int, "
                "rate = (int) 8000, "
                "channels = (int) 1, "
                "width = (int) 16, "
                "depth = (int) 16 "
                ));

static GstStaticPadTemplate sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
        GST_PAD_SINK,
        GST_PAD_ALWAYS,
        GST_STATIC_CAPS ("audio/x-adpcm, "
                "rate = (int) 8000, "
                "channels = (int) 1, "
                "width = (int) 16, "
                "depth = (int) 16 "
                ));

static void
gst_goo_decgsmfr_dispose (GObject* object)
{
    G_OBJECT_CLASS (parent_class)->dispose (object);

    return;
}

static void
gst_goo_decgsmfr_base_init (gpointer g_klass)
{
    GST_DEBUG_CATEGORY_INIT (gst_goo_decgsmfr_debug, "goodecgsmfr", 0,
                                "OpenMAX GSMFR decoder element");

    GstElementClass *e_klass = GST_ELEMENT_CLASS (g_klass);

    gst_element_class_add_pad_template (e_klass,
                                        gst_static_pad_template_get (&sink_factory));

    gst_element_class_add_pad_template (e_klass,
                                        gst_static_pad_template_get (&src_factory));

    gst_element_class_set_details (e_klass, &details);

    return;
}

static gboolean
gst_goo_decgsmfr_check_fixed_src_caps (GstGooAudioFilter* filter)
{
    GstGooDecGsmFr *self = GST_GOO_DECGSMFR (filter);
    OMX_AUDIO_PARAM_PCMMODETYPE *param;
    GstCaps *src_caps;

    param = GOO_TI_GSMFRDEC_GET_OUTPUT_PORT_PARAM (GST_GOO_AUDIO_FILTER (self)->component);

    src_caps = gst_caps_new_simple (
                                "audio/x-raw-int",
                                "endianness", G_TYPE_INT, G_BYTE_ORDER,
                                "signed", G_TYPE_BOOLEAN, TRUE,
                                "width", G_TYPE_INT, 16,
                                "depth", G_TYPE_INT, 16,
                                "rate", G_TYPE_INT, 8000,
                                "channels", G_TYPE_INT, 1,
                                NULL
                                );

    filter->src_caps = gst_caps_ref (src_caps);
    gst_pad_set_caps (GST_GOO_AUDIO_FILTER (self)->srcpad, src_caps);
    gst_caps_unref (src_caps);

    return TRUE;
}

static void
gst_goo_decgsmfr_class_init (GstGooDecGsmFrClass *klass)
{
    GObjectClass *g_klass = G_OBJECT_CLASS (klass);
    GstElementClass *gst_klass;

    /* gobject */
    g_klass->dispose = GST_DEBUG_FUNCPTR (gst_goo_decgsmfr_dispose);

    /* GST_GOO_FILTER */
    GstGooAudioFilterClass *gst_c_klass = GST_GOO_AUDIO_FILTER_CLASS (klass);
    gst_c_klass->check_fixed_src_caps_func = GST_DEBUG_FUNCPTR (gst_goo_decgsmfr_check_fixed_src_caps);

    return;
}

static void
gst_goo_decgsmfr_init (GstGooDecGsmFr *self, GstGooDecGsmFrClass *klass)
{
    GST_DEBUG ("Enter");

    GST_GOO_AUDIO_FILTER (self)->component = goo_component_factory_get_component
        (GST_GOO_AUDIO_FILTER (self)->factory, GOO_TI_GSMFR_DECODER);

    GooComponent *component = GST_GOO_AUDIO_FILTER (self)->component;

    /* component default parameters */
    {
        GOO_TI_GSMFRDEC_GET_OUTPUT_PORT_PARAM (component)->nChannels = CHANNELS_DEFAULT;
        GOO_TI_GSMFRDEC_GET_OUTPUT_PORT_PARAM (component)->nSamplingRate = DEFAULT_BITRATE;
    }

    /* input port */
    GST_GOO_AUDIO_FILTER (self)->inport = goo_component_get_port (component, "input0");
    g_assert (GST_GOO_AUDIO_FILTER (self)->inport != NULL);

    {
        GooPort *port = GST_GOO_AUDIO_FILTER (self)->inport;
        GOO_PORT_GET_DEFINITION (port)->nBufferSize = INPUT_BUFFERSIZE;
        GOO_PORT_GET_DEFINITION (port)->format.audio.eEncoding = OMX_AUDIO_CodingGSMFR;
    }

    /* output port */
    GST_GOO_AUDIO_FILTER (self)->outport = goo_component_get_port (component, "output0");
    g_assert (GST_GOO_AUDIO_FILTER (self)->outport != NULL);

    {
        GooPort *port = GST_GOO_AUDIO_FILTER (self)->outport;
        GOO_PORT_GET_DEFINITION (port)->nBufferSize = OUTPUT_BUFFERSIZE;
        GOO_PORT_GET_DEFINITION (port)->format.audio.eEncoding = OMX_AUDIO_CodingPCM;

        goo_port_set_process_buffer_function (port, gst_goo_audio_filter_outport_buffer);
    }

    /* Setcap functions */
    gst_pad_set_setcaps_function (GST_GOO_AUDIO_FILTER (self)->sinkpad, gst_goo_decgsmfr_sink_setcaps);
    gst_pad_set_setcaps_function (GST_GOO_AUDIO_FILTER (self)->srcpad, gst_goo_decgsmfr_src_setcaps);

    g_object_set_data (G_OBJECT (GST_GOO_AUDIO_FILTER (self)->component), "gst", self);
    g_object_set_data (G_OBJECT (self), "goo", GST_GOO_AUDIO_FILTER (self)->component);
    g_object_set (G_OBJECT (self), "process mode", 1, NULL);

    GST_DEBUG ("Exit");

    return;
}

static gboolean
gst_goo_decgsmfr_sink_setcaps (GstPad *pad, GstCaps *caps)
{
    GstStructure *structure;
    GstGooDecGsmFr *self;
    GooComponent *component;
    gchar *str_caps;
    guint sample_rate = DEFAULT_BITRATE;
    guint channels = CHANNELS_DEFAULT;

    self = GST_GOO_DECGSMFR (GST_PAD_PARENT (pad));
    component = (GooComponent *) GST_GOO_AUDIO_FILTER (self)->component;

    GST_DEBUG_OBJECT (self, "sink_setcaps");

    g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE);

    structure = gst_caps_get_structure (caps, 0);
    str_caps = gst_structure_to_string (structure);
    GST_DEBUG_OBJECT (self, "sink caps: %s", str_caps);
    g_free (str_caps);

    if (gst_goo_util_structure_is_parsed (structure))
        g_object_set (G_OBJECT (self), "process-mode", 0, NULL);

    gst_structure_get_int (structure, "rate", &sample_rate);
    gst_structure_get_int (structure, "channels", &channels);

    return gst_pad_set_caps (pad, caps);
}

static gboolean
gst_goo_decgsmfr_src_setcaps (GstPad *pad, GstCaps *caps)
{
    GstStructure *structure;
    GstGooDecGsmFr *self;
    GstElement* next_element;
    GooPort *outport;
    GstPad* peer;
    guint width = DEFAULT_WIDTH;
    guint depth = DEFAULT_DEPTH;
    gchar* str_peer;
    gchar dasf[] = "dasf";
    gint comp_res;

    self = GST_GOO_DECGSMFR (GST_PAD_PARENT (pad));
    outport = (GooPort *) GST_GOO_AUDIO_FILTER (self)->outport;
    peer = gst_pad_get_peer (pad);
    next_element = GST_ELEMENT (gst_pad_get_parent (peer));
    str_peer = gst_element_get_name (next_element);
    comp_res = strncmp (dasf, str_peer, 4);

    if (comp_res == 0)
    {
        GST_DEBUG_OBJECT (self, "DASF-SINK Activated: GSM-FR Dec");
        GST_GOO_AUDIO_FILTER (self)->dasf_mode = TRUE;
    }
    else
    {
        GST_DEBUG_OBJECT (self, "FILE-TO-FILE Activated: GSM-FR Dec");
        GST_GOO_AUDIO_FILTER (self)->dasf_mode = FALSE;
    }

    GST_DEBUG_OBJECT (self, "src_setcaps");
    GST_LOG ("caps are %" GST_PTR_FORMAT, caps);
    g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE);

    structure = gst_caps_get_structure (caps, 0);

    gst_structure_get_int (structure, "width", &width);
    gst_structure_get_int (structure, "depth", &depth);

    gst_object_unref (next_element);
    gst_object_unref (peer);

    return gst_pad_set_caps (pad, caps);
}
