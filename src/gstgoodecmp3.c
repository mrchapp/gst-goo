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

#include <goo-component.h>
#include <goo-ti-mp3dec.h>

#include <string.h>

#include "gstgoodecmp3.h"

GST_BOILERPLATE (GstGooDecMp3, gst_goo_decmp3, GstGooAudioFilter, GST_TYPE_GOO_AUDIO_FILTER);

GST_DEBUG_CATEGORY_STATIC (gst_goo_decmp3_debug);
#define GST_CAT_DEFAULT gst_goo_decmp3_debug

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

static gboolean gst_goo_decmp3_src_setcaps (GstPad *pad, GstCaps *caps);
static gboolean gst_goo_decmp3_sink_setcaps (GstPad *pad, GstCaps *caps);

#define GST_GOO_DECMP3_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_GOO_DECMP3, GstGooDecMp3Private))

struct _GstGooDecMp3Private
{
	guint incount;
	guint outcount;
};

/* default values */
#define BITPERSAMPLE_DEFAULT 16
#define CHANNELS_DEFAULT 2
#define SAMPLERATE_DEFAULT 44100
#define INPUT_BUFFERSIZE_DEFAULT 2304
/*#define INPUT_BUFFERSIZE_DEFAULT 4 * 1024*/
#define OUTPUT_BUFFERSIZE_DEFAULT 36864
/*#define OUTPUT_BUFFERSIZE_DEFAULT 4 * 1024*/
#define NUM_INPUT_BUFFERS_DEFAULT 2
/*#define NUM_INPUT_BUFFERS_DEFAULT 1*/
#define NUM_OUTPUT_BUFFERS_DEFAULT 2
/*#define NUM_OUTPUT_BUFFERS_DEFAULT 1*/
#define DEFAULT_WIDTH 16
#define DEFAULT_DEPTH 16
#define LAYER_DEFAULT 3

static const GstElementDetails details =
        GST_ELEMENT_DETAILS (
                "OpenMAX MP3 decoder",
                "Codec/Decoder/Audio",
                "Decodes MP3 streams with OpenMAX",
                "Texas Instrument"
                );

static GstStaticPadTemplate src_factory =
        GST_STATIC_PAD_TEMPLATE (
                "src",
                GST_PAD_SRC,
                GST_PAD_ALWAYS,
                GST_STATIC_CAPS ("audio/x-raw-int, "
                                 "signed = (boolean) true, "
                                 "width = (int) [ 8, 32 ], "
                                 "depth = (int) [ 8, 32 ], "
                                 "rate = (int) { 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 56000, 64000, 80000, 96000 },"
                                 "channels = (int) [ 1, 8 ]")
                );

/* FIXME: make three caps, for mpegversion 1, 2 and 2.5 */
static GstStaticPadTemplate sink_factory =
        GST_STATIC_PAD_TEMPLATE (
                "sink",
                GST_PAD_SINK,
                GST_PAD_ALWAYS,
                GST_STATIC_CAPS ("audio/mpeg, "
                                 "mpegversion = (int) 1, "
                                 "layer = (int) [ 1, 3 ], "
                                 "rate = (int) { 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 56000, 64000, 80000, 96000 }, "
                                 "channels = (int) [ 1, 2 ]")
                );

/* mp3 audio frame header parser from mencoder */

static gint tabsel_123[2][3][16] =
{
	{
		{ 0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,0 },
		{ 0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,0 },
		{ 0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,0 }
	},
	{
		{ 0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,0 },
		{ 0, 8,16,24,32,40,48, 56, 64, 80, 96,112,128,144,160,0 },
		{ 0, 8,16,24,32,40,48, 56, 64, 80, 96,112,128,144,160,0 }
	}
};

static glong freqs[9] =
{
	44100, 48000, 32000, 22050, 24000, 16000 , 11025 , 12000 , 8000
};


