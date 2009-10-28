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

#include "gstgoobuffer.h"
#include "gstgoofilter.h"
#include <string.h>

#define CONVERSION 1000000

GST_DEBUG_CATEGORY_STATIC (gst_goo_filter_debug);
#define GST_CAT_DEFAULT gst_goo_filter_debug
#define DEFAULT_PROCESS_MODE FRAMEMODE

/* signals */
enum
{
        LAST_SIGNAL
};

/* args */
enum
{
	PROP_0,
	PROP_NUM_INPUT_BUFFERS,
	PROP_NUM_OUTPUT_BUFFERS,
	PROP_PROCESSMODE
};

typedef enum ProcessMode
{
	FRAMEMODE,
	STREAMMODE

} GstGooFilterProcessMode;

static gboolean gst_goo_filter_src_setcaps (GstPad *pad, GstCaps *caps);
static gboolean gst_goo_filter_sink_setcaps (GstPad *pad, GstCaps *caps);

GstBuffer* gst_goo_filter_codec_data_processing (GstGooFilter* self, GstBuffer *buffer);
GstBuffer* gst_goo_filter_extra_buffer_processing (GstGooFilter* self, GstBuffer *buffer);

/** FIXME GStreamer should not insert the header.  OMX component should take
 * care of it.  Remove this function upon resolution of DR OMAPS00140835 and
 * OMAPS00140836 **/
GstBuffer* gst_goo_filter_insert_header (GstGooFilter* self, GstBuffer *buffer, guint counter);

static gboolean gst_goo_filter_is_last_dasf_buffer (GstGooFilter *self, guint counter);
#define GST_TYPE_PROCESS_MODE (gst_goo_filter_process_mode_get_type())

static GType
gst_goo_filter_process_mode_get_type ()
{
	static GType goo_filter_process_mode_type = 0;
	static GEnumValue goo_filter_process_mode[] =
	{
		{0, "0", "Frame Mode"},
		{1, "1", "Stream Mode"},
		{0, NULL, NULL},
	};
	if (!goo_filter_process_mode_type)
	{
		goo_filter_process_mode_type = g_enum_register_static ("GstGooFilterProcessMode", goo_filter_process_mode);
	}

	return goo_filter_process_mode_type;
}

#define GST_GOO_FILTER_GET_PRIVATE(obj)					\
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_GOO_FILTER, GstGooFilterPrivate))

struct _GstGooFilterPrivate
{
	guint num_input_buffers;
	guint num_output_buffers;

	guint incount;
	guint outcount;

	guint process_mode;
};

#define NUM_INPUT_BUFFERS_DEFAULT 1
#define NUM_OUTPUT_BUFFERS_DEFAULT 1

/**
 * gst_goo_filter_outport_buffer:
 * @port: A #GooPort instance
 * @buffer: An #OMX_BUFFERHEADERTYPE pointer
 * @data: A pointer to extra data
 *
 * This function is a generic callback for a libgoo's output port and push
 * a new GStreamer's buffer.
 *
 * This method can be reused in derived classes.
 **/
void
gst_goo_filter_outport_buffer (GooPort* port, OMX_BUFFERHEADERTYPE* buffer,
				  gpointer data)
{
	GST_DEBUG ("Enter");
	g_assert (buffer != NULL);
	g_assert (GOO_IS_PORT (port));
	g_assert (GOO_IS_COMPONENT (data));

	g_return_if_fail (buffer->nFlags != OMX_BUFFERFLAG_DATACORRUPT);

	GooComponent* component = GOO_COMPONENT (data);
	GstGooFilter* self =
		GST_GOO_FILTER (g_object_get_data (G_OBJECT (data), "gst"));
	g_assert (self != NULL);
	GstGooFilterPrivate* priv = GST_GOO_FILTER_GET_PRIVATE (self);

	GstBuffer* gst_buffer = gst_goo_buffer_new ();
	gst_goo_buffer_set_data (gst_buffer, component, buffer);
	priv->outcount++;

#if 0
	if (goo_port_is_tunneled (self->inport))
	{
		GST_DEBUG_OBJECT (self, "sem up");
         gst_goo_sem_up (self->dasfsrc_sem);
	}
#endif

	/** FIXME GStreamer should not insert the header.  OMX component should take
	 * care of it.  Remove this function upon resolution of DR OMAPS00140835 and
	 * OMAPS00140836 **/
	gst_buffer = gst_goo_filter_insert_header (self, gst_buffer, priv->outcount);
	g_assert (gst_buffer != NULL);

	gst_goo_filter_timestamp_buffer (self, gst_buffer, buffer);

	GST_BUFFER_OFFSET (gst_buffer) = priv->outcount;
	gst_buffer_set_caps (gst_buffer, GST_PAD_CAPS (self->srcpad));
	gst_pad_push (self->srcpad, gst_buffer);

	if (buffer->nFlags == OMX_BUFFERFLAG_EOS || goo_port_is_eos (port) ||
		gst_goo_filter_is_last_dasf_buffer (self, priv->outcount))
	{
		GST_INFO ("EOS flag found in output buffer (%d)",
			  buffer->nFilledLen);
		goo_component_set_done (self->component);
	}

	return;
}

