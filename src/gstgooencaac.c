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

#include <goo-ti-aacenc.h>
#include <string.h>
#include <goo-component.h>
#include <goo-ti-audio-manager.h>
#include <goo-ti-clock.h>

#if 1
#include <TIDspOmx.h>
#endif

#include "gstgoobuffer.h"
#include "gstgooencaac.h"
#include "gstgooutils.h"

#define GST_GOO_TYPE_BITRATEMODE \
	(goo_ti_aacenc_bitratemode_get_type ())

#define GST_GOO_TYPE_OUTPUTFORMAT \
	(gst_goo_aacenc_outputformat_type ())

static GType
gst_goo_aacenc_outputformat_type ()
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
	{
		/* the commented values are not supported by OMX */
		static GEnumValue values[] = {
/* 			{ OMX_AUDIO_AACStreamFormatMP2ADTS, */
/* 			  "MP2ADTS", "Audio Data Transport Stream 2" }, */
			{ OMX_AUDIO_AACStreamFormatMP4ADTS,
			  "MP4ADTS", "Audio Data Transport Stream 4" },
/* 			{ OMX_AUDIO_AACStreamFormatMP4LOAS, */
/* 			  "MP4LOAS", "Low Overhead Audio Stream format" }, */
/* 			{ OMX_AUDIO_AACStreamFormatMP4LATM, */
/* 			  "MP4LATM", "Low overhead Audio Transport Multiplex" }, */
			{ OMX_AUDIO_AACStreamFormatADIF,
			  "ADIF", "Audio Data Interchange Format" },
/* 			{ OMX_AUDIO_AACStreamFormatMP4FF, */
/* 			  "MP4FF", "inside MPEG-4/ISO File Format" }, */
			{ OMX_AUDIO_AACStreamFormatRAW,
			  "raw", "Raw Format" },
			{ 0, NULL, NULL }
		};

		type = g_enum_register_static ("GstGooAacEncOutputFormat",
					       values);
	}

	return type;
}

#define GST_GOO_TYPE_PROFILE \
	(gst_goo_aacenc_profile_type ())

static GType
gst_goo_aacenc_profile_type ()
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
	{
		/* the commented values are not supported by OMX */
		static GEnumValue values[] = {
/* 			{ OMX_AUDIO_AACObjectMain, "Main", "Main profile" }, */
			{ OMX_AUDIO_AACObjectLC,
			  "LC", "Low Complexity profile" },
/* 			{ OMX_AUDIO_AACObjectSSR, */
/* 			  "SSR", "Scalable Sample Rate profile" }, */
/* 			{ OMX_AUDIO_AACObjectLTP, */
/* 			  "LTP", "Long Term Prediction profile" }, */
			{ OMX_AUDIO_AACObjectHE,
			  "HE", "High Efficiency profile" },
/* 			{ OMX_AUDIO_AACObjectScalable, */
/* 			  "Scalable", "Scalable profile" }, */
/* 			{ OMX_AUDIO_AACObjectERLC, */
/* 			  "ERLC", "Error Resilient Low Complexity profile" }, */
/* 			{ OMX_AUDIO_AACObjectLD, "LD", "Low Delay profile" }, */
			{ OMX_AUDIO_AACObjectHE_PS,
			  "HEPS", "High Efficiency with Parametric Stereo profile" },
			{ 0, NULL, NULL },
		};
		type = g_enum_register_static ("GstGooAacEncProfile", values);
	}
	return type;
}

/* default values */
#define DEFAULT_CHANNELS     1
#define DEFAULT_CHANNELS_MODE OMX_AUDIO_ChannelModeMono
#define DEFAULT_SAMPLERATE   44100
#define DEFAULT_BITRATE      128000 /* 1024 * 128 ?? */
#define DEFAULT_PROFILE      OMX_AUDIO_AACObjectLC
#define DEFAULT_BITRATEMODE  GOO_TI_AACENC_BR_CBR
#define DEFAULT_OUTPUTFORMAT OMX_AUDIO_AACStreamFormatMP4ADTS
#define DEFAULT_MIMO_MODE FALSE