static gboolean
gst_goo_decmp3_header_check (GstGooDecMp3* self, guchar* buffer)
{
	gboolean mpeg25;
	gint stereo, ssize, lsf, crc, framesize, bitrate;
	gint padding, bitrate_index, sampling_frequency;

	OMX_AUDIO_PARAM_PCMMODETYPE* param =
		GOO_TI_MP3DEC_GET_OUTPUT_PARAM (GST_GOO_AUDIO_FILTER (self)->component);

	gulong newhead = buffer[0] << 24 |
		buffer[1] << 16 |
		buffer[2] <<  8 |
		buffer[3];

	if ((newhead & 0xffe00000) != 0xffe00000 ||
	    (newhead & 0x0000fc00) == 0x0000fc00)
	{
		GST_WARNING ("failed to recognize header!");
		return FALSE;
	}

	if ((4 - ((newhead >> 17) & 3)) != 3)
	{
		GST_WARNING ("stream not in layer-3");
		return FALSE;
	}

	if (newhead & ((long)1 << 20))
	{
		lsf = (newhead & ((long)1<<19)) ? 0x0 : 0x1;
		mpeg25 = FALSE;
	}
	else
	{
		lsf = 0x1;
		mpeg25 = TRUE;
	}

	if (mpeg25)
	{
		sampling_frequency = 6 + ((newhead >> 10) & 0x3);
	}
	else
	{
		sampling_frequency = ((newhead >> 10) & 0x3) + (lsf * 3);
	}

	if (sampling_frequency > 8)
	{
		GST_WARNING ("invalid sampling frequency");
		return FALSE; /* valid 0..8 */
	}

	crc = ((newhead >> 16) & 0x1) ^ 0x1;
	bitrate_index = ((newhead >> 12) & 0xf);
	padding = ((newhead >> 9) & 0x1);
/* 	fr->extension = ((newhead>>8)&0x1); */
/* 	fr->mode      = ((newhead>>6)&0x3); */
/* 	fr->mode_ext  = ((newhead>>4)&0x3); */
/* 	fr->copyright = ((newhead>>3)&0x1); */
/* 	fr->original  = ((newhead>>2)&0x1); */
/* 	fr->emphasis  = newhead & 0x3; */
	stereo = ((((newhead >> 6) & 0x3)) == 3) ? 1 : 2;

	if (!bitrate_index)
	{
		GST_WARNING ("free format not supported");
		return FALSE;
	}

	if (lsf)
	{
		ssize = (stereo == 1) ? 9 : 17;
	}
	else
	{
		ssize = (stereo == 1) ? 17 : 32;
	}

	if (crc)
	{
		ssize += 2;
	}

	framesize = tabsel_123[lsf][2][bitrate_index] * 144000;

	/* it's no necessary to know the bitrate in OMX */
	bitrate = tabsel_123[lsf][2][bitrate_index];

	if (!framesize)
	{
		GST_WARNING ("invalid framesize/bitrate_index");
		return FALSE; /* valid 1..14 */
	}

	framesize /= freqs[sampling_frequency]<<lsf;
	framesize += padding;

/* 	if (framesize <= 0 || framesize > MAXFRAMESIZE) */
/* 	{ */
/* 		return FALSE; */
/* 	} */

	param->nSamplingRate = freqs[sampling_frequency];
	GOO_TI_MP3DEC_GET_INPUT_PARAM (
		GST_GOO_AUDIO_FILTER (self)->component)->nSampleRate =
			freqs[sampling_frequency];
	param->nChannels = stereo;

	GST_INFO ("nSamplingRate = %d", param->nSamplingRate);
	GST_INFO ("nChannels = %d", param->nChannels);

	return TRUE;
}