static void
gst_goo_filter_wait_for_done (GstGooFilter* self)
{
	g_assert (self != NULL);
	GstGooFilterPrivate* priv = GST_GOO_FILTER_GET_PRIVATE (self);

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
		omx_buffer->nFlags = OMX_BUFFERFLAG_EOS;
		/** @todo timestamp the buffers */
		priv->incount++;
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

	if (goo_port_is_tunneled (self->outport))
	{
		GST_INFO ("Outport is tunneled: Setting done");
		goo_component_set_done (self->component);
	}
	else
	{
		GST_INFO ("Waiting for done signal");
		goo_component_wait_for_done (self->component);
	}

	return;
}

static gboolean
gst_goo_filter_sink_event (GstPad* pad, GstEvent* event)
{
	GST_LOG ("");

	GstGooFilter* self = GST_GOO_FILTER (gst_pad_get_parent (pad));
	GstGooFilterPrivate* priv = GST_GOO_FILTER_GET_PRIVATE (self);

	gboolean ret;

	g_assert (self->component != NULL);

	switch (GST_EVENT_TYPE (event))
	{
		case GST_EVENT_NEWSEGMENT:
			GST_INFO ("New segement event");
			ret = gst_pad_push_event (self->srcpad, event);
			break;
		case GST_EVENT_EOS:
			GST_INFO ("EOS event");
			gst_goo_filter_wait_for_done (self);
			ret = gst_pad_push_event (self->srcpad, event);
			break;
		default:
			ret = gst_pad_event_default (pad, event);
			break;
	}

	gst_object_unref (self);
	return ret;
}

