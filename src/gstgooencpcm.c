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

#include <goo-ti-pcmenc.h>
#include "gstgooencpcm.h"
#include "gstgoobuffer.h"
#include "gstgooutils.h"

#include <string.h>

/* default values */
#define DEFAULT_NUM_INPUT_BUFFERS  1
#define DEFAULT_NUM_OUTPUT_BUFFERS 1

enum _GstGooEncPcmProp
{
	PROP_0,
	PROP_NUM_INPUT_BUFFERS,
	PROP_NUM_OUTPUT_BUFFERS
};

#define GST_GOO_ENCPCM_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_GOO_ENCPCM, GstGooEncPcmPrivate))

struct _GstGooEncPcmPrivate
{
	guint incount;
	guint outcount;

};

static const GstElementDetails details =
	GST_ELEMENT_DETAILS ("OpenMAX PCM encoder",
			     "Codec/Decoder/Audio",
			     "Encodes Pulse-Code Modulation streams with OpenMAX",
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
						  "rate = (int) [ 8000, 48000 ], "
						  "channels = (int) 1")
		);

static GstStaticPadTemplate src_template =
	GST_STATIC_PAD_TEMPLATE ("src",
				 GST_PAD_SRC,
				 GST_PAD_ALWAYS,
				 GST_STATIC_CAPS ("audio/x-raw-int, "
						  "rate = (int) [ 8000, 48000 ], "
						  "channels = (int) 1")
		);

GST_DEBUG_CATEGORY_STATIC (gst_goo_encpcm_debug);
#define GST_CAT_DEFAULT gst_goo_encpcm_debug

#define _do_init(blah) \
	GST_DEBUG_CATEGORY_INIT (gst_goo_encpcm_debug, "gooencpcm", 0, "OpenMAX Pulse-Code Modulation element");

GST_BOILERPLATE_FULL (GstGooEncPcm, gst_goo_encpcm,
		      GstElement, GST_TYPE_ELEMENT, _do_init);

