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

#include <goo-ti-nbamrenc.h>
#include "gstgooencnbamr.h"
#include "gstgoobuffer.h"

#include <string.h>

#define GST_GOO_ENCNBAMR_BANDMODE_TYPE \
	(gst_goo_encnbamr_bandmode_get_type ())

static GType
gst_goo_encnbamr_bandmode_get_type ()
{
	static GType type = 0;

	if (type == 0)
	{
		static const GEnumValue values[] = {
			{ OMX_AUDIO_AMRBandModeNB0, "MR475", "MR475" },
			{ OMX_AUDIO_AMRBandModeNB1, "MR515", "MR525" },
			{ OMX_AUDIO_AMRBandModeNB2, "MR59", "MR59" },
			{ OMX_AUDIO_AMRBandModeNB3, "MR67", "MR67" },
			{ OMX_AUDIO_AMRBandModeNB4, "MR74", "MR74" },
			{ OMX_AUDIO_AMRBandModeNB5, "MR795", "MR795" },
			{ OMX_AUDIO_AMRBandModeNB6, "MR102", "MR102" },
			{ OMX_AUDIO_AMRBandModeNB7, "MR122", "MR122" },
			{ 0, NULL, NULL },
		};

		type = g_enum_register_static
			("GstGooEncNbAmrBandMode", values);
	}

	return type;
}

#define GST_GOO_ENCNBAMR_DTXMODE_TYPE \
	(gst_goo_encnbamr_dtxmode_get_type ())

static GType
gst_goo_encnbamr_dtxmode_get_type ()
{
	static GType type = 0;

	if (type == 0)
	{
		static const GEnumValue values[] = {
			{ OMX_AUDIO_AMRDTXModeOff, "Off", "Off" },
			{ OMX_AUDIO_AMRDTXModeOnVAD1, "VAD1",
			  "Voice Activity Detector 1" },
			{ OMX_AUDIO_AMRDTXModeOnVAD2, "VAD2",
			  "Voice Activity Detector 2 (VAD2)" },
			{ OMX_AUDIO_AMRDTXModeOnAuto, "Auto",
			  "Auto select Off, VAD1 or VAD2" },
			{ OMX_AUDIO_AMRDTXasEFR, "EFR",
			  "DTX as EFR instead of AMR standard" },
			{ 0, NULL, NULL },
		};

		type = g_enum_register_static
			("GstGooEncNbAmrDtxMode", values);
	}

	return type;
}

/* default values */
#define DEFAULT_BANDMODE OMX_AUDIO_AMRBandModeNB7
#define DEFAULT_DTXMODE OMX_AUDIO_AMRDTXModeOnAuto
#define DEFAULT_MIME TRUE
#define DEFAULT_MUX FALSE
#define DEFAULT_NUM_INPUT_BUFFERS  1
#define DEFAULT_NUM_OUTPUT_BUFFERS 1

enum _GstGooEncNbAmrProp
{
	PROP_0,
	PROP_BANDMODE,
	PROP_DTXMODE,
	PROP_MIME,
	PROP_MUX,
	PROP_NUM_INPUT_BUFFERS,
	PROP_NUM_OUTPUT_BUFFERS
};

static const GstElementDetails details =
	GST_ELEMENT_DETAILS ("OpenMAX NB-AMR encoder",
			     "Codec/Decoder/Audio",
			     "Encodes Adaptative Multi-Rate Narrow-Band streams with OpenMAX",
			     "Texas Instruments"
		);

static GstStaticPadTemplate sink_template =
	GST_STATIC_PAD_TEMPLATE ("sink",
				 GST_PAD_SINK,
				 GST_PAD_ALWAYS,
				 GST_STATIC_CAPS ("audio/x-raw-int, "
						  "width = (int) 16, "
						  "depth = (int) 16, "
						  "signed = (boolean) TRUE, "
						  "endianness = (int) BYTE_ORDER, "
						  "rate = (int) 8000,"
						  "channels = (int) 1")
		);

static GstStaticPadTemplate src_template =
	GST_STATIC_PAD_TEMPLATE ("src",
				 GST_PAD_SRC,
				 GST_PAD_ALWAYS,
				 GST_STATIC_CAPS ("audio/AMR, "
						  "rate = (int) 8000, "
						  "channels = (int) 1")
		);

GST_DEBUG_CATEGORY_STATIC (gst_goo_encnbamr_debug);
#define GST_CAT_DEFAULT gst_goo_encnbamr_debug