static GstFlowReturn
gst_goo_filter_chain (GstPad* pad, GstBuffer* buffer)
{
	GST_LOG ("");

	GstGooFilter* self = GST_GOO_FILTER (gst_pad_get_parent (pad));
	GstGooFilterPrivate* priv = GST_GOO_FILTER_GET_PRIVATE (self);
	GstGooAdapter* adapter = self->adapter;
	GstFlowReturn ret = GST_FLOW_OK;
	static OMX_S64 omx_normalize_timestamp;

	GstClockTime timestamp = GST_BUFFER_TIMESTAMP (buffer);

	if (priv->incount == 0)
	{
		omx_normalize_timestamp	= (gint64)timestamp / CONVERSION;
	}

	if (goo_port_is_tunneled (self->inport))
	{
		/* shall we send a ghost buffer here ? */
		GST_INFO ("port is tunneled");
		ret = GST_FLOW_OK;

		GST_DEBUG_OBJECT (self, "Buffer timestamp: time %" GST_TIME_FORMAT,
						GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buffer)));

		GST_DEBUG_OBJECT (self, "Buffer duration: %" GST_TIME_FORMAT, GST_TIME_ARGS (GST_BUFFER_DURATION (buffer)));

		GST_DEBUG_OBJECT (self, "Pushing buffer to next element. Size =%d", GST_BUFFER_SIZE (buffer));

		/** FIXME GStreamer should not insert the header.  OMX component should take
		 * care of it.	Remove this function upon resolution of DR OMAPS00140835 and
		 * OMAPS00140836 **/
 		priv->outcount++;
		buffer = gst_goo_filter_insert_header (self, buffer, priv->outcount);

		gst_buffer_set_caps (buffer, GST_PAD_CAPS (self->srcpad));
		gst_pad_push (self->srcpad, buffer);

		goto done;
	}

	if (goo_port_is_eos (self->inport))
	{
		GST_INFO ("port is eos");
		ret = GST_FLOW_UNEXPECTED;
		goto fail;
	}

	/** @todo GstGooAdapter! */
	if (GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DISCONT)) {
		gst_goo_adapter_clear (adapter);
	}

	if (priv->incount == 0 &&
	    goo_component_get_state (self->component) == OMX_StateLoaded)
	{

		/** Some filters require header processing,
			apended to the first buffer **/
		buffer = gst_goo_filter_codec_data_processing (self, GST_BUFFER (buffer));

		/** Todo: Use the gst_caps_fixatecaps_func to make
		    this cleaner **/
		if (!gst_goo_filter_check_fixed_src_caps (self))
			return GST_FLOW_NOT_NEGOTIATED;
		/** Remove gst_goo_filter_check_fixed_src_caps function when fixed **/

		g_object_set (self->inport,
				"buffercount",
				priv->num_input_buffers, NULL);
		g_object_set (self->outport,
				"buffercount",
				priv->num_output_buffers, NULL);

		GST_INFO ("going to idle");
		goo_component_set_state_idle (self->component);
		GST_INFO ("going to executing");
		goo_component_set_state_executing (self->component);

	}

	/** Function to perform post buffering processing **/
	buffer = gst_goo_filter_extra_buffer_processing (self, GST_BUFFER (buffer));
	g_assert (buffer != NULL);

	gst_goo_adapter_push (adapter, buffer);

	if (self->component->cur_state != OMX_StateExecuting)
	{
		goto done;
	}

	int omxbufsiz;

	if (priv->process_mode == STREAMMODE)
		omxbufsiz = GOO_PORT_GET_DEFINITION (self->inport)->nBufferSize;
	else
		omxbufsiz = GST_BUFFER_SIZE (buffer);

	while (gst_goo_adapter_available (adapter) >= omxbufsiz &&
	       ret == GST_FLOW_OK && omxbufsiz != 0)
	{

		GST_DEBUG ("Adapter available =%d  omxbufsiz = %d", gst_goo_adapter_available (adapter), omxbufsiz);

		OMX_BUFFERHEADERTYPE* omx_buffer;
		omx_buffer = goo_port_grab_buffer (self->inport);
		GST_DEBUG ("memcpy to buffer %d bytes", omxbufsiz);
		gst_goo_adapter_peek (adapter, omxbufsiz, omx_buffer);
		omx_buffer->nFilledLen = omxbufsiz;
		gst_goo_adapter_flush (adapter, omxbufsiz);
		/* transfer timestamp to openmax */
		{

			GST_DEBUG_OBJECT (self, "checking timestamp: time %" GST_TIME_FORMAT,
					GST_TIME_ARGS (timestamp));

			if (GST_CLOCK_TIME_IS_VALID (timestamp))
			{
				gint64 buffer_ts = (gint64)timestamp;
				omx_buffer->nTimeStamp = (OMX_S64)buffer_ts / CONVERSION;
				omx_buffer->nTimeStamp = omx_buffer->nTimeStamp - omx_normalize_timestamp;

			} else GST_WARNING_OBJECT (self, "Invalid timestamp!");
		}

		priv->incount++;
		goo_component_release_buffer (self->component, omx_buffer);
		ret = GST_FLOW_OK;
	}

	if (goo_port_is_tunneled (self->outport))
	{
		/** @todo send a ghost buffer */
		GstBuffer *ghost_buffer = (GstBuffer*) gst_ghost_buffer_new ();
		GST_BUFFER_TIMESTAMP (ghost_buffer) = timestamp;
		gst_pad_push (self->srcpad, ghost_buffer);
	}

	goto done;

fail:
	gst_goo_adapter_clear (adapter);
done:
	gst_object_unref (self);
	gst_buffer_unref (buffer);
	return ret;
}

GST_BOILERPLATE (GstGooFilter, gst_goo_filter, GstElement, GST_TYPE_ELEMENT);

