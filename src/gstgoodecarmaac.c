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

#include <goo-ti-armaacdec.h>
#include "gstgoodecarmaac.h"
#include "gstgoobuffer.h"
#include <string.h>

/* default values */
#define BITPERSAMPLE_DEFAULT 16
#define CHANNELS_DEFAULT 2
#define INPUT_PROFILE_DEFAULT OMX_AUDIO_AACObjectMain
#define RAW_DEFAULT OMX_AUDIO_AACStreamFormatRaw
#define INPUT_DEFAULT_FORMAT OMX_AUDIO_AACStreamFormatMax
#define SAMPLERATE_DEFAULT 44100
#define INPUT_BUFFERSIZE_DEFAULT 2000
#define OUTPUT_BUFFERSIZE_DEFAULT 8192
#define NUM_INPUT_BUFFERS_DEFAULT 1
#define NUM_OUTPUT_BUFFERS_DEFAULT 1
#define DEFAULT_WIDTH 16
#define DEFAULT_DEPTH 16

/*args*/
enum
{
	PROP_0,
	PROP_PROFILE,
	PROP_SBR,
	PROP_BITOUTPUT,
	PROP_PARAMETRICSTEREO,
    PROP_NUM_INPUT_BUFFERS,
    PROP_NUM_OUTPUT_BUFFERS
};

static const GstElementDetails details =
        GST_ELEMENT_DETAILS (
                "OpenMAX ARM-AAC decoder",
                "Codec/Decoder/Audio",
                "Decodes Advanced Audio Coding streams with OpenMAX using ARM",
                "Texas Instruments"
                );

/* FIXME: make three caps, for mpegversion 1, 2 and 2.5 */
static GstStaticPadTemplate sink_template =
        GST_STATIC_PAD_TEMPLATE (
                "sink",
                GST_PAD_SINK,
                GST_PAD_ALWAYS,
                GST_STATIC_CAPS ("audio/mpeg, "
				"mpegversion = (int) 4, "
				"rate = (int) [8000, 96000],"
				"channels = (int) [1, 2] "
				));

static GstStaticPadTemplate src_template =
GST_STATIC_PAD_TEMPLATE ("src",
		GST_PAD_SRC,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS ("audio/x-raw-int, "
				"rate = (int) [8000, 96000], "
				"channels = (int) [1, 2], "
				"width = (int) 16, "
				"depth = (int) 16 "
				));

GST_DEBUG_CATEGORY_STATIC (gst_goo_decarmaac_debug);
#define GST_CAT_DEFAULT gst_goo_decarmaac_debug

#define _do_init(blah) \
	GST_DEBUG_CATEGORY_INIT (gst_goo_decarmaac_debug, "goodecarmaac", 0, \
		"OpenMAX ARM-Advanced Audio Coding Decoder element");

GST_BOILERPLATE_FULL (GstGooDecArmAac, gst_goo_decarmaac,
	GstElement, GST_TYPE_ELEMENT, _do_init);

static gboolean
_goo_ti_armaacdec_set_profile (GstGooDecArmAac* self, guint profile)
{
	g_assert (self != NULL);
	GooComponent * component = self->component;


	gboolean retval = TRUE;


	self->profile = profile;


	switch (self->profile)
	{
		case 0:
			GOO_TI_ARMAACDEC_GET_INPUT_PORT_PARAM (component)->eAACProfile = OMX_AUDIO_AACObjectMain;
			break;
		case 1:
			GOO_TI_ARMAACDEC_GET_INPUT_PORT_PARAM (component)->eAACProfile = OMX_AUDIO_AACObjectLC;
			break;
		case 2:
			GOO_TI_ARMAACDEC_GET_INPUT_PORT_PARAM (component)->eAACProfile = OMX_AUDIO_AACObjectSSR;
			break;
		case 3:
			GOO_TI_ARMAACDEC_GET_INPUT_PORT_PARAM (component)->eAACProfile = OMX_AUDIO_AACObjectLTP;
			break;
		default:
			GOO_TI_ARMAACDEC_GET_INPUT_PORT_PARAM (component)->eAACProfile = OMX_AUDIO_AACObjectMain;
			break;
	}


	return retval;

}