static GstFlowReturn
process_output_buffer (GstGooEncPcm* self, OMX_BUFFERHEADERTYPE* buffer)
{
	GstBuffer* out = NULL;
	GstFlowReturn ret = GST_FLOW_ERROR;

	if (buffer->nFilledLen <= 0)
	{
		GST_INFO_OBJECT (self, "Received an empty buffer!");
		goo_component_release_buffer (self->component, buffer);
		return;
	}

	GST_DEBUG_OBJECT (self, "outcount = %d", self->outcount);
	out = gst_buffer_new_and_alloc (buffer->nFilledLen);
	g_assert (out != NULL);
	memmove (GST_BUFFER_DATA (out),
			buffer->pBuffer, buffer->nFilledLen);
	goo_component_release_buffer (self->component, buffer);

	if (out != NULL)
	{
		GST_BUFFER_DURATION (out) = self->duration;
		GST_BUFFER_OFFSET (out) = self->outcount++;
		GST_BUFFER_TIMESTAMP (out) = self->ts;
		if (self->ts != -1)
		{
			self->ts += self->duration;
		}

		gst_buffer_set_caps (out, GST_PAD_CAPS (self->srcpad));

		GST_DEBUG_OBJECT (self, "pushing gst buffer");
		ret = gst_pad_push (self->srcpad, out);
	}

	GST_INFO_OBJECT (self, "");

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
	GstGooEncPcm* self = GST_GOO_ENCPCM (
		g_object_get_data (G_OBJECT (data), "gst")
		);
	g_assert (self != NULL);

	{
		process_output_buffer (self, buffer);

		if (buffer->nFlags == OMX_BUFFERFLAG_EOS ||
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
omx_sync (GstGooEncPcm* self)
{
	g_assert (self != NULL);
	g_assert (self->component != NULL);

	OMX_AUDIO_PARAM_PCMMODETYPE* param;
	param = GOO_TI_PCMENC_GET_OUTPUT_PORT_PARAM (self->component);

	GST_INFO_OBJECT (self, "");

	return;
}

static void
omx_start (GstGooEncPcm* self)
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
omx_stop (GstGooEncPcm* self)
{
	g_assert (self != NULL);
	g_assert (self->component != NULL);

  GST_INFO_OBJECT (self,
									"self->component %x state %d",
									self->component,
									goo_component_get_state (self->component) == OMX_StatePause);
	if (goo_component_get_state (self->component) == OMX_StateExecuting ||
			goo_component_get_state (self->component) == OMX_StatePause)
	{
		GST_INFO_OBJECT (self, "Going to idle");
		goo_component_set_state_idle (self->component);
	}

	if (goo_component_get_state (self->component) == OMX_StateIdle)
	{
		GST_INFO_OBJECT (self, "Going to loaded");
		goo_component_set_state_loaded (self->component);
	}
  GST_INFO_OBJECT (self, "");
	return;
}

static void
omx_wait_for_done (GstGooEncPcm* self)
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
gst_goo_encpcm_event (GstPad* pad, GstEvent* event)
{
	GstGooEncPcm* self = GST_GOO_ENCPCM (gst_pad_get_parent (pad));

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
gst_goo_encpcm_setcaps (GstPad* pad, GstCaps* caps)
{
	GstGooEncPcm *self = GST_GOO_ENCPCM (gst_pad_get_parent (pad));
	GstStructure *structure;
	GstCaps *copy;

	structure = gst_caps_get_structure (caps, 0);

	/* get channel count */
	gst_structure_get_int (structure, "channels", &self->channels);
	gst_structure_get_int (structure, "rate", &self->rate);

	/* this is not wrong but will sound bad */
	if (self->channels != 1)
	{
		g_warning ("audio capture is optimized for mono channels");
	}

	/* create reverse caps */
	copy = gst_caps_new_simple ("audio/x-raw-int",
				    "channels", G_TYPE_INT, self->channels,
				    "rate", G_TYPE_INT, self->rate, NULL);

	/* precalc duration as it's constant now */
	self->duration =
		gst_util_uint64_scale_int (160, GST_SECOND,
					   self->rate * self->channels);

	gst_pad_set_caps (self->srcpad, copy);
	gst_caps_unref (copy);

	omx_start (self);
	gst_object_unref (self);

	GST_DEBUG_OBJECT (self, "");

	return TRUE;
}

static GstFlowReturn
gst_goo_encpcm_chain (GstPad* pad, GstBuffer* buffer)
{

	GstGooEncPcm* self = GST_GOO_ENCPCM (gst_pad_get_parent (pad));
	guint omxbufsiz = GOO_PORT_GET_DEFINITION (self->inport)->nBufferSize;
	GstGooEncPcmPrivate* priv = GST_GOO_ENCPCM_GET_PRIVATE (self);
	GstFlowReturn ret = GST_FLOW_OK;
	OMX_BUFFERHEADERTYPE* omx_buffer = NULL;
  GstBuffer* gst_buffer = NULL;

	GST_DEBUG_OBJECT (self, "");

	if (self->rate == 0 || self->channels == 0 ||
	    self->component->cur_state != OMX_StateExecuting)
	{
		goto not_negotiated;
	}


	/* take latest timestamp, FIXME timestamp is the one of the
	 * first buffer in the adapter. */
	if (GST_BUFFER_TIMESTAMP_IS_VALID (buffer))
	{
		self->ts = GST_BUFFER_TIMESTAMP (buffer);
	}


	if (goo_port_is_tunneled (self->inport))
	{
		GST_DEBUG_OBJECT (self, "Input Port is tunneled (DM)");

		GST_DEBUG_OBJECT (self, "Take the oxm buffer, process it and push it");
		ret = process_output_buffer(self,
																goo_port_grab_buffer (self->outport));
	}
	else
	{
		/* discontinuity clears adapter, FIXME, maybe we can set some
		 * encoder flag to mask the discont. */
		if (GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DISCONT))
		{
			gst_goo_adapter_clear (self->adapter);
			self->ts = 0;
		}




		GST_DEBUG_OBJECT (self, "");
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
	}

	goto done;


	/* ERRORS */
not_negotiated:
	{
		GST_ELEMENT_ERROR (self, STREAM, TYPE_NOT_FOUND,
				   (NULL), ("unknown type"));
		return GST_FLOW_NOT_NEGOTIATED;
	}

done:
  GST_DEBUG_OBJECT (self, "");
	gst_object_unref (self);
	gst_buffer_unref (buffer);
	return ret;
}

static GstStateChangeReturn
gst_goo_encpcm_state_change (GstElement* element, GstStateChange transition)
{
	g_assert (GST_IS_GOO_ENCPCM (element));

	GstGooEncPcm* self = GST_GOO_ENCPCM (element);
	GstStateChangeReturn ret;

	switch (transition)
	{
	case GST_STATE_CHANGE_READY_TO_PAUSED:
		GST_DEBUG_OBJECT (self, "GST_STATE_CHANGE_READY_TO_PAUSED");
		self->rate = 0;
		self->channels = 0;
		self->ts = 0;
		self->outcount = 0;
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
		GST_OBJECT_LOCK (self);
		goo_component_set_state_pause (self->component);
		GST_OBJECT_UNLOCK (self);
		break;

	case GST_STATE_CHANGE_PAUSED_TO_READY:
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
gst_goo_encpcm_set_property (GObject* object, guint prop_id,
			       const GValue* value, GParamSpec* pspec)
{
	g_assert (GST_IS_GOO_ENCPCM (object));
	GstGooEncPcm* self = GST_GOO_ENCPCM (object);

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
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_encpcm_get_property (GObject* object, guint prop_id,
			       GValue* value, GParamSpec* pspec)
{
	g_assert (GST_IS_GOO_ENCPCM (object));
	GstGooEncPcm* self = GST_GOO_ENCPCM (object);

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
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_encpcm_dispose (GObject* object)
{
	G_OBJECT_CLASS (parent_class)->dispose (object);

	GstGooEncPcm* self = GST_GOO_ENCPCM (object);

  if (G_LIKELY (self->adapter))
	{
		GST_DEBUG ("unrefing adapter");
		g_object_unref (self->adapter);
	}

	if (G_LIKELY (self->inport))
	{
		GST_DEBUG ("unrefing inport");
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

	return;
}

static void
gst_goo_encpcm_base_init (gpointer g_klass)
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
gst_goo_encpcm_class_init (GstGooEncPcmClass* klass)
{
	GObjectClass* g_klass = G_OBJECT_CLASS (klass);
	GstElementClass* gst_klass = GST_ELEMENT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GstGooEncPcmPrivate));

	g_klass->set_property = gst_goo_encpcm_set_property;
	g_klass->get_property = gst_goo_encpcm_get_property;
	g_klass->dispose = gst_goo_encpcm_dispose;

	gst_klass->change_state =
		GST_DEBUG_FUNCPTR (gst_goo_encpcm_state_change);

	GParamSpec* spec;

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
gst_goo_encpcm_init (GstGooEncPcm* self, GstGooEncPcmClass* klass)
{
	/* goo stuff */
	self->factory = goo_ti_component_factory_get_instance ();
	self->component =
		goo_component_factory_get_component (self->factory,
						     GOO_TI_PCM_ENCODER);

	self->inport = goo_component_get_port (self->component, "input0");
	g_assert (self->inport != NULL);

	self->outport = goo_component_get_port (self->component, "output0");
	g_assert (self->outport != NULL);

	/* gst stuff */
	self->sinkpad = gst_pad_new_from_static_template (&sink_template,
							  "sink");
	gst_pad_set_setcaps_function
		(self->sinkpad, GST_DEBUG_FUNCPTR (gst_goo_encpcm_setcaps));
	gst_pad_set_event_function
		(self->sinkpad, GST_DEBUG_FUNCPTR (gst_goo_encpcm_event));
	gst_pad_set_chain_function
		(self->sinkpad, GST_DEBUG_FUNCPTR (gst_goo_encpcm_chain));
	gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);

	self->srcpad = gst_pad_new_from_static_template (&src_template,
							 "src");
	gst_pad_use_fixed_caps (self->srcpad);
	gst_element_add_pad (GST_ELEMENT (self), self->srcpad);

	/* gobject stuff */
	g_object_set_data (G_OBJECT (self->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", self->component);

	self->adapter = gst_goo_adapter_new ();

	/* Initializing the private structure */
	GstGooEncPcmPrivate* priv = GST_GOO_ENCPCM_GET_PRIVATE (self);
	priv->incount = 0;
	priv->outcount = 0;

	GST_DEBUG_OBJECT (self, "");

        return;
}