static GstStateChangeReturn
gst_goo_filter_change_state (GstElement* element, GstStateChange transition)
{
	GST_LOG ("");

	GstGooFilter* self = GST_GOO_FILTER (element);
	GstStateChangeReturn result;
	GstGooFilterPrivate* priv = GST_GOO_FILTER_GET_PRIVATE (self);

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

	result = GST_ELEMENT_CLASS (parent_class)->change_state (element,
								 transition);

	switch (transition)
	{
	case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
		/* goo_component_set_state_paused (self->component); */
		break;
	case GST_STATE_CHANGE_PAUSED_TO_READY:
		GST_INFO ("going to idle");
		goo_component_set_state_idle (self->component);
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
gst_goo_filter_set_property (GObject* object, guint prop_id,
				const GValue* value, GParamSpec* pspec)
{
	GstGooFilterPrivate* priv = GST_GOO_FILTER_GET_PRIVATE (object);
	GstGooFilter* self = GST_GOO_FILTER (object);

	switch (prop_id)
	{
	case PROP_NUM_INPUT_BUFFERS:
		priv->num_input_buffers = g_value_get_uint (value);
		break;
	case PROP_NUM_OUTPUT_BUFFERS:
		priv->num_output_buffers = g_value_get_uint (value);
		break;
	case PROP_PROCESSMODE:
		priv->process_mode = g_value_get_enum (value);
		gst_goo_filter_set_process_mode (self, priv->process_mode);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_filter_get_property (GObject* object, guint prop_id,
				GValue* value, GParamSpec* pspec)
{
	GstGooFilterPrivate* priv = GST_GOO_FILTER_GET_PRIVATE (object);

	switch (prop_id)
	{
	case PROP_NUM_INPUT_BUFFERS:
		g_value_set_uint (value, priv->num_input_buffers);
		break;
	case PROP_NUM_OUTPUT_BUFFERS:
		g_value_set_uint (value, priv->num_output_buffers);
		break;
	case PROP_PROCESSMODE:
		g_value_set_enum (value, priv->process_mode);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_filter_dispose (GObject* object)
{
        GstGooFilter* me;

        G_OBJECT_CLASS (parent_class)->dispose (object);

        me = GST_GOO_FILTER (object);

		if (G_LIKELY (me->adapter))
		{
			GST_DEBUG ("unrefing adapter");
			g_object_unref (me->adapter);
		}

#if 0
		if (G_LIKELY (me->dasfsrc_sem))
		{
			GST_DEBUG ("unrefing dasfsrc_sem");
			gst_goo_sem_free (me->dasfsrc_sem);
		}
#endif

        if (G_LIKELY (me->inport))
        {
		GST_DEBUG ("unrefing inport");
                g_object_unref (me->inport);
        }

        if (G_LIKELY (me->outport))
        {
		GST_DEBUG ("unrefing outport");
                g_object_unref (me->outport);
        }

	if (G_LIKELY (me->component))
        {
		GST_DEBUG ("GOO component = %d",
				  G_OBJECT (me->component)->ref_count);


		GST_DEBUG ("unrefing component");
                g_object_unref (me->component);
        }

	if (G_LIKELY (me->factory))
	{
		GST_DEBUG ("GOO factory = %d",
				  G_OBJECT (me->factory)->ref_count);

		GST_DEBUG ("unrefing factory");
		g_object_unref (me->factory);
	}

        return;
}

static gboolean
gst_goo_filter_check_fixed_src_caps_default (GstGooFilter* self)
{
	return TRUE;
}


static gboolean
gst_goo_filter_timestamp_buffer_default (GstGooFilter* self, GstBuffer *gst_buf, OMX_BUFFERHEADERTYPE* buffer)
{
	/** No timestamp **/
	self->running_time = GST_CLOCK_TIME_NONE;
	GST_BUFFER_DURATION (gst_buf) = GST_CLOCK_TIME_NONE;

	return TRUE;

}

static gboolean
gst_goo_filter_set_process_mode_default (GstGooFilter* self, guint value)
{
	return TRUE;
}

/** FIXME GStreamer should not insert the header.  OMX component should take
 * care of it.  Remove this function upon resolution of DR OMAPS00140835 and
 * OMAPS00140836 **/
static GstBuffer*
gst_goo_filter_insert_header_default (GstGooFilter* self, GstBuffer *buffer, guint counter)
{

	GST_DEBUG ("No need to insert header");
	return GST_BUFFER (buffer);

}

static gboolean
gst_goo_filter_is_last_dasf_buffer_default (GstGooFilter* self, guint counter)
{
	/** This function is within an OR statement.  It needs to return false
 	 *  so that this function is not relevant in the default mode **/

	GST_DEBUG ("No need to set eos by last dasf buffer");
	return FALSE;

}

static GstBuffer*
gst_goo_filter_codec_data_processing_default (GstGooFilter* self, GstBuffer *buffer)
{

	GST_DEBUG ("No need to process codec data");
	return GST_BUFFER (buffer);

}

static GstBuffer*
gst_goo_filter_extra_buffer_processing_default (GstGooFilter* self, GstBuffer *buffer)
{
	return buffer;
}


static void
gst_goo_filter_base_init (gpointer g_klass)
{
        GST_DEBUG_CATEGORY_INIT (gst_goo_filter_debug, "goofilter", 0,
                                 "Gst OpenMax parent class for filters");

	return;
}

static void
gst_goo_filter_class_init (GstGooFilterClass* klass)
{
	GObjectClass* g_klass;
	GParamSpec* pspec;
	GstElementClass* gst_klass;

	/* gobject */
	g_klass = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GstGooFilterPrivate));

	g_klass->set_property =
	GST_DEBUG_FUNCPTR (gst_goo_filter_set_property);
	g_klass->get_property =
	GST_DEBUG_FUNCPTR (gst_goo_filter_get_property);
	g_klass->dispose = GST_DEBUG_FUNCPTR (gst_goo_filter_dispose);

	pspec = g_param_spec_uint ("input-buffers", "Input buffers",
			"The number of OMX input buffers",
			1, 10, NUM_INPUT_BUFFERS_DEFAULT,
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_NUM_INPUT_BUFFERS,
			pspec);

	pspec = g_param_spec_uint ("output-buffers", "Output buffers",
			"The number of OMX output buffers",
			1, 10, NUM_OUTPUT_BUFFERS_DEFAULT,
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_NUM_OUTPUT_BUFFERS,
			pspec);

	GParamSpec* spec;
	spec = g_param_spec_enum ("process-mode", "Process mode",
			"Stream/Frame mode",
			GST_TYPE_PROCESS_MODE,
			DEFAULT_PROCESS_MODE, G_PARAM_READWRITE);

	g_object_class_install_property (g_klass, PROP_PROCESSMODE, spec);


	/* GST */
	gst_klass = GST_ELEMENT_CLASS (klass);
	gst_klass->change_state = GST_DEBUG_FUNCPTR (gst_goo_filter_change_state);

	/* GST GOO FILTER */
	GstGooFilterClass* gst_c_klass = GST_GOO_FILTER_CLASS (klass);
	gst_c_klass->timestamp_buffer_func = GST_DEBUG_FUNCPTR (gst_goo_filter_timestamp_buffer_default);
	gst_c_klass->insert_header_func = GST_DEBUG_FUNCPTR (gst_goo_filter_insert_header_default);
	gst_c_klass->last_dasf_buffer_func = GST_DEBUG_FUNCPTR (gst_goo_filter_is_last_dasf_buffer_default);
	gst_c_klass->codec_data_processing_func = GST_DEBUG_FUNCPTR (gst_goo_filter_codec_data_processing_default);
	gst_c_klass->extra_buffer_processing_func = GST_DEBUG_FUNCPTR (gst_goo_filter_extra_buffer_processing_default);
	gst_c_klass->check_fixed_src_caps_func = GST_DEBUG_FUNCPTR (gst_goo_filter_check_fixed_src_caps_default);
	gst_c_klass->set_process_mode_func = GST_DEBUG_FUNCPTR (gst_goo_filter_set_process_mode_default);

	return;
}

static void
gst_goo_filter_init (GstGooFilter* self, GstGooFilterClass* klass)
{
	GST_DEBUG ("Enter");

	GstGooFilterPrivate* priv = GST_GOO_FILTER_GET_PRIVATE (self);
	priv->num_input_buffers = NUM_INPUT_BUFFERS_DEFAULT;
	priv->num_output_buffers = NUM_OUTPUT_BUFFERS_DEFAULT;
	priv->incount = 0;
	priv->outcount = 0;
	priv->process_mode = DEFAULT_PROCESS_MODE;

	self->factory = goo_ti_component_factory_get_instance ();

	/* GST */
	GstPadTemplate* pad_template;

	pad_template = gst_element_class_get_pad_template
	(GST_ELEMENT_CLASS (klass), "sink");
	g_return_if_fail (pad_template != NULL);

	self->sinkpad = gst_pad_new_from_template (pad_template, "sink");
	gst_pad_set_event_function (self->sinkpad,
			GST_DEBUG_FUNCPTR (gst_goo_filter_sink_event));
	gst_pad_set_chain_function (self->sinkpad,
			GST_DEBUG_FUNCPTR (gst_goo_filter_chain));

	gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);

	pad_template = gst_element_class_get_pad_template
	(GST_ELEMENT_CLASS (klass), "src");
	g_return_if_fail (pad_template != NULL);

	self->srcpad = gst_pad_new_from_template (pad_template, "src");

	gst_element_add_pad (GST_ELEMENT (self), self->srcpad);

	self->adapter = gst_goo_adapter_new ();
#if 0
	self->dasfsrc_sem = gst_goo_sem_new (0);
#endif

	GST_DEBUG ("Exit");
	return;
}

gboolean
gst_goo_filter_timestamp_buffer (GstGooFilter* self, GstBuffer *gst_buffer, OMX_BUFFERHEADERTYPE* buffer)
{
	GstGooFilterClass* klass = GST_GOO_FILTER_GET_CLASS (self);

	gboolean retval = FALSE;
	if (klass->timestamp_buffer_func != NULL)
	{
		retval = (klass->timestamp_buffer_func) (self, gst_buffer, buffer);
	}

	return retval;

}

GstBuffer*
gst_goo_filter_codec_data_processing (GstGooFilter* self, GstBuffer *buffer)
{
	GstGooFilterClass* klass = GST_GOO_FILTER_GET_CLASS (self);

	GstBuffer *retbuf = NULL;
	if (klass->codec_data_processing_func != NULL)
	{
		retbuf = (klass->codec_data_processing_func) (self, buffer);
	}

	return retbuf;
}

GstBuffer*
gst_goo_filter_insert_header(GstGooFilter* self, GstBuffer *buffer, guint counter)
{
	/** FIXME GStreamer should not insert the header.  OMX component should take
 	 * care of it.  Remove this function upon resolution of DR OMAPS00140835 and
	 * OMAPS00140836 **/

	GstGooFilterClass* klass = GST_GOO_FILTER_GET_CLASS (self);

	GstBuffer *retbuf = NULL;
	if (klass->codec_data_processing_func != NULL)
	{
		retbuf = (klass->insert_header_func) (self, buffer, counter);
	}

	return retbuf;
}

static gboolean
gst_goo_filter_is_last_dasf_buffer (GstGooFilter* self, guint output_count)
{

	GstGooFilterClass* klass = GST_GOO_FILTER_GET_CLASS (self);

	gboolean retval = FALSE;
	if (klass->last_dasf_buffer_func != NULL)
	{
		retval = (klass->last_dasf_buffer_func) (self, output_count);
	}

	return retval;
}

GstBuffer*
gst_goo_filter_extra_buffer_processing (GstGooFilter* self, GstBuffer *buffer)
{
	GstGooFilterClass* klass = GST_GOO_FILTER_GET_CLASS (self);

	GstBuffer *retbuf = NULL;
	if (klass->extra_buffer_processing_func != NULL)
	{
		retbuf = (klass->extra_buffer_processing_func) (self, buffer);
	}

	return retbuf;

}

gboolean
gst_goo_filter_check_fixed_src_caps (GstGooFilter* self)
{
	GstGooFilterClass* klass = GST_GOO_FILTER_GET_CLASS (self);

	gboolean retval = FALSE;
	if (klass->check_fixed_src_caps_func != NULL)
	{
		retval = (klass->check_fixed_src_caps_func) (self);
	}

	return retval;

}

gboolean
gst_goo_filter_set_process_mode (GstGooFilter* self, guint value)
{
	GstGooFilterClass* klass = GST_GOO_FILTER_GET_CLASS (self);

	gboolean retval = FALSE;
	if (klass->set_process_mode_func != NULL)
	{
		retval = (klass->set_process_mode_func) (self, value);
	}

	return retval;

}



