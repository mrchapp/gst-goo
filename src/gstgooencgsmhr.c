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

#include <goo-ti-gsmhrenc.h>
#include "gstgooencgsmhr.h"
#include "gstgoobuffer.h"

#include <string.h>


/* default values */

#define DEFAULT_DTX					OMX_TRUE
#define DEFAULT_HIPASS_FILTER 		OMX_FALSE
#define DEFAULT_NUM_INPUT_BUFFERS  	1
#define DEFAULT_NUM_OUTPUT_BUFFERS 	1

enum _GstGooEncGsmHrProp
{
	PROP_0,
	PROP_DTX,
	PROP_HIPASS_FILTER,
	PROP_NUM_INPUT_BUFFERS,
	PROP_NUM_OUTPUT_BUFFERS
};

static const GstElementDetails details =
	GST_ELEMENT_DETAILS ("OpenMAX GSM-HR encoder",
			     "Codec/Decoder/Audio",
			     "Encodes GSM HALF RATE streams with OpenMAX",
			     "Texas Instruments"
		);

static GstStaticPadTemplate
sink_template =
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

static GstStaticPadTemplate
src_template =
	GST_STATIC_PAD_TEMPLATE ("src",
				 GST_PAD_SRC,
				 GST_PAD_ALWAYS,
				 GST_STATIC_CAPS ("audio/x-gsm, "
						  "rate = (int) 8000, "
						  "channels = (int) 1")
		);

GST_DEBUG_CATEGORY_STATIC (gst_goo_encgsmhr_debug);

#define GST_CAT_DEFAULT gst_goo_encgsmhr_debug

#define _do_init(blah) \
	GST_DEBUG_CATEGORY_INIT (gst_goo_encgsmhr_debug, "gooencgsmhr", 0,\
	"OpenMAX GSM Half Rate element");

GST_BOILERPLATE_FULL (GstGooEncGsmHr, gst_goo_encgsmhr,
			GstElement, GST_TYPE_ELEMENT, _do_init);