enum __GstGooEncAacProp
{
	PROP_0,
	PROP_BITRATE,
	PROP_BITRATEMODE,
	PROP_OUTPUTFORMAT,
	PROP_CHANNELS,
	PROP_PROFILE,
	PROP_MIMO_MODE
};

static const GstElementDetails details =
	GST_ELEMENT_DETAILS (
		"OpenMAX AAC encoder",
		"Codec/Decoder/Audio",
		"Encodes Advanced Audio Coding streams with OpenMAX",
		"Texas Instrument"
		);

static GstStaticPadTemplate src_template =
	GST_STATIC_PAD_TEMPLATE (
		"src",
		GST_PAD_SRC,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS ("audio/mpeg, "
				 "mpegversion = (int) 4, "
				 "rate = (int) [ 8000, 96000 ],"
				 "channels = (int) [ 1, 2 ]"
			));

static GstStaticPadTemplate sink_template =
	GST_STATIC_PAD_TEMPLATE (
		"sink",
		GST_PAD_SINK,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS ("audio/x-raw-int, "
				 "endianness = (int) BYTE_ORDER, "
				 "signed = (boolean) true, "
				 "width = (int) 16, "
				 "depth = (int) 16, "
				 "rate = (int) [ 8000, 96000 ], "
				 "channels = (int) [ 1, 2 ] "
			));

GST_DEBUG_CATEGORY_STATIC (gst_goo_encaac_debug);
#define GST_CAT_DEFAULT gst_goo_encaac_debug

#define _do_init(blah)							\
GST_DEBUG_CATEGORY_INIT (gst_goo_encaac_debug, "gooencaac", 0, "OpenMAX Advanced Audio Coding encoder element")

GST_BOILERPLATE_FULL (GstGooEncAac, gst_goo_encaac,
		      GstElement, GST_TYPE_ELEMENT, _do_init);

/* for debugging:
 */
static guint duration_in = 0;
static guint duration_out = 0;

/* Setting up MIMO mixer modes for the component.
 * The default value will be using DASF mixer value
 * unless it is specified by the MIMO mode.
 */

static gboolean
_goo_ti_aacenc_set_mimo_mode (GstGooEncAac* self)
{
    g_assert (self != NULL);
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

    goo_component_set_config_by_name (self->component,
            GOO_TI_AUDIO_COMPONENT (self->component)->datapath_param_name,
            &datapath);

    return retval;
}

static GstFlowReturn
process_output_buffer (GstGooEncAac* self, OMX_BUFFERHEADERTYPE* buffer)
{
	GstBuffer* out = NULL;
	GstFlowReturn ret = GST_FLOW_ERROR;
	guint bytes_pending;
	GstClockTime duration;

	GST_DEBUG_OBJECT (self, "outcount = %d", self->outcount);

	GST_OBJECT_LOCK (self);
	bytes_pending = self->bytes_pending;
	self->bytes_pending = 0;
	GST_OBJECT_UNLOCK (self);

#if 0
	out = gst_goo_buffer_new ();
	gst_goo_buffer_set_data (out, self->component, buffer);
#else
	out = gst_buffer_new_and_alloc (buffer->nFilledLen);
	g_assert (out != NULL);
	memmove (GST_BUFFER_DATA (out),
		 buffer->pBuffer, buffer->nFilledLen);
	goo_component_release_buffer (self->component, buffer);
#endif

	if (out != NULL)
	{
		GST_DEBUG("Compute the duration (constant when Dasf-Mode");
		duration  = (goo_port_is_tunneled (self->inport))? 	self->duration :
																												bytes_pending * self->ns_per_byte;

		GST_BUFFER_DURATION (out) = duration;
		GST_BUFFER_OFFSET (out) = self->outcount++;
		GST_BUFFER_TIMESTAMP (out) = self->ts;
		GST_DEBUG ("audio timestamp: %"GST_TIME_FORMAT,
			   GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (out)));
		if (self->ts != -1)
		{
			self->ts += duration;
		}

		GST_DEBUG ("output buffer=0x%08x (time=%"GST_TIME_FORMAT", duration=%"GST_TIME_FORMAT", flags=%08x)",
				out,
				GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (out)),
				GST_TIME_ARGS (GST_BUFFER_DURATION (out)),
				GST_BUFFER_FLAGS (out)
			);