static gboolean
_goo_ti_armaacdec_set_sbr (GstGooDecArmAac* self, gboolean sbr)
{
	g_assert (self != NULL);
	GooComponent* component = self->component;

	gboolean retval = TRUE;

	self->sbr = sbr;

	if (sbr == TRUE)
	{
		GOO_TI_ARMAACDEC_GET_INPUT_PORT_PARAM (component)->eAACProfile = OMX_AUDIO_AACObjectHE;
	}

	return retval;

}

static gboolean
_goo_ti_armaacdec_set_parametric_stereo (GstGooDecArmAac* self, gboolean parametric_stereo)
{
	g_assert (self != NULL);
	GooComponent* component = self->component;

	gboolean retval = TRUE;

	self->parametric_stereo = parametric_stereo;

	if (parametric_stereo == TRUE)
	{
		GOO_TI_ARMAACDEC_GET_INPUT_PORT_PARAM (component)->eAACProfile = OMX_AUDIO_AACObjectHE_PS;
	}

	return retval;

}

static gboolean
_goo_ti_armaacdec_set_bit_output (GstGooDecArmAac* self, gboolean bit_output)
{
	g_assert (self != NULL);

	GooComponent * component = self->component;
	gboolean retval = TRUE;


	self->bit_output = bit_output;


	switch (self->bit_output)
	{
		case 0:
			GOO_TI_ARMAACDEC_GET_OUTPUT_PORT_PARAM (component)->nBitPerSample = 16;
			break;
		case 1:
			GOO_TI_ARMAACDEC_GET_OUTPUT_PORT_PARAM (component)->nBitPerSample = 24;
			break;
		default:
			GOO_TI_ARMAACDEC_GET_OUTPUT_PORT_PARAM (component)->nBitPerSample = 16;
			break;
	}


	return retval;

}

static GstFlowReturn
process_output_buffer (GstGooDecArmAac* self, OMX_BUFFERHEADERTYPE* buffer)
{
	GstBuffer* out = NULL;
	GstFlowReturn ret = GST_FLOW_ERROR;
	gint iNum;
	gint iDenom;

	if (buffer->nFilledLen <= 0)
	{
		GST_INFO_OBJECT (self, "Received an empty buffer!");
		goo_component_release_buffer (self->component, buffer);
		return GST_FLOW_OK;
	}

	out = gst_buffer_new_and_alloc (buffer->nFilledLen);
	memmove (GST_BUFFER_DATA (out),
		buffer->pBuffer, buffer->nFilledLen);
	iNum = buffer->nFilledLen * 8;
	iDenom = self->rate * self->channels * 16;
	self->duration = gst_util_uint64_scale_int (GST_SECOND,
		iNum,
		iDenom);

	GST_DEBUG_OBJECT (self, "Output buffer size %d", buffer->nFilledLen);
	goo_component_release_buffer (self->component, buffer);

	if (out != NULL)
	{
		GST_BUFFER_DURATION (out) = self->duration;
		GST_BUFFER_OFFSET (out) = self->outcount++;
		GST_BUFFER_TIMESTAMP (out) = self->ts;
		GST_DEBUG_OBJECT (self, "timestamp %u", self->ts);
		if (self->ts != -1)
		{
			self->ts += self->duration;
		}

		gst_buffer_set_caps (out, GST_PAD_CAPS (self->srcpad));

		GST_DEBUG_OBJECT (self, "pushing gst buffer");
		if (((self->ts - self->prev_ts) > 0 ) || (self->outcount == 0))
		{
			ret = gst_pad_push (self->srcpad, out);
			self->prev_ts = self->ts;
		}
	}

	GST_INFO_OBJECT (self, "");

	return ret;
}

static void
omx_output_buffer_cb (GooPort* port, OMX_BUFFERHEADERTYPE* buffer,
	gpointer data)
{
	g_return_if_fail (buffer->nFlags != OMX_BUFFERFLAG_DATACORRUPT);

	g_assert (GOO_IS_PORT (port));
	g_assert (buffer != NULL);
	g_assert (GOO_IS_COMPONENT (data));

	GooComponent* component = GOO_COMPONENT (data);
	GstGooDecArmAac* self = GST_GOO_DECARMAAC (
		g_object_get_data (G_OBJECT (data), "gst")
		);
	g_assert (self != NULL);

	{
		process_output_buffer (self, buffer);

		if (buffer->nFlags & OMX_BUFFERFLAG_EOS == 0x1 ||
		    goo_port_is_eos (port))
		{
			GST_INFO_OBJECT (self,
					 "EOS found in output buffer (%d)",
					 buffer->nFilledLen);
			goo_component_set_done (self->component);
		}
	}

	GST_INFO_OBJECT (self, "");

	return;
}