#define _do_init(blah) \
	GST_DEBUG_CATEGORY_INIT (gst_goo_encnbamr_debug, "gooencnbamr", 0, "OpenMAX Adaptative Multi-Rate Narrow-Band element");

GST_BOILERPLATE_FULL (GstGooEncNbAmr, gst_goo_encnbamr,
		      GstElement, GST_TYPE_ELEMENT, _do_init);

/* for debugging:
 */
static guint duration_in = 0;
static guint duration_out = 0;


static GstFlowReturn
process_output_buffer (GstGooEncNbAmr* self, OMX_BUFFERHEADERTYPE* buffer)
{
	GstBuffer* out = NULL;
	GstFlowReturn ret = GST_FLOW_ERROR;
	guint bytes_pending;
	GstClockTime duration;

	if (buffer->nFilledLen <= 0)
	{
		GST_INFO_OBJECT (self, "Received an empty buffer!");
		goo_component_release_buffer (self->component, buffer);
		return GST_FLOW_OK;
	}

	GST_DEBUG_OBJECT (self, "outcount = %d", self->outcount);

	GST_OBJECT_LOCK (self);
	bytes_pending = self->bytes_pending;
	self->bytes_pending = 0;
	GST_OBJECT_UNLOCK (self);

	if (G_UNLIKELY (self->outcount == 0 && self->mime == TRUE &&
			self->mux == FALSE))
	{
		GST_INFO_OBJECT (self, "Sending a MIME header");
		const gchar data[] = { 0x23, 0x21, 0x41, 0x4d, 0x52, 0x0a };
		out = gst_buffer_new_and_alloc (6 + buffer->nFilledLen);
		memcpy (GST_BUFFER_DATA (out), data, 6);
		memcpy (GST_BUFFER_DATA (out) + 6,
			buffer->pBuffer, buffer->nFilledLen);
		goo_component_release_buffer (self->component, buffer);
	}
	else
	{
#if 0
 		out = gst_goo_buffer_new ();
 		gst_goo_buffer_set_data (out, self->component, buffer);
#else
		out = gst_buffer_new_and_alloc (buffer->nFilledLen);
		memmove (GST_BUFFER_DATA (out),
			 buffer->pBuffer, buffer->nFilledLen);
		goo_component_release_buffer (self->component, buffer);
#endif
	}

	if (out != NULL)
	{
		duration  = bytes_pending * self->ns_per_byte;

		GST_BUFFER_DURATION (out) = duration;
		GST_BUFFER_OFFSET (out) = self->outcount++;
		GST_BUFFER_TIMESTAMP (out) = self->ts;
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
	g_return_if_fail (buffer->nFlags != OMX_BUFFERFLAG_DATACORRUPT);

	g_assert (GOO_IS_PORT (port));
	g_assert (buffer != NULL);
	g_assert (GOO_IS_COMPONENT (data));

	GooComponent* component = GOO_COMPONENT (data);
	GstGooEncNbAmr* self = GST_GOO_ENCNBAMR (
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
omx_sync (GstGooEncNbAmr* self)
{
	g_assert (self != NULL);
	g_assert (self->component != NULL);

	OMX_AUDIO_PARAM_AMRTYPE* param;
	param = GOO_TI_NBAMRENC_GET_OUTPUT_PORT_PARAM (self->component);

	param->eAMRFrameFormat = (self->mime == TRUE) ?
		OMX_AUDIO_AMRFrameFormatFSF :
		OMX_AUDIO_AMRFrameFormatConformance;
	param->eAMRBandMode = self->bandmode;
	param->eAMRDTXMode = self->dtxmode;

	GST_INFO_OBJECT (self, "");

	return;
}

static void
omx_start (GstGooEncNbAmr* self)
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
	}

	if (goo_component_get_state (self->component) == OMX_StateIdle)
	{
		GST_INFO_OBJECT (self, "going to executing");
		goo_component_set_state_executing (self->component);
	}

	return;
}

static void
omx_stop (GstGooEncNbAmr* self)
{
	g_assert (self != NULL);
	g_assert (self->component != NULL);

	if (goo_component_get_state (self->component) == OMX_StateExecuting)
	{
		GST_INFO_OBJECT (self, "going to idle");
		goo_component_set_state_idle (self->component);
	}

	if (goo_component_get_state (self->component) == OMX_StateIdle)
	{
		GST_INFO_OBJECT (self, "going to loaded");
		goo_component_set_state_loaded (self->component);
	}

	return;
}

static void
omx_wait_for_done (GstGooEncNbAmr* self)
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
gst_goo_encnbamr_event (GstPad* pad, GstEvent* event)
{
	GstGooEncNbAmr* self = GST_GOO_ENCNBAMR (gst_pad_get_parent (pad));

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
gst_goo_encnbamr_setcaps (GstPad* pad, GstCaps* caps)
{
	GstGooEncNbAmr *self = GST_GOO_ENCNBAMR (gst_pad_get_parent (pad));
	GstStructure *structure;
	GstCaps *copy;

	structure = gst_caps_get_structure (caps, 0);

	/* get channel count */
	gst_structure_get_int (structure, "channels", &self->channels);
	gst_structure_get_int (structure, "rate", &self->rate);

	/* this is not wrong but will sound bad */
	if (self->channels != 1)
	{
		g_warning ("gooencnbamr is only optimized for mono channels");
	}

	if (self->rate != 8000)
	{
		g_warning ("gooencnbamr is only optimized for 8000 Hz");
	}

	/* create reverse caps */
	copy = gst_caps_new_simple ("audio/AMR",
				    "channels", G_TYPE_INT, self->channels,
				    "rate", G_TYPE_INT, self->rate, NULL);


//	gint outputframes = 0;
//	g_object_get (self->component, "frames-buffer", &outputframes, NULL);
//	GST_DEBUG_OBJECT (self, "frames-buffer=%d", outputframes);

	/* precalc duration as it's constant now */
//	self->duration =
//		gst_util_uint64_scale_int (160, GST_SECOND,
//					   self->rate * self->channels);

	{
		guint nBitPerSample=16;
		guint bytes_per_second = nBitPerSample / 8;
		bytes_per_second *= self->channels ;
		bytes_per_second *= self->rate;
		self->ns_per_byte = (gint)(1000000000.0 / (gfloat)bytes_per_second);
	}

	gst_pad_set_caps (self->srcpad, copy);
	gst_caps_unref (copy);

	omx_start (self);
	gst_object_unref (self);

	GST_DEBUG_OBJECT (self, "");

	return TRUE;
}

static GstFlowReturn
gst_goo_encnbamr_chain (GstPad* pad, GstBuffer* buffer)
{
	GstGooEncNbAmr* self = GST_GOO_ENCNBAMR (gst_pad_get_parent (pad));
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

	ret = GST_FLOW_OK;
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
	return ret;

	/* ERRORS */
not_negotiated:
	{
		GST_ELEMENT_ERROR (self, CORE, NEGOTIATION, (NULL),
				   ("format wasn't negotiated before chain function"));
		ret = GST_FLOW_NOT_NEGOTIATED;
		goto done;
	}
}

static GstStateChangeReturn
gst_goo_encnbamr_state_change (GstElement* element, GstStateChange transition)
{
	g_assert (GST_IS_GOO_ENCNBAMR (element));

	GstGooEncNbAmr* self = GST_GOO_ENCNBAMR (element);
	GstStateChangeReturn ret;

	switch (transition)
	{
	case GST_STATE_CHANGE_READY_TO_PAUSED:
		self->rate = 0;
		self->channels = 0;
		self->bytes_pending = 0;
		self->ts = 0;

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
gst_goo_encnbamr_set_property (GObject* object, guint prop_id,
			       const GValue* value, GParamSpec* pspec)
{
	g_assert (GST_IS_GOO_ENCNBAMR (object));
	GstGooEncNbAmr* self = GST_GOO_ENCNBAMR (object);

	switch (prop_id)
	{
	case PROP_NUM_OUTPUT_BUFFERS:
		g_object_set_property (G_OBJECT (self->outport),
				       "buffercount", value);
		break;
	case PROP_NUM_INPUT_BUFFERS:
		g_object_set_property (G_OBJECT (self->inport),
				       "buffercount", value);
		break;
	case PROP_MIME:
		self->mime = g_value_get_boolean (value);
		break;
	case PROP_BANDMODE:
		self->bandmode = g_value_get_enum (value);
		break;
	case PROP_DTXMODE:
		self->dtxmode = g_value_get_enum (value);
		break;
	case PROP_MUX:
		self->mux = g_value_get_boolean (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_encnbamr_get_property (GObject* object, guint prop_id,
			       GValue* value, GParamSpec* pspec)
{
	g_assert (GST_IS_GOO_ENCNBAMR (object));
	GstGooEncNbAmr* self = GST_GOO_ENCNBAMR (object);

	switch (prop_id)
	{
	case PROP_NUM_OUTPUT_BUFFERS:
		g_object_get_property (G_OBJECT (self->outport),
				       "buffercount", value);
		break;
	case PROP_NUM_INPUT_BUFFERS:
		g_object_get_property (G_OBJECT (self->inport),
				       "buffercount", value);
		break;
	case PROP_MIME:
		g_value_set_boolean (value, self->mime);
		break;
	case PROP_BANDMODE:
		g_value_set_enum (value, self->bandmode);
		break;
	case PROP_DTXMODE:
		g_value_set_enum (value, self->dtxmode);
		break;
	case PROP_MUX:
		g_value_set_boolean (value, self->mux);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_encnbamr_dispose (GObject* object)
{
	G_OBJECT_CLASS (parent_class)->dispose (object);

	GstGooEncNbAmr* self = GST_GOO_ENCNBAMR (object);

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
gst_goo_encnbamr_base_init (gpointer g_klass)
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
gst_goo_encnbamr_class_init (GstGooEncNbAmrClass* klass)
{
	GObjectClass* g_klass = G_OBJECT_CLASS (klass);
	GstElementClass* gst_klass = GST_ELEMENT_CLASS (klass);

	g_klass->set_property = gst_goo_encnbamr_set_property;
	g_klass->get_property = gst_goo_encnbamr_get_property;
	g_klass->dispose = gst_goo_encnbamr_dispose;

	gst_klass->change_state =
		GST_DEBUG_FUNCPTR (gst_goo_encnbamr_state_change);

	GParamSpec* spec;
	spec = g_param_spec_boolean ("mime", "Mime",
				     "Generate MIME typed output", FALSE,
				     G_PARAM_READWRITE  | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_MIME, spec);

	spec = g_param_spec_enum ("band-mode", "Band Mode",
				  "Encoding band mode (Kbps)",
				  GST_GOO_ENCNBAMR_BANDMODE_TYPE,
				  DEFAULT_BANDMODE,
				  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_BANDMODE, spec);

	spec = g_param_spec_enum ("dtx-mode", "DTX Mode",
				  "Encoding discontinous transmission mode",
				  GST_GOO_ENCNBAMR_DTXMODE_TYPE,
				  DEFAULT_DTXMODE,
				  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_DTXMODE, spec);

	spec = g_param_spec_boolean ("mux", "Mux Enable",
				"The component is going to be muxed", FALSE,
				G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_MUX, spec);

	spec = g_param_spec_uint ("input-buffers", "Input buffers",
				  "The number of OMX input buffers",
				  1, 10, DEFAULT_NUM_INPUT_BUFFERS,
				  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_NUM_INPUT_BUFFERS,
					 spec);

	spec = g_param_spec_uint ("output-buffers", "Output buffers",
				  "The number of OMX output buffers",
				  1, 10, DEFAULT_NUM_OUTPUT_BUFFERS,
				  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_NUM_OUTPUT_BUFFERS,
					 spec);

	return;
}

static void
gst_goo_encnbamr_init (GstGooEncNbAmr* self, GstGooEncNbAmrClass* klass)
{
	/* goo stuff */
	self->factory = goo_ti_component_factory_get_instance ();
	self->component =
		goo_component_factory_get_component (self->factory,
						     GOO_TI_NBAMR_ENCODER);

	self->mime = DEFAULT_MIME;
	self->bandmode = DEFAULT_BANDMODE;
	self->dtxmode = DEFAULT_DTXMODE;
	self->mux = DEFAULT_MUX;

	self->inport = goo_component_get_port (self->component, "input0");
	g_assert (self->inport != NULL);

	self->outport = goo_component_get_port (self->component, "output0");
	g_assert (self->outport != NULL);

	/* gst stuff */
	self->sinkpad = gst_pad_new_from_static_template (&sink_template,
							  "sink");
	gst_pad_set_setcaps_function
		(self->sinkpad, GST_DEBUG_FUNCPTR (gst_goo_encnbamr_setcaps));
	gst_pad_set_event_function
		(self->sinkpad, GST_DEBUG_FUNCPTR (gst_goo_encnbamr_event));
	gst_pad_set_chain_function
		(self->sinkpad, GST_DEBUG_FUNCPTR (gst_goo_encnbamr_chain));
	gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);

	self->srcpad = gst_pad_new_from_static_template (&src_template,
							 "src");
	gst_pad_use_fixed_caps (self->srcpad);
	gst_element_add_pad (GST_ELEMENT (self), self->srcpad);

	/* gobject stuff */
	g_object_set_data (G_OBJECT (self->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", self->component);

	self->adapter = gst_goo_adapter_new ();

	GST_DEBUG_OBJECT (self, "");

        return;
}