duration_out += duration;
GST_DEBUG ("duration_out=%"GST_TIME_FORMAT, GST_TIME_ARGS (duration_out));
		GST_DEBUG ("audio timestamp: %"GST_TIME_FORMAT,
			GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (out)));

		gst_buffer_set_caps (out, GST_PAD_CAPS (self->srcpad));

		GST_DEBUG_OBJECT (self, "pushing gst buffer");
		ret = gst_pad_push (self->srcpad, out);
	}

	GST_DEBUG_OBJECT (self, "");

	return ret;
}

static void
omx_output_buffer_cb (GooPort* port,
		      OMX_BUFFERHEADERTYPE* buffer,
		      gpointer data)
{
	g_assert (buffer != NULL);
	g_assert (GOO_IS_PORT (port));
	g_assert (GOO_IS_COMPONENT (data));

	g_return_if_fail (buffer->nFlags != OMX_BUFFERFLAG_DATACORRUPT);

	GooComponent* component = GOO_COMPONENT (data);
	GstGooEncAac* self = GST_GOO_ENCAAC (
		g_object_get_data (G_OBJECT (data), "gst")
		);
	g_assert (self != NULL);

	{
		if (buffer->nFilledLen <= 0)
		{
			GST_INFO_OBJECT (self, "Received an empty buffer!");
			goo_component_release_buffer (self->component, buffer);
		}
		else
		{
			process_output_buffer (self, buffer);
		}

		if (buffer->nFlags & OMX_BUFFERFLAG_EOS == 0x1 ||
		    goo_port_is_eos (port))
		{
			GST_INFO_OBJECT (self,
					 "EOS found in output buffer (%d)",
					 buffer->nFilledLen);
			goo_component_set_done (self->component);
		}
	}

	GST_DEBUG_OBJECT (self, "");

	return;
}

static void
omx_sync (GstGooEncAac* self)
{
	g_assert (self != NULL);
	g_assert (self->component != NULL);

	if (!goo_port_is_tunneled (self->inport))
	{
		goo_port_set_process_buffer_function (self->outport,
						      omx_output_buffer_cb);
	}
	else
	{
		g_object_set (self->component,
			      "bitrate-mode", self->bitratemode, NULL);
	}

	GST_INFO_OBJECT (self, "");

	return;
}

static void
omx_start (GstGooEncAac* self)
{
	g_assert (self != NULL);
	g_assert (self->component != NULL);

	GST_OBJECT_LOCK (self);
	if (goo_component_get_state (self->component) == OMX_StateLoaded)
	{
		omx_sync (self);

		GST_INFO_OBJECT (self, "going to idle");
		goo_component_set_state_idle (self->component);

		if (goo_ti_audio_component_is_dasf_mode (
			GOO_TI_AUDIO_COMPONENT (self->component)))
		{
			GST_INFO ("audio encoder in dasf-mode");
			GST_INFO ("creating a clock");
			self->clock = goo_component_factory_get_component (
				self->factory, GOO_TI_CLOCK);
			g_assert (self->clock);

			GST_INFO ("set the clock and the stream id in goo ti audio component");
			goo_ti_audio_component_set_clock (
				GOO_TI_AUDIO_COMPONENT (self->component),
				self->clock);
			goo_component_set_state_idle (self->clock);
		}
	}

	if (goo_component_get_state (self->component) == OMX_StateIdle)
	{
		GST_INFO_OBJECT (self, "going to executing");
		goo_component_set_state_executing (self->component);

		if (goo_ti_audio_component_is_dasf_mode (GOO_TI_AUDIO_COMPONENT (self->component)))
		{
			goo_component_set_state_executing (self->clock);
			goo_ti_clock_set_starttime (GOO_TI_CLOCK (self->clock), 0);
		}
	}
	GST_OBJECT_UNLOCK (self);

	return;
}