static void
omx_sync (GstGooDecArmAac* self)
{
	g_assert (self != NULL);
	g_assert (self->component != NULL);

	OMX_PARAM_PORTDEFINITIONTYPE *def = NULL;
	OMX_AUDIO_PARAM_AACPROFILETYPE* param;

	GST_INFO_OBJECT (self, "");

	return;
}

static void
omx_start (GstGooDecArmAac* self)
{
	g_assert (self != NULL);
	g_assert (self->component != NULL);

	if (!goo_port_is_tunneled (self->inport))
	{
		goo_port_set_process_buffer_function (self->outport,
						      omx_output_buffer_cb);
	}

	if (goo_component_get_state (self->component) == OMX_StateLoaded)
	{
		omx_sync (self);
		GST_INFO_OBJECT (self, "going to idle");
		goo_component_set_state_idle (self->component);
		GST_INFO_OBJECT (self, "gone to idle");
	}

	if (goo_component_get_state (self->component) == OMX_StateIdle)
	{
		GST_INFO_OBJECT (self, "going to executing");
		goo_component_set_state_executing (self->component);
	}

	return;
}

static void
omx_stop (GstGooDecArmAac* self)
{
	g_assert (self != NULL);
	g_assert (self->component != NULL);

	if (goo_component_get_state (self->component) == OMX_StateExecuting)
	{
		GST_INFO_OBJECT (self, "going to idle");
		goo_component_set_state_idle (self->component);
		GST_INFO_OBJECT (self, "gone to idle");
	}

	if (goo_component_get_state (self->component) == OMX_StateIdle)
	{
		GST_INFO_OBJECT (self, "going to loaded");
		goo_component_set_state_loaded (self->component);
		GST_INFO_OBJECT (self, "gone to loaded");
	}

	return;
}