static GstStateChangeReturn
gst_goo_decmp3_change_state (GstElement* element, GstStateChange transition)
{
	GST_LOG ("");

	GstGooAudioFilter* self = GST_GOO_AUDIO_FILTER (element);
	GstStateChangeReturn result;

	g_assert (self->component != NULL);
	g_assert (self->inport != NULL);
	g_assert (self->outport != NULL);

	switch (transition)
	{
	case GST_STATE_CHANGE_NULL_TO_READY:
		break;
	case GST_STATE_CHANGE_READY_TO_PAUSED:
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
gst_goo_decmp3_set_property (GObject* object, guint prop_id,
			     const GValue* value, GParamSpec* pspec)
{
	GstGooDecMp3Private* priv = GST_GOO_DECMP3_GET_PRIVATE (object);

	switch (prop_id)
	{
	default:
		break;
	}

	return;
}

static void
gst_goo_decmp3_get_property (GObject* object, guint prop_id,
			     GValue* value, GParamSpec* pspec)
{
	GstGooDecMp3Private* priv = GST_GOO_DECMP3_GET_PRIVATE (object);

	switch (prop_id)
	{
	default:
		break;
	}

	return;
}

static void
gst_goo_decmp3_base_init (gpointer g_klass)
{
        GST_DEBUG_CATEGORY_INIT (gst_goo_decmp3_debug, "goodecmp3", 0,
                                 "OpenMAX MP3 decoder element");

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

static GstBuffer*
gst_goo_decmp3_codec_data_processing (GstGooAudioFilter * filter, GstBuffer *buffer)
{
	GstGooDecMp3 *self = GST_GOO_DECMP3 (filter);

	gst_goo_decmp3_header_check (self, GST_BUFFER_DATA(buffer));

	return buffer;
}
static gboolean
gst_goo_decmp3_check_fixed_src_caps (GstGooAudioFilter *filter)
{
	GstGooDecMp3 *self = GST_GOO_DECMP3 (filter);
	OMX_AUDIO_PARAM_PCMMODETYPE *param;
	GstCaps *caps;

	param = GOO_TI_MP3DEC_GET_OUTPUT_PARAM (GST_GOO_AUDIO_FILTER (self)->component);

	caps = gst_caps_new_simple (
						"audio/x-raw-int",
						"endianness", G_TYPE_INT, G_BYTE_ORDER,
						"signed", G_TYPE_BOOLEAN, TRUE,
						"width", G_TYPE_INT, 16,
						"depth", G_TYPE_INT, 16,
						"rate", G_TYPE_INT, param->nSamplingRate,
						"channels", G_TYPE_INT, param->nChannels,
						NULL);

	gst_pad_set_caps (GST_GOO_AUDIO_FILTER (self)->srcpad, caps);
	gst_caps_unref (caps);

	return TRUE;

}

static gboolean
gst_goo_decmp3_process_mode_default (GstGooAudioFilter *self, guint value)
{
	GooComponent* component = GST_GOO_AUDIO_FILTER (self)->component;
	gboolean frame_mode = value ? TRUE : FALSE;

	GST_DEBUG_OBJECT (self, "Set frame_mode to: %d", frame_mode);



	g_object_set (G_OBJECT (component),"frame-mode", value ? TRUE: FALSE,
					  NULL);
}

static void
gst_goo_decmp3_component_wait_for_done (GooComponent* self)
{
	g_assert (GOO_IS_COMPONENT (self));
	if(self->cur_state == OMX_StateExecuting)
	{
		while (!goo_component_is_done (self))
		{
			goo_semaphore_down (self->done_sem, FALSE);
		}
	}
	else
		g_assert(self->cur_state == OMX_StateIdle);

	return;
}

static void
gst_goo_decmp3_wait_for_done (GstGooAudioFilter* self)
{
	g_assert (self != NULL);

	/* flushing the last buffers in adapter */

	OMX_BUFFERHEADERTYPE* omx_buffer;
	OMX_PARAM_PORTDEFINITIONTYPE* param =
		GOO_PORT_GET_DEFINITION (self->inport);
	GstGooAdapter* adapter = self->adapter;
	int omxbufsiz = param->nBufferSize;
	int avail = gst_goo_adapter_available (adapter);

	if (avail < omxbufsiz && avail > 0)
	{
		GST_INFO ("Marking EOS buffer");
		omx_buffer = goo_port_grab_buffer (self->inport);
		GST_DEBUG ("Peek to buffer %d bytes", avail);
		gst_goo_adapter_peek (adapter, avail, omx_buffer);
		omx_buffer->nFilledLen = avail;
		/* let's send the EOS flag right now */
		omx_buffer->nFlags = OMX_BUFFERFLAG_EOS;
		/** @todo timestamp the buffers */
		goo_component_release_buffer (self->component, omx_buffer);
	}
	else if (avail == 0)
	{
		GST_DEBUG ("Sending empty buffer with EOS flag in it");
		goo_component_send_eos (self->component);
	}
	else
	{
		/* For some reason the adapter didn't extract all
		   possible buffers */
		GST_ERROR ("Adapter algorithm error!");
		goo_component_send_eos (self->component);
	}

	gst_goo_adapter_clear (adapter);

	GST_INFO ("Waiting for done signal");
	gst_goo_decmp3_component_wait_for_done (self->component);

	return;
}

static gboolean
gst_goo_decmp3_sink_event (GstPad* pad, GstEvent* event)
{
	GST_LOG ("");

	GstGooAudioFilter* self = GST_GOO_AUDIO_FILTER (gst_pad_get_parent (pad));

	gboolean ret;

	g_assert (self->component != NULL);

	switch (GST_EVENT_TYPE (event))
	{
	case GST_EVENT_EOS:
		GST_INFO ("EOS event");
		gst_goo_decmp3_wait_for_done (self);
		ret = gst_pad_push_event (self->srcpad, event);
		break;
	default:
		ret = gst_pad_event_default (pad, event);
		break;
	}

	gst_object_unref (self);
	return ret;
}

static void
gst_goo_decmp3_class_init (GstGooDecMp3Class* klass)
{
	GObjectClass* g_klass = G_OBJECT_CLASS (klass);
	GstElementClass* gst_klass;

	/* gobject */

	g_type_class_add_private (klass, sizeof (GstGooDecMp3Private));

	g_klass->set_property =
		GST_DEBUG_FUNCPTR (gst_goo_decmp3_set_property);
	g_klass->get_property =
		GST_DEBUG_FUNCPTR (gst_goo_decmp3_get_property);

	/* GST GOO FILTER overrides */
	GstGooAudioFilterClass *gst_c_klass = GST_GOO_AUDIO_FILTER_CLASS (klass);
	gst_c_klass->codec_data_processing_func = GST_DEBUG_FUNCPTR (gst_goo_decmp3_codec_data_processing);
	gst_c_klass->check_fixed_src_caps_func = GST_DEBUG_FUNCPTR (gst_goo_decmp3_check_fixed_src_caps);
	gst_c_klass->set_process_mode_func = GST_DEBUG_FUNCPTR (gst_goo_decmp3_process_mode_default);

	gst_klass = GST_ELEMENT_CLASS (klass);

	/* Workaround OMAPS00141041 & OMAPS00141042 */
	gst_klass->change_state = GST_DEBUG_FUNCPTR (gst_goo_decmp3_change_state);

	return;
}

static void
gst_goo_decmp3_init (GstGooDecMp3* self, GstGooDecMp3Class* klass)
{
	GST_DEBUG ("");

	GST_GOO_AUDIO_FILTER (self)->component = goo_component_factory_get_component
		(GST_GOO_AUDIO_FILTER (self)->factory, GOO_TI_MP3_DECODER);

        GooComponent* component = GST_GOO_AUDIO_FILTER (self)->component;

        /* deactivate dasf-mode by default */
		g_object_set (G_OBJECT (component), "dasf-mode", FALSE,
					  "frame-mode", FALSE,
					  NULL);

        /* component default parameters */
        {
			GOO_TI_MP3DEC_GET_OUTPUT_PARAM (component)->nBitPerSample =
				BITPERSAMPLE_DEFAULT;
			GOO_TI_MP3DEC_GET_OUTPUT_PARAM (component)->nChannels =
				CHANNELS_DEFAULT;
			GOO_TI_MP3DEC_GET_OUTPUT_PARAM (component)->nSamplingRate =
				SAMPLERATE_DEFAULT;
			GOO_TI_MP3DEC_GET_INPUT_PARAM (component)->nSampleRate =
				SAMPLERATE_DEFAULT;

        }

        /* input port */
        GST_GOO_AUDIO_FILTER (self)->inport = goo_component_get_port (component, "input0");
        g_assert (GST_GOO_AUDIO_FILTER (self)->inport != NULL);

        {
                GooPort* port = GST_GOO_AUDIO_FILTER (self)->inport;
                GOO_PORT_GET_DEFINITION (port)->nBufferSize =
                        INPUT_BUFFERSIZE_DEFAULT;
                GOO_PORT_GET_DEFINITION (port)->format.audio.eEncoding =
                        OMX_AUDIO_CodingMP3;
        }

        GST_GOO_AUDIO_FILTER (self)->outport = goo_component_get_port (component, "output0");
        g_assert (GST_GOO_AUDIO_FILTER (self)->outport != NULL);

        /* output port */
        {
                GooPort* port = GST_GOO_AUDIO_FILTER (self)->outport;
                GOO_PORT_GET_DEFINITION (port)->nBufferSize =
                        OUTPUT_BUFFERSIZE_DEFAULT;
                GOO_PORT_GET_DEFINITION (port)->format.audio.eEncoding =
                        OMX_AUDIO_CodingMP3;
				/** Use the PARENT's callback function */
                goo_port_set_process_buffer_function
                        (port, gst_goo_audio_filter_outport_buffer);
        }

	GstGooDecMp3Private* priv = GST_GOO_DECMP3_GET_PRIVATE (self);
	priv->incount = 0;
	priv->outcount = 0;

	/** Setcaps functions **/

	gst_pad_set_setcaps_function (GST_GOO_AUDIO_FILTER (self)->sinkpad, gst_goo_decmp3_sink_setcaps);
	gst_pad_set_setcaps_function (GST_GOO_AUDIO_FILTER (self)->srcpad, gst_goo_decmp3_src_setcaps);
	gst_pad_set_event_function (GST_GOO_AUDIO_FILTER(self)->sinkpad, gst_goo_decmp3_sink_event);

	g_object_set_data (G_OBJECT (GST_GOO_AUDIO_FILTER (self)->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", GST_GOO_AUDIO_FILTER (self)->component);

        return;
}

static gboolean
gst_goo_decmp3_sink_setcaps (GstPad *pad, GstCaps *caps)
{
	GstStructure *structure;
	GstGooDecMp3 *self;
	GooComponent *component;
	gchar *str_caps;
	guint sample_rate = SAMPLERATE_DEFAULT;
	guint channels = CHANNELS_DEFAULT;
	guint layer = LAYER_DEFAULT;
	guint factor;

	self = GST_GOO_DECMP3 (GST_PAD_PARENT (pad));
	component = (GooComponent *) GST_GOO_AUDIO_FILTER (self)->component;

	GST_DEBUG_OBJECT (self, "sink_setcaps");

	g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE );

	structure = gst_caps_get_structure (caps, 0);
	str_caps = gst_structure_to_string (structure);
	GST_DEBUG_OBJECT (self, "sink caps: %s", str_caps);
	g_free (str_caps);

	gst_structure_get_int (structure, "rate", &sample_rate);
	gst_structure_get_int (structure, "channels", &channels);
	gst_structure_get_int (structure, "layer", &layer);
	
	factor = 0.02 * channels * sample_rate;
	GST_GOO_AUDIO_FILTER (self)->duration = 
		gst_util_uint64_scale_int (GST_SECOND, factor, 
			sample_rate * channels);

	g_object_set(component, "layer", layer, NULL);
	GOO_TI_MP3DEC_GET_OUTPUT_PARAM (component)->nChannels = channels;
	GOO_TI_MP3DEC_GET_INPUT_PARAM (component)->nChannels = channels;
	GOO_TI_MP3DEC_GET_OUTPUT_PARAM (component)->nSamplingRate = sample_rate;
	GOO_TI_MP3DEC_GET_INPUT_PARAM (component)->nSampleRate = sample_rate;

	return gst_pad_set_caps (pad, caps);

}

static gboolean
gst_goo_decmp3_src_setcaps (GstPad *pad, GstCaps *caps)
{
	GstStructure *structure;
	GstGooDecMp3 *self;
	GstElement* next_element;
	GooPort *outport;
	GstPad* peer;
	guint width = DEFAULT_WIDTH;
	guint depth = DEFAULT_DEPTH;
	gchar* str_peer;
	gchar dasf[] = "dasf";
	gint comp_res;

	self = GST_GOO_DECMP3 (GST_PAD_PARENT (pad));
	outport = (GooPort *) GST_GOO_AUDIO_FILTER (self)->outport;
	peer = gst_pad_get_peer (pad);
	next_element = GST_ELEMENT (gst_pad_get_parent (peer));
	str_peer = gst_element_get_name (next_element);
	comp_res = strncmp (dasf, str_peer, 4);
    
	if (comp_res == 0)
	{
		GST_DEBUG_OBJECT (self, "DASF-SINK Activated: MP3 Dec");
		GST_GOO_AUDIO_FILTER (self)->dasf_mode = TRUE;
	}
	else
	{
		GST_DEBUG_OBJECT (self, "FILE-TO-FILE Activated: MP3 Dec");
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