static void
omx_stop (GstGooEncAac* self)
{
	g_assert (self != NULL);
	g_assert (self->component != NULL);

	GST_OBJECT_LOCK (self);
	if (goo_component_get_state (self->component) == OMX_StateExecuting)
	{
		GST_INFO_OBJECT (self, "going to idle");
		goo_component_set_state_idle (self->component);

		if (goo_ti_audio_component_is_dasf_mode (GOO_TI_AUDIO_COMPONENT (self->component)))
		{
			GST_INFO_OBJECT (self, "clock going to idle");
			goo_component_set_state_idle (self->clock);
		}
	}

	if (goo_component_get_state (self->component) == OMX_StateIdle)
	{
		GST_INFO_OBJECT (self, "going to loaded");
		goo_component_set_state_loaded (self->component);

		if (goo_ti_audio_component_is_dasf_mode (GOO_TI_AUDIO_COMPONENT (self->component)))
		{
			GST_INFO_OBJECT (self, "clock going to loaded");
			goo_component_set_state_loaded (self->clock);
		}
	}
	GST_OBJECT_UNLOCK (self);

	return;
}

static void
omx_wait_for_done (GstGooEncAac* self)
{
	g_assert (self != NULL);

	OMX_BUFFERHEADERTYPE* omx_buffer = NULL;
	OMX_PARAM_PORTDEFINITIONTYPE* param = NULL;

	param = GOO_PORT_GET_DEFINITION (self->inport);
	int omxbufsiz = param->nBufferSize;
	int avail = gst_goo_adapter_available (self->adapter);

	if (goo_port_is_tunneled (self->inport))
	{
		GST_INFO_OBJECT (self, "Tunneled Input port: Setting done");
		goo_component_set_done (self->component);
		return;
	}

	if (avail < omxbufsiz && avail >= 0)
	{
		GST_DEBUG_OBJECT (self, "Sending an EOS buffer");
		goo_component_send_eos (self->component);
	}
	else
	{
		/* For some reason the adapter didn't extract all
		   possible buffers */
		g_assert_not_reached ();
	}

	gst_goo_adapter_clear (self->adapter);

	GST_INFO_OBJECT (self, "Waiting for done signal");
	goo_component_wait_for_done (self->component);

	return;
}

static gboolean
gst_goo_encaac_event (GstPad* pad, GstEvent* event)
{
	GstGooEncAac* self = GST_GOO_ENCAAC (gst_pad_get_parent (pad));

	GST_DEBUG ("encaac %s", GST_EVENT_TYPE_NAME (event));

	gboolean ret = FALSE;

	switch (GST_EVENT_TYPE (event))
	{
		case GST_EVENT_NEWSEGMENT:
			GST_INFO_OBJECT (self, "New segement event");
			ret = gst_pad_push_event (self->srcpad, event);
			break;
		case GST_EVENT_EOS:
			GST_INFO_OBJECT (self, "EOS event");
			omx_wait_for_done (self);
			ret = gst_pad_push_event (self->srcpad, event);
			break;
		default:
			ret = gst_pad_event_default (pad, event);
			break;
	}

	gst_object_unref (self);
	return ret;
}