static void
omx_wait_for_done (GstGooDecArmAac* self)
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
gst_goo_decarmaac_setcaps (GstPad* pad, GstCaps* caps)
{
	GstGooDecArmAac *self = GST_GOO_DECARMAAC (gst_pad_get_parent (pad));
	GstStructure *structure = gst_caps_get_structure (caps, 0);
	GstPad *peer;
	GstBuffer *audio_header;
	GstElement *prev_element;
	guint channels = CHANNELS_DEFAULT;
	gchar *str_caps;
	gchar *str_peer;
	gchar compare[] = "queue";
	gint comp_res;
	const GValue *value;
	GstCaps *copy;
	static gint sample_rates[] = {96000, 88200, 64000, 48000, 44100, 32000,
		24000, 22050, 16000, 12000, 11025, 8000};
	gint32 format;

	peer = gst_pad_get_peer (pad);
	prev_element = GST_ELEMENT (gst_pad_get_parent (peer));
	str_peer = gst_element_get_name (prev_element);
	comp_res = strncmp (compare, str_peer, 5);

	g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE);

	structure = gst_caps_get_structure (caps, 0);
	str_caps = gst_structure_to_string (structure);

	GST_DEBUG_OBJECT (self, "sink caps: %s", str_caps);
	OMX_AUDIO_PARAM_AACPROFILETYPE* param = GOO_TI_ARMAACDEC_GET_INPUT_PORT_PARAM (self->component);

	if (gst_structure_has_field (structure, "codec_data"))
	{
		gint iIndex = 0;
		value = gst_structure_get_value (structure, "codec_data");
		audio_header = gst_value_get_buffer (value);

		// Lets see if we can get some information out of this crap
		iIndex = (((GST_BUFFER_DATA (audio_header)[0] & 0x7) << 1) |
			((GST_BUFFER_DATA (audio_header)[1] & 0x80) >> 7));
		channels = (GST_BUFFER_DATA (audio_header)[1] & 0x78) >> 3;
		GST_DEBUG_OBJECT (self, "iIndex %d channels %d", iIndex, channels);

		self->channels = channels;
		self->rate = sample_rates[iIndex];

		format = OMX_AUDIO_AACStreamFormatRAW;

		GST_DEBUG_OBJECT (self, "channels %d rate %d", self->channels,
			self->rate);
	}
	else
	{
		// get channel count
		gst_structure_get_int (structure, "channels", &self->channels);
		gst_structure_get_int (structure, "rate", &self->rate);

		if ((comp_res == 0) || ((self->sbr == FALSE) && (self->parametric_stereo == FALSE)))
		{
			GST_DEBUG_OBJECT (self, "simple AAC");
		}
		else
		{
			GST_DEBUG_OBJECT (self, "He or eAAC+");
			if (self->parametric_stereo)
				self->rate /= 2;
		}
		format = OMX_AUDIO_AACStreamFormatMax;
	}

	param->nSampleRate = self->rate;
	param->nChannels = self->channels;
	param->eAACStreamFormat = format;

	// create reverse caps
	copy = gst_caps_new_simple ("audio/x-raw-int",
		"width", G_TYPE_INT, 16,
		"depth", G_TYPE_INT, 16,
		"signed", G_TYPE_BOOLEAN, TRUE,
		"endianness", G_TYPE_INT, G_BYTE_ORDER,
	    "channels", G_TYPE_INT, self->channels,
	    "rate", G_TYPE_INT, self->rate, NULL);

	if ((self->sbr == TRUE) && (gst_structure_has_field (structure, "codec_data")))
	{
		self->rate *= 2;
		copy = gst_caps_new_simple ("audio/x-raw-int",
			"width", G_TYPE_INT, 16,
			"depth", G_TYPE_INT, 16,
			"signed", G_TYPE_BOOLEAN, TRUE,
			"endianness", G_TYPE_INT, G_BYTE_ORDER,
			"channels", G_TYPE_INT, 2,
			"rate", G_TYPE_INT, self->rate, NULL);
	}


	if ((self->parametric_stereo == TRUE) && (gst_structure_has_field (structure, "codec_data")))
	{
		copy = gst_caps_new_simple ("audio/x-raw-int",
			"width", G_TYPE_INT, 16,
			"depth", G_TYPE_INT, 16,
			"signed", G_TYPE_BOOLEAN, TRUE,
			"endianness", G_TYPE_INT, G_BYTE_ORDER,
			"channels", G_TYPE_INT, 2,
			"rate", G_TYPE_INT, self->rate, NULL);
	}

	gst_object_unref (prev_element);
	gst_object_unref (peer);

	gst_pad_set_caps (self->srcpad, copy);
	gst_caps_unref (copy);

	omx_start (self);
	gst_object_unref (self);

	GST_DEBUG_OBJECT (self, "");

	return TRUE;
}

static GstFlowReturn
gst_goo_decarmaac_chain (GstPad* pad, GstBuffer* buffer)
{
	GstGooDecArmAac* self = GST_GOO_DECARMAAC (gst_pad_get_parent (pad));
	guint omxbufsiz = GOO_PORT_GET_DEFINITION (self->inport)->nBufferSize;
	GstFlowReturn ret;

	if (self->rate == 0 || self->channels == 0 ||
	    self->component->cur_state != OMX_StateExecuting)
	{
		goto not_negotiated;
	}

	if (goo_port_is_tunneled (self->inport))
	{
		GST_DEBUG_OBJECT (self, "DASF Source");
		OMX_BUFFERHEADERTYPE* omxbuf = NULL;
		omxbuf = goo_port_grab_buffer (self->outport);
		ret = process_output_buffer (self, omxbuf);
		goto done;
	}

	/* discontinuity clears adapter, FIXME, maybe we can set some
	 * encoder flag to mask the discont. */
	if (GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DISCONT))
	{
		gst_goo_adapter_clear (self->adapter);
		self->ts = 0;
	}

	/* take latest timestamp, FIXME timestamp is the one of the
	 * first buffer in the adapter. */
	if (GST_BUFFER_TIMESTAMP_IS_VALID (buffer))
	{
		self->ts = GST_BUFFER_TIMESTAMP (buffer);
		self->val_ts = TRUE;
	}

	ret = GST_FLOW_OK;
	GST_DEBUG_OBJECT (self, "Pushing a GST buffer to adapter (%d)",
			  GST_BUFFER_SIZE (buffer));
	gst_goo_adapter_push (self->adapter, buffer);

	/* Collect samples until we have enough for an output frame */
	while (gst_goo_adapter_available (self->adapter) >= omxbufsiz)
	{
		GST_DEBUG_OBJECT (self, "Popping an OMX buffer");
		OMX_BUFFERHEADERTYPE* omxbuf;
		omxbuf = goo_port_grab_buffer (self->inport);
		gst_goo_adapter_peek (self->adapter, omxbufsiz, omxbuf);
		omxbuf->nFilledLen = omxbufsiz;
		gst_goo_adapter_flush (self->adapter, omxbufsiz);
		goo_component_release_buffer (self->component, omxbuf);
	}