static GstFlowReturn
process_output_buffer (GstGooEncGsmHr* self,
			OMX_BUFFERHEADERTYPE* buffer)
{
	GstBuffer* out = NULL;
	GstFlowReturn ret = GST_FLOW_ERROR;

	if (buffer->nFilledLen <= 0)
	{
		GST_INFO_OBJECT (self, "Received an empty buffer!");
		goo_component_release_buffer (self->component, buffer);
		return GST_FLOW_OK;
	}
	else
	{
		out = gst_buffer_new_and_alloc (buffer->nFilledLen);
		memmove (GST_BUFFER_DATA (out),
					buffer->pBuffer, buffer->nFilledLen);
		goo_component_release_buffer (self->component, buffer);
	}

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
omx_output_buffer_cb (GooPort* port, OMX_BUFFERHEADERTYPE* buffer,
		gpointer data)
{
	g_return_if_fail (buffer->nFlags != OMX_BUFFERFLAG_DATACORRUPT);
	g_assert (GOO_IS_PORT (port));
	g_assert (buffer != NULL);
	g_assert (GOO_IS_COMPONENT (data));

	GooComponent* component = GOO_COMPONENT (data);
	GstGooEncGsmHr* self = GST_GOO_ENCGSMHR (
			g_object_get_data (G_OBJECT (data), "gst"));
	g_assert (self != NULL);

	{
		process_output_buffer (self, buffer);

		if (buffer->nFlags & OMX_BUFFERFLAG_EOS == 0x1 ||
		    goo_port_is_eos (port))
		{
			GST_INFO_OBJECT (self, "EOS found in output buffer (%d)",
					 buffer->nFilledLen);
			goo_component_set_done (self->component);
		}
	}

	GST_INFO_OBJECT (self, "");
	return;
}

static void omx_sync (GstGooEncGsmHr* self)
{
	g_assert (self != NULL);
	g_assert (self->component != NULL);

	OMX_AUDIO_PARAM_GSMHRTYPE* param;
	param = GOO_TI_GSMHRENC_GET_OUTPUT_PORT_PARAM (self->component);

	param->bDTX = self->bDTX;
	param->bHiPassFilter = self->bHiPassFilter;

	GST_INFO_OBJECT (self, "");
	return;
}

static void omx_start (GstGooEncGsmHr* self)
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

static void omx_stop (GstGooEncGsmHr* self)
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

static void omx_wait_for_done (GstGooEncGsmHr* self)
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
gst_goo_encgsmhr_event (GstPad* pad, GstEvent* event)
{
	GstGooEncGsmHr* self = GST_GOO_ENCGSMHR (gst_pad_get_parent (pad));

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
gst_goo_encgsmhr_setcaps (GstPad* pad, GstCaps* caps)
{
	GstGooEncGsmHr *self = GST_GOO_ENCGSMHR (gst_pad_get_parent (pad));
	GstStructure *structure;
	GstCaps *copy;

	structure = gst_caps_get_structure (caps, 0);

	self->duration =
		gst_util_uint64_scale_int (160, GST_SECOND, 1); /* TODO: ver que
									que ondas con lo de channel y rate */

	gst_pad_set_caps (self->srcpad, copy);
	gst_caps_unref (copy);

	omx_start (self);
	gst_object_unref (self);

	GST_DEBUG_OBJECT (self, "");

	return TRUE;
}

static GstFlowReturn
gst_goo_encgsmhr_chain (GstPad* pad, GstBuffer* buffer)
{
	GstGooEncGsmHr* self = GST_GOO_ENCGSMHR (gst_pad_get_parent (pad));
	guint omxbufsiz = GOO_PORT_GET_DEFINITION (self->inport)->nBufferSize;
	GstFlowReturn ret;

	if (self->component->cur_state != OMX_StateExecuting)
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

static GstStateChangeReturn
gst_goo_encgsmhr_state_change (GstElement* element,
					GstStateChange transition)
{
	g_assert (GST_IS_GOO_ENCGSMHR (element));

	GstGooEncGsmHr* self = GST_GOO_ENCGSMHR (element);
	GstStateChangeReturn ret;

	switch (transition)
	{
	case GST_STATE_CHANGE_READY_TO_PAUSED:
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
gst_goo_encgsmhr_set_property (GObject* object, guint prop_id,
					const GValue* value, GParamSpec* pspec)
{
	g_assert (GST_IS_GOO_ENCGSMHR (object));
	GstGooEncGsmHr* self = GST_GOO_ENCGSMHR (object);

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
	case PROP_DTX:
		self->bDTX = g_value_get_boolean (value);
		break;
	case PROP_HIPASS_FILTER:
		self->bHiPassFilter = g_value_get_boolean (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_encgsmhr_get_property (GObject* object, guint prop_id,
					GValue* value, GParamSpec* pspec)
{
	g_assert (GST_IS_GOO_ENCGSMHR (object));
	GstGooEncGsmHr* self = GST_GOO_ENCGSMHR (object);

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
	case PROP_DTX:
		g_value_set_boolean (value, self->bDTX);
		break;
	case PROP_HIPASS_FILTER:
		g_value_set_boolean (value, self->bHiPassFilter);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_encgsmhr_dispose (GObject* object)
{
	G_OBJECT_CLASS (parent_class)->dispose (object);

	GstGooEncGsmHr* self = GST_GOO_ENCGSMHR (object);

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
gst_goo_encgsmhr_base_init (gpointer g_klass)
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
gst_goo_encgsmhr_class_init (GstGooEncGsmHrClass* klass)
{
	GObjectClass* g_klass = G_OBJECT_CLASS (klass);
	GstElementClass* gst_klass = GST_ELEMENT_CLASS (klass);

	g_klass->set_property = gst_goo_encgsmhr_set_property;
	g_klass->get_property = gst_goo_encgsmhr_get_property;
	g_klass->dispose = gst_goo_encgsmhr_dispose;

	gst_klass->change_state =
		GST_DEBUG_FUNCPTR (gst_goo_encgsmhr_state_change);

	GParamSpec* spec;
	spec = g_param_spec_boolean ("bDTX", "DTX",
					"Enable Discontinuous Transmisssion ",
					DEFAULT_DTX,
					G_PARAM_READWRITE  | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_DTX, spec);

	spec = g_param_spec_boolean ("bHiPassFilter", "HiPass Filter",
					"Enable HiPass Filter ",
					DEFAULT_HIPASS_FILTER,
					G_PARAM_READWRITE  | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_HIPASS_FILTER,
					spec);

	spec = g_param_spec_uint ("input-buffers", "Input buffers",
					"The number of OMX input buffers",
					1, 1, DEFAULT_NUM_INPUT_BUFFERS,
					G_PARAM_READWRITE  | G_PARAM_CONSTRUCT);
					/*G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY);*/
	g_object_class_install_property (g_klass, PROP_NUM_INPUT_BUFFERS,
					 spec);

	spec = g_param_spec_uint ("output-buffers", "Output buffers",
					"The number of OMX output buffers",
					1, 1, DEFAULT_NUM_OUTPUT_BUFFERS,
					G_PARAM_READWRITE  | G_PARAM_CONSTRUCT);
					/*G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY);*/
	g_object_class_install_property (g_klass, PROP_NUM_OUTPUT_BUFFERS,
					spec);

	return;
}

static void
gst_goo_encgsmhr_init (GstGooEncGsmHr* self, GstGooEncGsmHrClass* klass)
{
	/* goo stuff */
	self->factory = goo_ti_component_factory_get_instance ();
	self->component =
		goo_component_factory_get_component (self->factory,
						     GOO_TI_GSMHR_ENCODER);

	self->bDTX = DEFAULT_DTX;
	self->bHiPassFilter = DEFAULT_HIPASS_FILTER;

	self->inport = goo_component_get_port (self->component, "input0");
	g_assert (self->inport != NULL);

	self->outport = goo_component_get_port (self->component, "output0");
	g_assert (self->outport != NULL);

	/* gst stuff */
	self->sinkpad = gst_pad_new_from_static_template (&sink_template,
							  "sink");
	gst_pad_set_setcaps_function
		(self->sinkpad, GST_DEBUG_FUNCPTR (gst_goo_encgsmhr_setcaps));
	gst_pad_set_event_function
		(self->sinkpad, GST_DEBUG_FUNCPTR (gst_goo_encgsmhr_event));
	gst_pad_set_chain_function
		(self->sinkpad, GST_DEBUG_FUNCPTR (gst_goo_encgsmhr_chain));
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