static gboolean
gst_goo_encaac_setcaps (GstPad * pad, GstCaps * caps)
{
	GstGooEncAac* self = GST_GOO_ENCAAC (gst_pad_get_parent (pad));
	GstStructure* structure = gst_caps_get_structure (caps, 0);
	OMX_AUDIO_PARAM_PCMMODETYPE* in_param = NULL;
	OMX_AUDIO_PARAM_AACPROFILETYPE* out_param = NULL;

	gint channels, samplerate, width;
	gulong samples, bytes, fmt = 0;
	gboolean result = FALSE;

	GstCaps* srccaps = NULL;

	GST_DEBUG_OBJECT (self, "");
	if (!gst_caps_is_fixed (caps))
	{
		goto done;
	}

	GST_DEBUG_OBJECT (self, "try getting channels and rate");
	if (!gst_structure_get_int (structure, "channels", &channels) ||
	    !gst_structure_get_int (structure, "rate", &samplerate))
	{
		goto done;
	}

	GST_DEBUG_OBJECT (self, "see if audio/x-raw-int exists");
	if (gst_structure_has_name (structure, "audio/x-raw-int"))
	{
		gst_structure_get_int (structure, "width", &width);
		switch (width)
		{
		case 16:
			fmt = 16;
			break;
		default:
			g_return_val_if_reached (FALSE);
		}
	}
	else
	{
		g_return_val_if_reached (FALSE);
	}


	/* 20 msec */
	gint outputframes = 0;
	g_object_get (self->component, "frames-buffer", &outputframes, NULL);
	GST_DEBUG_OBJECT (self, "frames-buffer=%d", outputframes);
//	self->duration = outputframes *
//		gst_util_uint64_scale_int (1, GST_SECOND, 50);

	GST_OBJECT_LOCK (self);
	/* input params */
	in_param = GOO_TI_AACENC_GET_INPUT_PORT_PARAM (self->component);
	in_param->nBitPerSample = fmt;
	in_param->nChannels = channels;
	in_param->nSamplingRate = samplerate;

	/* output params */
	out_param = GOO_TI_AACENC_GET_OUTPUT_PORT_PARAM (self->component);

	out_param->nChannels = channels;
	out_param->nSampleRate = samplerate;
	if (channels == 1)
	{
		out_param->eChannelMode = OMX_AUDIO_ChannelModeMono;
	}
	if (channels == 2)
	{
		out_param->eChannelMode = OMX_AUDIO_ChannelModeStereo;
	}
	GST_OBJECT_UNLOCK (self);

	GST_DEBUG_OBJECT (self, "omx_start");
	omx_start (self);

	if (goo_port_is_tunneled (self->inport))
	{
		/*In case of DM (DASF-MODE), we set the duration to a constant */
	   /* precalc duration as it's constant now */
		self->duration =
			gst_util_uint64_scale_int (160, GST_SECOND, samplerate * channels);
	}
	else
	{
		guint bytes_per_second = in_param->nBitPerSample / 8;
		bytes_per_second *= in_param->nChannels;
		bytes_per_second *= in_param->nSamplingRate;
		self->ns_per_byte = (gint)(1000000000.0 / (gfloat)bytes_per_second);
	}

	/* now create a caps for it all */
	srccaps = gst_caps_new_simple ("audio/mpeg",
				       "mpegversion", G_TYPE_INT, 4,
				       "channels", G_TYPE_INT, channels,
				       "rate", G_TYPE_INT, samplerate,
				       NULL);

	result = gst_pad_set_caps (self->srcpad, srccaps);
	gst_caps_unref (srccaps);

done:
	gst_object_unref (self);
	return result;
}

static GstFlowReturn
gst_goo_encaac_chain (GstPad* pad, GstBuffer* buffer)
{
	GstGooEncAac* self = GST_GOO_ENCAAC (gst_pad_get_parent (pad));
	guint omxbufsiz = GOO_PORT_GET_DEFINITION (self->inport)->nBufferSize;
	GstFlowReturn result;

	if (self->component->cur_state != OMX_StateExecuting)
	{
		goto not_negotiated;
	}

	if (goo_port_is_tunneled (self->inport))
	{
		GST_DEBUG_OBJECT (self, "DASF Source");
		OMX_BUFFERHEADERTYPE* omxbuf = NULL;
		omxbuf = goo_port_grab_buffer (self->outport);
		self->ts = OMX2GST_TIMESTAMP (goo_ti_clock_get_timestamp (
			GOO_TI_CLOCK (self->clock)));
		GST_DEBG_OBJECT ("timestamp: %"GST_TIME_FORMAT,
			GST_TIME_ARGS (self->ts));
		result = process_output_buffer (self, omxbuf);
		return result;
	}

	/* discontinuity clears adapter, FIXME, maybe we can set some
	 * encoder flag to mask the discont. */
	if (GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DISCONT))
	{
GST_DEBUG ("********* CLEARING ADAPTER!!! ************");
		gst_goo_adapter_clear (self->adapter);
//		self->ts = 0;
	}

	/* take latest timestamp, FIXME timestamp is the one of the
	 * first buffer in the adapter. */