done:
	GST_DEBUG_OBJECT (self, "");
	gst_object_unref (self);
	return ret;

	/* ERRORS */
not_negotiated:
	{
		GST_ELEMENT_ERROR (self, CORE, NEGOTIATION, (NULL),
				   ("format wasn't negotiated before chain function"));
		gst_buffer_unref (buffer);
		ret = GST_FLOW_NOT_NEGOTIATED;
		goto done;
	}
}

static gboolean
gst_goo_decarmaac_bytepos_to_time (GstGooDecArmAac *self,
    gint64 bytepos, GstClockTime * ts)
{

	GST_DEBUG_OBJECT (self, "rate %d", self->rate);
  if (bytepos == -1) {
    *ts = GST_CLOCK_TIME_NONE;
    return TRUE;
  }

  if (bytepos == 0) {
    *ts = 0;
    return TRUE;
  }

  /* Cannot convert anything except 0 if we don't have a bitrate yet */
  if (self->rate == 0)
    return FALSE;

  *ts = (GstClockTime) gst_util_uint64_scale (GST_SECOND, bytepos * 8,
      self->rate);

  return TRUE;
}

static void
gst_goo_decarmaac_wait_for_done (GstGooDecArmAac* self)
{
	g_assert (self != NULL);

	/* flushing the last buffers in adapter */

	OMX_BUFFERHEADERTYPE* omx_buffer;
	OMX_PARAM_PORTDEFINITIONTYPE* param =
		GOO_PORT_GET_DEFINITION (self->inport);
	GstGooAdapter* adapter = self->adapter;
	int omxbufsiz = param->nBufferSize;
	int avail = gst_goo_adapter_available (adapter);

	if (goo_port_is_tunneled (self->inport))
	{
		GST_INFO ("Input port is tunneled: Setting done");
		goo_component_set_done (self->component);
		return;
	}

	if (avail < omxbufsiz && avail > 0)
	{
		GST_INFO ("Marking EOS buffer");
		omx_buffer = goo_port_grab_buffer (self->inport);
		GST_DEBUG ("Peek to buffer %d bytes", avail);
		gst_goo_adapter_peek (adapter, avail, omx_buffer);
		omx_buffer->nFilledLen = avail;
		/* let's send the EOS flag right now */
		omx_buffer->nFlags |= OMX_BUFFERFLAG_EOS;
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
	if (goo_port_is_tunneled (self->outport))
	{
		GST_INFO ("Outport is tunneled: Setting done");
		goo_component_set_done (self->component);
	}
	else
	{
		GST_INFO ("Not Waiting for done signal");
		GST_INFO ("Working around OMAPS00198928");
		goo_component_set_done (self->component);
		//goo_component_wait_for_done (self->component);
	}

	return;
}

static gboolean
gst_goo_decarmaac_sink_event (GstPad * pad, GstEvent * event)
{
  gboolean res;
  GstGooDecArmAac *self;

  self = GST_GOO_DECARMAAC (gst_pad_get_parent (pad));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_NEWSEGMENT:
    {
      gdouble rate, applied_rate;
      GstFormat format;
      gint64 start, stop, pos;
      gboolean update;

      gst_event_parse_new_segment_full (event, &update, &rate, &applied_rate,
          &format, &start, &stop, &pos);

      GST_DEBUG_OBJECT (self, "rate %ld applied_rate %ld start %ld stop %ld pos %ld",
      			rate, applied_rate, format, start, stop, pos);

      if (format == GST_FORMAT_BYTES)
      {
      	GstClockTime seg_start, seg_stop, seg_pos;

        /* stop time is allowed to be open-ended, but not start & pos */
        if (!gst_goo_decarmaac_bytepos_to_time (self, stop, &seg_stop))
          seg_stop = GST_CLOCK_TIME_NONE;
        if (gst_goo_decarmaac_bytepos_to_time (self, start, &seg_start) &&
            gst_goo_decarmaac_bytepos_to_time (self, pos, &seg_pos))
        {
          gst_event_unref (event);
          event = gst_event_new_new_segment_full (update, rate, applied_rate,
              GST_FORMAT_TIME, seg_start, seg_stop, seg_pos);
          format = GST_FORMAT_TIME;
          GST_DEBUG_OBJECT (self, "Converted incoming segment to TIME. "
              "start = %" G_GINT64_FORMAT ", stop = %" G_GINT64_FORMAT
              "pos = %" G_GINT64_FORMAT, seg_start, seg_stop, seg_pos);
        }
      }

      if (format != GST_FORMAT_TIME)
      {
        /* Unknown incoming segment format. Output a default open-ended
         * TIME segment */
        gst_event_unref (event);
        event = gst_event_new_new_segment_full (update, rate, applied_rate,
            GST_FORMAT_TIME, 0, GST_CLOCK_TIME_NONE, 0);
      }

      res = gst_pad_push_event (self->srcpad, event);
      break;
    }
    case GST_EVENT_FLUSH_STOP:
      /* Clear our adapter and set up for a new position */
      gst_goo_adapter_clear (self->adapter);
      omx_wait_for_done (self);
      res = gst_pad_push_event (self->srcpad, event);
      break;
    case GST_EVENT_EOS:
      GST_INFO ("EOS event");
      gst_goo_decarmaac_wait_for_done (self);
      res = gst_pad_push_event (self->srcpad, event);
      break;
    default:
      res = gst_pad_push_event (self->srcpad, event);
      break;
  }

  gst_object_unref (self);

  return res;
}

static GstStateChangeReturn
gst_goo_decarmaac_state_change (GstElement* element, GstStateChange transition)
{
	g_assert (GST_IS_GOO_DECARMAAC (element));

	GstGooDecArmAac* self = GST_GOO_DECARMAAC (element);
	GstStateChangeReturn ret;

	switch (transition)
	{
	case GST_STATE_CHANGE_READY_TO_PAUSED:
		self->rate = 0;
		self->channels = 0;
		self->ts = 0;
		self->prev_ts = 0;
		self->outcount = 0;
		gst_segment_init (&self->segment, GST_FORMAT_TIME);
		gst_goo_adapter_clear (self->adapter);
		break;
	default:
		break;
	}

	ret = GST_ELEMENT_CLASS (parent_class)->change_state (element,
							      transition);

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

	GST_DEBUG_OBJECT (self, "");

	return ret;
}

static void
gst_goo_decarmaac_set_property (GObject* object, guint prop_id,
			       const GValue* value, GParamSpec* pspec)
{
	g_assert (GST_IS_GOO_DECARMAAC (object));
	GstGooDecArmAac* self = GST_GOO_DECARMAAC (object);

	OMX_AUDIO_PARAM_AACPROFILETYPE* param = NULL;
	param = GOO_TI_ARMAACDEC_GET_INPUT_PORT_PARAM (self->component);

	switch (prop_id)
	{
		case PROP_PROFILE:
            _goo_ti_armaacdec_set_profile (self,
                g_value_get_uint (value));
            break;
        case PROP_SBR:
            _goo_ti_armaacdec_set_sbr (self,
                g_value_get_boolean (value));
            break;
        case PROP_BITOUTPUT:
            _goo_ti_armaacdec_set_bit_output (self,
                g_value_get_boolean (value));
		    break;
        case PROP_PARAMETRICSTEREO:
            _goo_ti_armaacdec_set_parametric_stereo (self,
                g_value_get_boolean (value));
            break;
		case PROP_NUM_OUTPUT_BUFFERS:
			g_object_set_property (G_OBJECT (self->outport),
				       "buffercount", value);
			break;
		case PROP_NUM_INPUT_BUFFERS:
			g_object_set_property (G_OBJECT (self->inport),
				       "buffercount", value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}

	return;
}

static void
gst_goo_decarmaac_get_property (GObject* object, guint prop_id,
			       GValue* value, GParamSpec* pspec)
{
	g_assert (GST_IS_GOO_DECARMAAC (object));
	GstGooDecArmAac* self = GST_GOO_DECARMAAC (object);

	OMX_AUDIO_PARAM_AACPROFILETYPE* param = NULL;
	param = GOO_TI_ARMAACDEC_GET_INPUT_PORT_PARAM (self->component);

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
		case PROP_NUM_OUTPUT_BUFFERS:
			g_object_get_property (G_OBJECT (self->outport),
				       "buffercount", value);
			break;
		case PROP_NUM_INPUT_BUFFERS:
			g_object_get_property (G_OBJECT (self->inport),
				       "buffercount", value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}

	return;
}

static void
gst_goo_decarmaac_dispose (GObject* object)
{
	G_OBJECT_CLASS (parent_class)->dispose (object);

	GstGooDecArmAac* self = GST_GOO_DECARMAAC (object);

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
		g_object_unref (self->component);
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
gst_goo_decarmaac_base_init (gpointer g_klass)
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
gst_goo_decarmaac_class_init (GstGooDecArmAacClass* klass)
{
	GObjectClass* g_klass = G_OBJECT_CLASS (klass);
	GstElementClass* gst_klass = GST_ELEMENT_CLASS (klass);

	g_klass->set_property =
        GST_DEBUG_FUNCPTR (gst_goo_decarmaac_set_property);
	g_klass->get_property =
        GST_DEBUG_FUNCPTR (gst_goo_decarmaac_get_property);
	g_klass->dispose =
        GST_DEBUG_FUNCPTR (gst_goo_decarmaac_dispose);

	gst_klass->change_state =
		GST_DEBUG_FUNCPTR (gst_goo_decarmaac_state_change);

	GParamSpec* spec;

	spec = g_param_spec_uint ("input-buffers", "Input buffers",
				  "The number of OMX input buffers",
				  1, 4, NUM_INPUT_BUFFERS_DEFAULT,
				  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_NUM_INPUT_BUFFERS,
					 spec);

	spec = g_param_spec_uint ("output-buffers", "Output buffers",
				  "The number of OMX output buffers",
				  1, 4, NUM_OUTPUT_BUFFERS_DEFAULT,
				  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_NUM_OUTPUT_BUFFERS,
					 spec);

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

	return;
}

static void
gst_goo_decarmaac_init (GstGooDecArmAac* self, GstGooDecArmAacClass* klass)
{
	/* goo stuff */
	self->factory = goo_ti_component_factory_get_instance ();
	self->component =
		goo_component_factory_get_component (self->factory,
						     GOO_TI_ARMAAC_DECODER);

	self->inport = goo_component_get_port (self->component, "input0");
	g_assert (self->inport != NULL);

	self->outport = goo_component_get_port (self->component, "output0");
	g_assert (self->outport != NULL);

	/* gst stuff */
	self->sinkpad = gst_pad_new_from_static_template (&sink_template,
							  "sink");
	gst_pad_set_setcaps_function
		(self->sinkpad, GST_DEBUG_FUNCPTR (gst_goo_decarmaac_setcaps));
	gst_pad_set_event_function
		(self->sinkpad, GST_DEBUG_FUNCPTR (gst_goo_decarmaac_sink_event));
	gst_pad_set_chain_function
		(self->sinkpad, GST_DEBUG_FUNCPTR (gst_goo_decarmaac_chain));
	gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);

	self->srcpad = gst_pad_new_from_static_template (&src_template, "src");
	gst_pad_use_fixed_caps (self->srcpad);
	gst_element_add_pad (GST_ELEMENT (self), self->srcpad);

	/* gobject stuff */
	g_object_set_data (G_OBJECT (self->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", self->component);

	self->adapter = gst_goo_adapter_new ();

	GST_DEBUG_OBJECT (self, "");

    return;
}