//	if (GST_BUFFER_TIMESTAMP_IS_VALID (buffer))
//	{
//		self->ts = GST_BUFFER_TIMESTAMP (buffer);
//	}

	result = GST_FLOW_OK;
	GST_DEBUG_OBJECT (self, "Pushing a GST buffer to adapter (%d)",
			  GST_BUFFER_SIZE (buffer));
	gst_goo_adapter_push (self->adapter, buffer);

	GST_DEBUG ("buffer=0x%08x (time=%"GST_TIME_FORMAT", duration=%"GST_TIME_FORMAT", flags=%08x)",
			buffer,
			GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buffer)),
			GST_TIME_ARGS (GST_BUFFER_DURATION (buffer)),
			GST_BUFFER_FLAGS (buffer)
		);
duration_in += GST_BUFFER_DURATION (buffer);
GST_DEBUG ("duration_in=%"GST_TIME_FORMAT, GST_TIME_ARGS (duration_in));

	/* Collect samples until we have enough for an output frame */
	while (gst_goo_adapter_available (self->adapter) >= omxbufsiz)
	{
		GST_DEBUG_OBJECT (self, "Popping an OMX buffer");
		OMX_BUFFERHEADERTYPE* omxbuf;
		omxbuf = goo_port_grab_buffer (self->inport);
		gst_goo_adapter_peek (self->adapter, omxbufsiz, omxbuf);
		omxbuf->nFilledLen = omxbufsiz;
		gst_goo_adapter_flush (self->adapter, omxbufsiz);

		GST_OBJECT_LOCK (self);
		self->bytes_pending += omxbufsiz;
		goo_component_release_buffer (self->component, omxbuf);
		GST_OBJECT_UNLOCK (self);
	}

done:
	GST_DEBUG_OBJECT (self, "");
	gst_object_unref (self);
	gst_buffer_unref (buffer);

	return result;

	/* ERRORS */
not_negotiated:
	{
		GST_ELEMENT_ERROR (self, CORE, NEGOTIATION, (NULL),
				   ("failed to negotiate MPEG/AAC format with next element"));
		result = GST_FLOW_ERROR;
		goto done;
	}
}

static GstStateChangeReturn
gst_goo_encaac_change_state (GstElement* element, GstStateChange transition)
{
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
	GstGooEncAac *self = GST_GOO_ENCAAC (element);

	/* upwards state changes */
	switch (transition)
	{
	case GST_STATE_CHANGE_READY_TO_PAUSED:
		GST_OBJECT_LOCK (self);
		{
			self->bytes_pending = 0;
			self->ts = 0;
			gst_goo_adapter_clear (self->adapter);
			if (GOO_COMPONENT (self->component)->cur_state == OMX_StateIdle &&
					(self->mimo_mode == TRUE))
			{
			    GooTiAudioManager* gAm=NULL;
			    gint ret;
			    gAm =  GOO_TI_AUDIO_COMPONENT (self->component)->manager;
			    g_assert (gAm != NULL);
			    gAm->cmd->AM_Cmd = AM_CommandWarnSampleFreqChange;
			    gAm->cmd->param1 = 8000;
			    gAm->cmd->param2 = 5;
			    ret = write (gAm->fdwrite, gAm->cmd, sizeof (AM_COMMANDDATATYPE));
			    g_assert (ret ==  sizeof (AM_COMMANDDATATYPE));
			}
		}
		GST_OBJECT_UNLOCK (self);
		break;
	default:
		break;
	}

	ret = GST_ELEMENT_CLASS (parent_class)->change_state (element,
							      transition);

	/* downwards state changes */
	switch (transition)
	{
	case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
		/* goo_component_set_state_paused (self->component); */
		break;
	case GST_STATE_CHANGE_READY_TO_NULL:
		omx_stop (self);
		break;
	default:
		break;
	}

	return ret;
}

static void
gst_goo_encaac_set_property (GObject* object, guint prop_id,
			     const GValue* value, GParamSpec* pspec)
{
	g_assert (GST_IS_GOO_ENCAAC (object));
	GstGooEncAac* self = GST_GOO_ENCAAC (object);

	OMX_AUDIO_PARAM_AACPROFILETYPE* param = NULL;
	param = GOO_TI_AACENC_GET_OUTPUT_PORT_PARAM (self->component);

	switch (prop_id)
	{
	case PROP_BITRATE:
		param->nBitRate = g_value_get_uint (value);
		break;
	case PROP_BITRATEMODE:
		self->bitratemode = g_value_get_enum (value);
		break;
	case PROP_OUTPUTFORMAT:
		param->eAACStreamFormat = g_value_get_enum (value);
		break;
	case PROP_PROFILE:
		param->eAACProfile = g_value_get_enum (value);
		break;
	case PROP_MIMO_MODE:
		self->mimo_mode = g_value_get_boolean (value);
		_goo_ti_aacenc_set_mimo_mode (self);
	    break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_encaac_get_property (GObject* object, guint prop_id,
			     GValue* value, GParamSpec* pspec)
{
	g_assert (GST_IS_GOO_ENCAAC (object));
	GstGooEncAac* self = GST_GOO_ENCAAC (object);

	OMX_AUDIO_PARAM_AACPROFILETYPE* param = NULL;
	param = GOO_TI_AACENC_GET_OUTPUT_PORT_PARAM (self->component);

	switch (prop_id)
	{
	case PROP_BITRATE:
		g_value_set_uint (value, param->nBitRate);
		break;
	case PROP_BITRATEMODE:
		g_value_set_enum (value, self->bitratemode);
		break;
	case PROP_OUTPUTFORMAT:
		g_value_set_enum (value, param->eAACStreamFormat);
		break;
	case PROP_PROFILE:
		g_value_set_enum (value, param->eAACProfile);
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

static void
gst_goo_encaac_dispose (GObject* object)
{
	G_OBJECT_CLASS (parent_class)->dispose (object);

	GstGooEncAac* self = GST_GOO_ENCAAC (object);

	if (G_LIKELY (self->inport))
	{
		GST_DEBUG ("unrefing outport");
		g_object_unref (self->inport);
	}

	if (G_LIKELY (self->outport))
	{
		GST_DEBUG ("unrefing outport");
		g_object_unref (self->outport);
	}

	if (G_LIKELY (self->component))
	{
		GST_DEBUG ("unrefing component");
		G_OBJECT(self->component)->ref_count = 1;
		g_object_unref (self->component);
	}

	if (G_LIKELY (self->clock != NULL))
	{
		GST_DEBUG ("unrefing clock");
		g_object_unref (self->clock);
	}

	if (G_LIKELY (self->factory))
	{
		GST_DEBUG ("unrefing factory");
		g_object_unref (self->factory);
	}

	if (G_LIKELY (self->adapter))
	{
		GST_DEBUG ("unrefing adapter");
		g_object_unref (self->adapter);
	}

	return;
}

static void
gst_goo_encaac_base_init (gpointer g_klass)
{

	GstElementClass* e_klass = GST_ELEMENT_CLASS (g_klass);

	gst_element_class_add_pad_template (e_klass,
					    gst_static_pad_template_get
					    (&sink_template));

	gst_element_class_add_pad_template (e_klass,
					    gst_static_pad_template_get
					    (&src_template));

	gst_element_class_set_details (e_klass, &details);

	return;
}

static void
gst_goo_encaac_class_init (GstGooEncAacClass* klass)
{
	GObjectClass* g_klass = G_OBJECT_CLASS (klass);
	GstElementClass* gst_klass = GST_ELEMENT_CLASS (klass);

	g_klass->set_property = gst_goo_encaac_set_property;
	g_klass->get_property = gst_goo_encaac_get_property;
	g_klass->dispose = gst_goo_encaac_dispose;

	GParamSpec* spec;

	spec = g_param_spec_uint ("bitrate", "Bitrate (bps)",
				  "Bitrate in bits/sec",
				  0, 128000, DEFAULT_BITRATE,
				  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_BITRATE, spec);

	spec = g_param_spec_enum ("bitrate-mode", "Bit Rate Mode",
				  "Specifies Bit Rate Mode",
				  GST_GOO_TYPE_BITRATEMODE,
				  DEFAULT_BITRATEMODE,
				  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_BITRATEMODE, spec);

	spec = g_param_spec_enum ("outputformat", "Output format",
				  "Format of output frames",
				  GST_GOO_TYPE_OUTPUTFORMAT,
				  DEFAULT_OUTPUTFORMAT,
				  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_OUTPUTFORMAT, spec);

	spec = g_param_spec_enum ("profile", "Profile",
				  "MPEG/AAC encoding profile",
				  GST_GOO_TYPE_PROFILE,
				  DEFAULT_PROFILE,
				  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_PROFILE, spec);

	spec = g_param_spec_boolean ("mimo-mode", "MIMO Mixer Mode",
	            "Activates MIMO Operation",
	            DEFAULT_MIMO_MODE, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_MIMO_MODE, spec);

	gst_klass->change_state =
		GST_DEBUG_FUNCPTR (gst_goo_encaac_change_state);

	return;
}

static void
gst_goo_encaac_init (GstGooEncAac* self, GstGooEncAacClass *klass)
{
	/* goo stuff */
	{
		self->factory = goo_ti_component_factory_get_instance ();
		self->component = goo_component_factory_get_component
			(self->factory, GOO_TI_AAC_ENCODER);

		self->inport =
			goo_component_get_port (self->component, "input0");
		g_assert (self->inport != NULL);

		self->outport =
			goo_component_get_port (self->component, "output0");
		g_assert (self->outport != NULL);

		OMX_AUDIO_PARAM_AACPROFILETYPE* param = NULL;
		param = GOO_TI_AACENC_GET_OUTPUT_PORT_PARAM (self->component);

		param->nBitRate = DEFAULT_BITRATE;
		param->eAACStreamFormat = DEFAULT_OUTPUTFORMAT;
		param->eAACProfile = DEFAULT_PROFILE;
		param->nChannels = DEFAULT_CHANNELS;
		param->eChannelMode = DEFAULT_CHANNELS_MODE;
		param->nSampleRate = DEFAULT_SAMPLERATE;
	}

	/* gst stuff */
	{
		self->sinkpad = gst_pad_new_from_static_template
			(&sink_template, "sink");
		gst_pad_set_setcaps_function
			(self->sinkpad,
			 GST_DEBUG_FUNCPTR (gst_goo_encaac_setcaps));
		gst_pad_set_event_function
			(self->sinkpad,
			 GST_DEBUG_FUNCPTR (gst_goo_encaac_event));
		gst_pad_set_chain_function
			(self->sinkpad,
			 GST_DEBUG_FUNCPTR (gst_goo_encaac_chain));
		gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);

		self->srcpad = gst_pad_new_from_static_template
			(&src_template, "src");
		gst_pad_use_fixed_caps (self->srcpad);
		gst_element_add_pad (GST_ELEMENT (self), self->srcpad);
	}

	/* gobject stuff */
	g_object_set_data (G_OBJECT (self->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", self->component);

	self->bitratemode = GOO_TI_AACENC_BR_CBR;
	self->adapter = gst_goo_adapter_new ();
	self->mimo_mode = FALSE;

	GST_DEBUG_OBJECT (self, "");

        return;
}
