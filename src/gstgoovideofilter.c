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
#include "gstgoovideofilter.h"
#include <gst/gstghostpad.h>
#include <string.h>

#define GST2OMX_TIMESTAMP(ts) (OMX_S64) ts / 1000;
#define OMX2GST_TIMESTAMP(ts) (guint64) ts * 1000;

GST_DEBUG_CATEGORY_STATIC (gst_goo_video_filter_debug);
#define GST_CAT_DEFAULT gst_goo_video_filter_debug
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

} GstGooVideoFilterProcessMode;

static GstCaps* gst_goo_video_filter_getcaps (GstPad * pad);
static GstCaps* gst_goo_video_filter_transform_caps (GstGooVideoFilter * self,
    GstPadDirection direction, GstCaps * caps);
static gboolean gst_goo_video_filter_configure_caps (GstGooVideoFilter* self, GstCaps * in,
	GstCaps * out);
static gboolean gst_goo_video_filter_setcaps (GstPad *pad, GstCaps *caps);
GstBuffer* gst_goo_video_filter_codec_data_processing (GstGooVideoFilter* self, GstBuffer *buffer);
GstBuffer* gst_goo_video_filter_extra_buffer_processing (GstGooVideoFilter* self, GstBuffer *buffer);

#define GST_TYPE_PROCESS_MODE (gst_goo_video_filter_process_mode_get_type())

static GType
gst_goo_video_filter_process_mode_get_type ()
{
	static GType goo_video_filter_process_mode_type = 0;
	static GEnumValue goo_video_filter_process_mode[] =
	{
		{0, "0", "Frame Mode"},
		{1, "1", "Stream Mode"},
		{0, NULL, NULL},
	};
	if (!goo_video_filter_process_mode_type)
	{
		goo_video_filter_process_mode_type = g_enum_register_static ("GstGooVideoFilterProcessMode", goo_video_filter_process_mode);
	}

	return goo_video_filter_process_mode_type;
}

#define GST_GOO_VIDEO_FILTER_GET_PRIVATE(obj)					\
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_GOO_VIDEO_FILTER, GstGooVideoFilterPrivate))

struct _GstGooVideoFilterPrivate
{
	guint num_input_buffers;
	guint num_output_buffers;

	guint incount;
	guint outcount;
	guint process_mode;

	GstSegment segment;
	gboolean flag_start;

};

#define NUM_INPUT_BUFFERS_DEFAULT 4
#define NUM_OUTPUT_BUFFERS_DEFAULT 4

#define LOG_CAPS(pad, caps)						\
{									\
	gchar* strcaps = gst_caps_to_string (caps);			\
	GST_INFO_OBJECT (pad, "caps = %s", strcaps ? strcaps : "NULL"); \
	g_free (strcaps);						\
}

#define DEBUG_CAPS(msg, caps)				\
{							\
	gchar* strcaps = gst_caps_to_string (caps);	\
	GST_DEBUG (msg, strcaps);			\
	g_free (strcaps);				\
}

/**
 * gst_goo_video_filter_outport_buffer:
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
gst_goo_video_filter_outport_buffer (GooPort* port, OMX_BUFFERHEADERTYPE* buffer,
				  gpointer data)
{
	g_return_if_fail (buffer->nFlags != OMX_BUFFERFLAG_DATACORRUPT);

	GST_DEBUG ("Enter");

	g_assert (GOO_IS_PORT (port));
	g_assert (buffer != NULL);
	g_assert (GOO_IS_COMPONENT (data));


	GooComponent* component = GOO_COMPONENT (data);
	GstGooVideoFilter* self =
		GST_GOO_VIDEO_FILTER (g_object_get_data (G_OBJECT (data), "gst"));
	g_assert (self != NULL);
	GstGooVideoFilterPrivate* priv = GST_GOO_VIDEO_FILTER_GET_PRIVATE (self);

	GstBuffer* gst_buffer = gst_goo_buffer_new ();
	gst_goo_buffer_set_data (gst_buffer, component, buffer);
	priv->outcount++;

	gst_goo_video_filter_timestamp_buffer (self, gst_buffer, buffer);

	GST_BUFFER_OFFSET (gst_buffer) = priv->outcount;
	gst_buffer_set_caps (gst_buffer, GST_PAD_CAPS (self->srcpad));
	gst_pad_push (self->srcpad, gst_buffer);

	if (buffer->nFlags == OMX_BUFFERFLAG_EOS || goo_port_is_eos (port))
	{
		GST_INFO ("EOS flag found in output buffer (%d)",
			  buffer->nFilledLen);
		goo_component_set_done (self->component);
	}

	return;
}

static void
gst_goo_video_filter_wait_for_done (GstGooVideoFilter* self)
{
	g_assert (self != NULL);
	GstGooVideoFilterPrivate* priv = GST_GOO_VIDEO_FILTER_GET_PRIVATE (self);

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
gst_goo_video_filter_sink_event (GstPad* pad, GstEvent* event)
{
	GST_LOG ("");

	GstGooVideoFilter* self = GST_GOO_VIDEO_FILTER (gst_pad_get_parent (pad));
	GstGooVideoFilterPrivate* priv = GST_GOO_VIDEO_FILTER_GET_PRIVATE (self);

	gboolean ret;

	g_assert (self->component != NULL);

	switch (GST_EVENT_TYPE (event))
	{
		case GST_EVENT_NEWSEGMENT:
			GST_INFO ("New segement event");
			priv->incount = 0;
			priv->outcount = 0;
			priv->flag_start = TRUE;
			ret = gst_pad_push_event (self->srcpad, event);
			break;
		case GST_EVENT_EOS:
			GST_INFO ("EOS event");
			gst_goo_video_filter_wait_for_done (self);
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
gst_goo_video_filter_setup_tunnel (GstGooVideoFilter *self)
{

	GooComponent *peer_component;

	if (!GOO_IS_TI_VIDEO_DECODER(self->component))
	{
		GST_INFO ("Tunneling only implemented for goo video decoders");
		return FALSE;
	}

	peer_component = gst_goo_utils_get_peer_component (self->srcpad);

	if (G_UNLIKELY (peer_component == NULL))
	{
		GST_INFO ("The component in next element doesn't have a GooComponent associated with it");
		return FALSE;
	}

	/** Until the change of the tunnel is done in the IDLE state
		only this combination will be possible **/
	if (!GOO_IS_TI_POST_PROCESSOR (peer_component))
	{
		GST_INFO ("The component in next element is not a GOO post processor component");
		g_object_unref (peer_component);
		return FALSE;
	}

	/** Configure the next component tunneled port since we won't
		have the caps configured by then.
		@Todo: Change this to find which port is actually tunneled **/
	
		GooPort *peer_port = goo_component_get_port (peer_component, "input0");
	{
	
		GOO_PORT_GET_DEFINITION (peer_port)->format.video.nFrameWidth =
			GOO_PORT_GET_DEFINITION (self->outport)->format.video.nFrameWidth;

		GOO_PORT_GET_DEFINITION (peer_port)->format.video.nFrameHeight =
			GOO_PORT_GET_DEFINITION (self->outport)->format.video.nFrameHeight;

		GOO_PORT_GET_DEFINITION (peer_port)->format.video.eColorFormat =
			GOO_PORT_GET_DEFINITION (self->outport)->format.video.eColorFormat;

		GOO_PORT_GET_DEFINITION (peer_port)->nBufferCountActual =
			GOO_PORT_GET_DEFINITION (self->outport)->nBufferCountActual;
	}
	
	/** @Todo: Change this to find which port is actually tunneled **/
	goo_component_set_tunnel_by_name (self->component, "output0",
						  peer_component, "input0", OMX_BufferSupplyInput);

	
	/*Sinkpp is buffer supplier*/
	goo_component_set_supplier_port (peer_component, peer_port, OMX_BufferSupplyInput);
		
	g_object_unref (peer_port);
	
	GST_INFO ("Tunneled component successfully");

	goo_component_set_state_idle (self->component);

	g_object_unref (peer_component);

	return TRUE;

}


static GstFlowReturn
gst_goo_video_filter_chain (GstPad* pad, GstBuffer* buffer)
{
	GST_LOG ("");

	GstGooVideoFilter* self = GST_GOO_VIDEO_FILTER (gst_pad_get_parent (pad));
	GstGooVideoFilterPrivate* priv = GST_GOO_VIDEO_FILTER_GET_PRIVATE (self);
	GstGooAdapter* adapter = self->adapter;
	GstFlowReturn ret = GST_FLOW_OK;

	GstClockTime timestamp = GST_BUFFER_TIMESTAMP (buffer);
	gint64 buffer_stamp = GST_BUFFER_TIMESTAMP (buffer);

	if (priv->incount == 0)
	{
		self->omx_normalize_timestamp	= GST2OMX_TIMESTAMP (timestamp);
	}
	
	if (goo_port_is_tunneled (self->inport))
	{
		/* shall we send a ghost buffer here ? */
		GST_INFO ("port is tunneled");
		ret = GST_FLOW_OK;
		if (goo_component_get_state (self->component) == OMX_StateLoaded)
		{
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
		goto fail;
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

	if (self->seek_active == TRUE)
	{
		if (buffer_stamp < self->seek_time)
		{
			GST_DEBUG_OBJECT (self, "Dropping buffer at %"GST_TIME_FORMAT, 
				GST_TIME_ARGS (buffer_stamp));
			gst_goo_adapter_clear (adapter);
			goto done;
		}
		else
		{
			GST_DEBUG_OBJECT (self, "Continue buffer at %"GST_TIME_FORMAT, 
				GST_TIME_ARGS (buffer_stamp));
			self->seek_active = FALSE;
		}
	}
	
	if (priv->incount == 0 &&
	    goo_component_get_state (self->component) == OMX_StateLoaded)
	{

		/** Some video_filters require header processing,
			apended to the first buffer **/
		buffer = gst_goo_video_filter_codec_data_processing (self, GST_BUFFER (buffer));

		g_object_set (self->inport,
				"buffercount",
				priv->num_input_buffers, NULL);
		g_object_set (self->outport,
				"buffercount",
				priv->num_output_buffers, NULL);

		/** If the tunnel was created then the component has been set to IDLE **/
		if (!gst_goo_video_filter_setup_tunnel (self))
		{
			GST_INFO ("going to idle");
			goo_component_set_state_idle (self->component);

		}

		GST_INFO ("going to executing");
		goo_component_set_state_executing (self->component);

	}

	/** Function to perform post buffering processing **/
	buffer = gst_goo_video_filter_extra_buffer_processing (self, GST_BUFFER (buffer));

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

		/*This is done for first frame or when de base time are modified*/
		if (priv->flag_start == TRUE){
			GST_DEBUG_OBJECT (self, "OMX starttime flag %d \n",omx_buffer->nFlags );
			omx_buffer->nFlags |= OMX_BUFFERFLAG_STARTTIME;
			priv->flag_start = FALSE;

		}

		/* transfer timestamp to openmax */
		{

			GST_DEBUG_OBJECT (self, "checking timestamp: time %" GST_TIME_FORMAT,
					GST_TIME_ARGS (timestamp));

			if (GST_CLOCK_TIME_IS_VALID (timestamp))
			{
				gint64 buffer_ts = (gint64)timestamp;

				omx_buffer->nTimeStamp = GST2OMX_TIMESTAMP (buffer_ts);

				GST_INFO_OBJECT (self, "OMX timestamp before normalize = %lld", omx_buffer->nTimeStamp);

				omx_buffer->nTimeStamp = omx_buffer->nTimeStamp - self->omx_normalize_timestamp;

				GST_INFO_OBJECT (self, "OMX timestamp = %lld", omx_buffer->nTimeStamp);

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
	gst_buffer_unref (buffer);
	gst_goo_adapter_clear (adapter);
done:
	gst_object_unref (self);
	return ret;
}

GST_BOILERPLATE (GstGooVideoFilter, gst_goo_video_filter, GstElement, GST_TYPE_ELEMENT);

static GstStateChangeReturn
gst_goo_video_filter_change_state (GstElement* element, GstStateChange transition)
{
	GST_LOG ("");

	GstGooVideoFilter* self = GST_GOO_VIDEO_FILTER (element);
	GstStateChangeReturn result;
	GstGooVideoFilterPrivate* priv = GST_GOO_VIDEO_FILTER_GET_PRIVATE (self);

	g_assert (self->component != NULL);
	g_assert (self->inport != NULL);
	g_assert (self->outport != NULL);

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
gst_goo_video_filter_set_property (GObject* object, guint prop_id,
				const GValue* value, GParamSpec* pspec)
{
	GstGooVideoFilterPrivate* priv = GST_GOO_VIDEO_FILTER_GET_PRIVATE (object);
	GstGooVideoFilter* self = GST_GOO_VIDEO_FILTER (object);

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
		gst_goo_video_filter_set_process_mode (self, priv->process_mode);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_video_filter_get_property (GObject* object, guint prop_id,
				GValue* value, GParamSpec* pspec)
{
	GstGooVideoFilterPrivate* priv = GST_GOO_VIDEO_FILTER_GET_PRIVATE (object);

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
gst_goo_video_filter_dispose (GObject* object)
{
        GstGooVideoFilter* me;

        G_OBJECT_CLASS (parent_class)->dispose (object);

        me = GST_GOO_VIDEO_FILTER (object);

		if (G_LIKELY (me->adapter))
		{
			GST_DEBUG ("unrefing adapter");
			g_object_unref (me->adapter);
		}

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
		GST_DEBUG ("Component refcount = %d",
			  G_OBJECT (me->component)->ref_count);

		GST_DEBUG ("unrefing component");
		G_OBJECT(me->component)->ref_count = 1;
                g_object_unref (me->component);
        }

	if (G_LIKELY (me->factory))
	{
		GST_DEBUG ("Factory refcount = %d",
			  G_OBJECT (me->factory)->ref_count);


		GST_DEBUG ("unrefing factory");
		g_object_unref (me->factory);
	}

        return;
}


static gboolean
gst_goo_video_filter_timestamp_buffer_default (GstGooVideoFilter* self, GstBuffer *gst_buffer, OMX_BUFFERHEADERTYPE* buffer)
{
	GstGooVideoFilterPrivate* priv = GST_GOO_VIDEO_FILTER_GET_PRIVATE (self);

	gint64 buffer_ts = (gint64)buffer->nTimeStamp;
	guint64 timestamp = OMX2GST_TIMESTAMP (buffer_ts);

	GST_DEBUG_OBJECT (self, "");

	/* We need to remove the OMX timestamp normalization */
	timestamp += OMX2GST_TIMESTAMP(self->omx_normalize_timestamp);

	if (GST_CLOCK_TIME_IS_VALID (timestamp) && timestamp != 0)
	{
		GST_INFO_OBJECT (self, "Already had a timestamp: %" GST_TIME_FORMAT,
		GST_TIME_ARGS (timestamp));
		GST_BUFFER_TIMESTAMP (gst_buffer) = timestamp;
		GST_BUFFER_DURATION (gst_buffer) = gst_util_uint64_scale_int (
			GST_SECOND, self->rate_denominator, self->rate_numerator);
	}
	else
	{
		/*Estimating timestamps */
		GstClockTime next_time;

		if (priv->outcount == 1 && priv->process_mode == STREAMMODE)
		{
			GstEvent *event = gst_event_new_new_segment (FALSE, 1.0, GST_FORMAT_TIME,
				0, GST_CLOCK_TIME_NONE, 0);
			GST_DEBUG_OBJECT (self, "Pushing newsegment");
			gst_pad_push_event (self->srcpad, event);
		}

		next_time = gst_util_uint64_scale_int (priv->outcount * GST_SECOND,
				self->rate_denominator, self->rate_numerator);
		GST_BUFFER_TIMESTAMP (gst_buffer) = self->running_time;
		GST_DEBUG_OBJECT (self, "Estimating timestamp: time %" GST_TIME_FORMAT,
					GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (gst_buffer)));

		GST_BUFFER_DURATION (gst_buffer) = gst_util_uint64_scale_int (
			GST_SECOND, self->rate_denominator, self->rate_numerator);

		self->running_time = next_time;
	}

	GST_DEBUG_OBJECT (self, "");

	return TRUE;

}

static gboolean
gst_goo_video_filter_set_process_mode_default (GstGooVideoFilter* self, guint value)
{
	return TRUE;
}

static GstBuffer*
gst_goo_video_filter_codec_data_processing_default (GstGooVideoFilter* self, GstBuffer *buffer)
{

	GST_DEBUG ("No need to process codec data");
	return GST_BUFFER (buffer);

}

static GstBuffer*
gst_goo_video_filter_extra_buffer_processing_default (GstGooVideoFilter* self, GstBuffer *buffer)
{
	return buffer;
}


static void
gst_goo_video_filter_base_init (gpointer g_klass)
{
        GST_DEBUG_CATEGORY_INIT (gst_goo_video_filter_debug, "goovideofilter", 0,
                                 "Gst OpenMax parent class for video filters");

	return;
}

static void
gst_goo_video_filter_class_init (GstGooVideoFilterClass* klass)
{
	GObjectClass* g_klass;
	GParamSpec* pspec;
	GstElementClass* gst_klass;

	/* gobject */
	g_klass = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GstGooVideoFilterPrivate));

	g_klass->set_property =
	GST_DEBUG_FUNCPTR (gst_goo_video_filter_set_property);
	g_klass->get_property =
	GST_DEBUG_FUNCPTR (gst_goo_video_filter_get_property);
	g_klass->dispose = GST_DEBUG_FUNCPTR (gst_goo_video_filter_dispose);

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
	gst_klass->change_state = GST_DEBUG_FUNCPTR (gst_goo_video_filter_change_state);

	/* GST GOO VIDEO_FILTER */
	GstGooVideoFilterClass* gst_c_klass = GST_GOO_VIDEO_FILTER_CLASS (klass);
	gst_c_klass->timestamp_buffer_func = GST_DEBUG_FUNCPTR (gst_goo_video_filter_timestamp_buffer_default);
	gst_c_klass->codec_data_processing_func = GST_DEBUG_FUNCPTR (gst_goo_video_filter_codec_data_processing_default);
	gst_c_klass->extra_buffer_processing_func = GST_DEBUG_FUNCPTR (gst_goo_video_filter_extra_buffer_processing_default);
	gst_c_klass->set_process_mode_func = GST_DEBUG_FUNCPTR (gst_goo_video_filter_set_process_mode_default);

	return;
}

static void
gst_goo_video_filter_init (GstGooVideoFilter* self, GstGooVideoFilterClass* klass)
{
	GST_DEBUG ("Enter");

	GstGooVideoFilterPrivate* priv = GST_GOO_VIDEO_FILTER_GET_PRIVATE (self);
	priv->num_input_buffers = NUM_INPUT_BUFFERS_DEFAULT;
	priv->num_output_buffers = NUM_OUTPUT_BUFFERS_DEFAULT;
	priv->incount = 0;
	priv->outcount = 0;
	priv->flag_start = TRUE;
	priv->process_mode = DEFAULT_PROCESS_MODE;

	self->factory = goo_ti_component_factory_get_instance ();
	self->omx_normalize_timestamp = 0;
	self->rate_numerator = 15;
	self->rate_denominator = 1;
	self->seek_active = FALSE;

	/* GST */
	GstPadTemplate* pad_template;

	pad_template = gst_element_class_get_pad_template
	(GST_ELEMENT_CLASS (klass), "sink");
	g_return_if_fail (pad_template != NULL);

	self->sinkpad = gst_pad_new_from_template (pad_template, "sink");
	gst_pad_set_event_function (self->sinkpad,
		GST_DEBUG_FUNCPTR (gst_goo_video_filter_sink_event));
	gst_pad_set_chain_function (self->sinkpad,
		GST_DEBUG_FUNCPTR (gst_goo_video_filter_chain));
	gst_pad_set_setcaps_function (self->sinkpad,
		GST_DEBUG_FUNCPTR (gst_goo_video_filter_setcaps));

	gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);

	pad_template = gst_element_class_get_pad_template
		(GST_ELEMENT_CLASS (klass), "src");
	g_return_if_fail (pad_template != NULL);

	self->srcpad = gst_pad_new_from_template (pad_template, "src");
	gst_pad_set_setcaps_function (self->srcpad,
		GST_DEBUG_FUNCPTR (gst_goo_video_filter_setcaps));

	gst_element_add_pad (GST_ELEMENT (self), self->srcpad);

	self->adapter = gst_goo_adapter_new ();

	GST_DEBUG ("Exit");
	return;
}

gboolean
gst_goo_video_filter_timestamp_buffer (GstGooVideoFilter* self, GstBuffer *gst_buffer, OMX_BUFFERHEADERTYPE* buffer)
{
	GstGooVideoFilterClass* klass = GST_GOO_VIDEO_FILTER_GET_CLASS (self);

	gboolean retval = FALSE;
	if (klass->timestamp_buffer_func != NULL)
	{
		retval = (klass->timestamp_buffer_func) (self, gst_buffer, buffer);
	}

	return retval;

}

GstBuffer*
gst_goo_video_filter_codec_data_processing (GstGooVideoFilter* self, GstBuffer *buffer)
{
	GstGooVideoFilterClass* klass = GST_GOO_VIDEO_FILTER_GET_CLASS (self);

	GstBuffer *retbuf;
	if (klass->codec_data_processing_func != NULL)
	{
		retbuf = (klass->codec_data_processing_func) (self, buffer);
	}

	return retbuf;
}

GstBuffer*
gst_goo_video_filter_extra_buffer_processing (GstGooVideoFilter* self, GstBuffer *buffer)
{
	GstGooVideoFilterClass* klass = GST_GOO_VIDEO_FILTER_GET_CLASS (self);

	GstBuffer *retbuf;
	if (klass->extra_buffer_processing_func != NULL)
	{
		retbuf = (klass->extra_buffer_processing_func) (self, buffer);
	}

	return retbuf;

}

gboolean
gst_goo_video_filter_set_process_mode (GstGooVideoFilter* self, guint value)
{
	GstGooVideoFilterClass* klass = GST_GOO_VIDEO_FILTER_GET_CLASS (self);

	gboolean retval = FALSE;
	if (klass->set_process_mode_func != NULL)
	{
		retval = (klass->set_process_mode_func) (self, value);
	}

	return retval;

}

/* this is a blatant copy of GstBaseTransform transform caps function */
static GstCaps *
gst_goo_video_filter_transform_caps (GstGooVideoFilter * self,
    GstPadDirection direction, GstCaps * caps)
{
	GstCaps *ret;
	GstGooVideoFilterClass *klass;

	klass = GST_GOO_VIDEO_FILTER_GET_CLASS (self);

	/* if there is a custom transform function, use this */
	if (klass->transform_caps)
	{
		GstCaps *temp;
		gint i;

		/* start with empty caps */
		ret = gst_caps_new_empty ();
		GST_DEBUG_OBJECT (self, "transform caps (direction = %d)", direction);

		if (gst_caps_is_any (caps)) {
			/* for any caps we still have to call the transform function */
			GST_DEBUG_OBJECT (self, "from: ANY");
			temp = klass->transform_caps (self, direction, caps);
			GST_DEBUG_OBJECT (self, "  to: %" GST_PTR_FORMAT, temp);

			temp = gst_caps_make_writable (temp);
			gst_caps_append (ret, temp);
		}
		else
		{
			/* we send caps with just one structure to the transform
			     function as this is easier for the element */
			for (i = 0; i < gst_caps_get_size (caps); i++)
			{
       		 	GstCaps *nth;
				nth = gst_caps_copy_nth (caps, i);
				GST_DEBUG_OBJECT (self, "from[%d]: %" GST_PTR_FORMAT, i, nth);
				temp = klass->transform_caps (self, direction, nth);
				gst_caps_unref (nth);
				GST_DEBUG_OBJECT (self, "  to[%d]: %" GST_PTR_FORMAT, i, temp);

				temp = gst_caps_make_writable (temp);
				/* FIXME: here we need to only append those structures, that are not yet
				* in there
				* gst_caps_append (ret, temp);
				*/
				gst_caps_merge (ret, temp);
			}

			GST_DEBUG_OBJECT (self, "merged: (%d)", gst_caps_get_size (ret));
			/* now simplify caps
			gst_caps_do_simplify (ret);
			GST_DEBUG_OBJECT (self, "simplified: (%d)", gst_caps_get_size (ret));
			*/
		}
	}
	else
	{
		/* else use the identity transform */
		ret = gst_caps_ref (caps);
	}

	GST_DEBUG_OBJECT (self, "to: (%d) %" GST_PTR_FORMAT,
		gst_caps_get_size (ret), ret);

	return ret;
}

/* this is a blatant copy of GstBaseTransform setcasp function */
static gboolean
gst_goo_video_filter_setcaps (GstPad* pad, GstCaps* caps)
{
	GstGooVideoFilter* self;
	GstGooVideoFilterClass *klass;
	GstGooVideoFilterPrivate* priv;
	GstPad* otherpad;
	GstPad* otherpeer;
	GstCaps* othercaps = NULL;
	gboolean ret = TRUE;
	gboolean peer_checked = FALSE;

	self = GST_GOO_VIDEO_FILTER (gst_pad_get_parent (pad));
	klass = GST_GOO_VIDEO_FILTER_GET_CLASS (self);

	GST_DEBUG_OBJECT (self, "");

	g_return_val_if_fail (GST_CAPS_IS_SIMPLE (caps), FALSE);
	g_return_val_if_fail (priv != NULL, FALSE);

	otherpad = (pad == self->srcpad) ? self->sinkpad : self->srcpad;
	otherpeer = gst_pad_get_peer (otherpad);

	/* if we get called recursively, we bail out now to avoid an
	 * infinite loop. */
	if (GST_PAD_IS_IN_SETCAPS (otherpad))
	{
		goto done;
	}

	/* caps must be fixed here */
	if (!gst_caps_is_fixed (caps))
	{
		goto unfixed_caps;
	}

	othercaps = gst_goo_video_filter_transform_caps
		(self, GST_PAD_DIRECTION (pad), caps);

	if (othercaps)
	{
		GstCaps* intersect;
		const GstCaps *templ_caps;

		templ_caps = gst_pad_get_pad_template_caps (otherpad);
		intersect = gst_caps_intersect (othercaps, templ_caps);

		gst_caps_unref (othercaps);
		othercaps = intersect;
	}

	/* check if transform is empty */
	if (!othercaps || gst_caps_is_empty (othercaps))
	{
		goto no_transform;
	}

	/* if the othercaps are not fixed, we need to fixate them,
	 * first attempt is by attempting passthrough if the othercaps
	 * are a superset of caps. */
	if (!gst_caps_is_fixed (othercaps))
	{
		GstCaps *temp;

		DEBUG_CAPS ("transform returned non fixed %s", othercaps);
		/* see if the target caps are a superset of the source caps,
		 * in this case we can try to perform passthrough */
		temp = gst_caps_intersect (othercaps, caps);
		DEBUG_CAPS ("intersect returned %s", temp);

		if (temp)
		{
			if (!gst_caps_is_empty (temp) && otherpeer)
			{
				DEBUG_CAPS ("try passthrough with %s", caps);
				/* try passthrough. we know it's fixed,
				 * because caps is fixed */
				if (gst_pad_accept_caps (otherpeer, caps))
				{
					DEBUG_CAPS ("peer accepted %s", caps);
					/* peer accepted unmodified caps, we
					 * free the original non-fixed
					 * caps and work with the passthrough
					 * caps */
					gst_caps_unref (othercaps);
					othercaps = gst_caps_ref (caps);
					/* mark that we checked othercaps with
					 * the peer, this makes sure we don't
					 * call accept_caps again with these
					 * same caps */
					peer_checked = TRUE;
				}
				else
				{
					DEBUG_CAPS ("peer did not accept %s",
						    caps);
				}
			}
			gst_caps_unref (temp);
		}
	}

	/* second attempt at fixation is done by intersecting with
	 * the peer caps */
	if (!gst_caps_is_fixed (othercaps) && otherpeer)
	{
		/* intersect against what the peer can do */
		GstCaps *peercaps;
		GstCaps *intersect;

		DEBUG_CAPS ("othercaps now %s", othercaps);

		peercaps = gst_pad_get_caps (otherpeer);
		intersect = gst_caps_intersect (peercaps, othercaps);
		gst_caps_unref (peercaps);
		gst_caps_unref (othercaps);
		othercaps = intersect;
		peer_checked = FALSE;

		DEBUG_CAPS ("filtering against peer yields %s", othercaps);
	}

	if (gst_caps_is_empty (othercaps))
	{
		goto no_transform_possible;
	}

	/* third attempt at fixation, call the fixate vmethod and
	 * ultimately call the pad fixate function. */
	if (!gst_caps_is_fixed (othercaps))
	{
		GstCaps *temp;

		LOG_CAPS (otherpad, othercaps);

		/* since we have no other way to fixate left, we might as well
		 * just take the first of the caps list and fixate that */

		/* FIXME: when fixating using the vmethod, it might make sense
		 * to fixate each of the caps; but Wim doesn't see a use case
		 * for that yet */
		temp = gst_caps_copy_nth (othercaps, 0);
		gst_caps_unref (othercaps);
		othercaps = temp;
		peer_checked = FALSE;

		DEBUG_CAPS ("trying to fixate %s", othercaps);
		if (klass->fixate_caps)
		{
			GST_DEBUG_OBJECT (self, "trying to fixate %" GST_PTR_FORMAT
				" using caps %" GST_PTR_FORMAT
				" on pad %s:%s using fixate_caps vmethod", othercaps, caps,
			GST_DEBUG_PAD_NAME (otherpad));
			klass->fixate_caps (self, GST_PAD_DIRECTION (pad), caps, othercaps);
		}

	}

	/* if still not fixed, no other option but to let the default pad
	 * fixate function do its job */
	if (!gst_caps_is_fixed (othercaps))
	{
		LOG_CAPS (otherpad, othercaps);
		gst_pad_fixate_caps (otherpad, othercaps);
	}
	DEBUG_CAPS ("after fixating %s", othercaps);

	/* caps should be fixed now, if not we have to fail. */
	if (!gst_caps_is_fixed (othercaps))
	{
		goto could_not_fixate;
	}

	/* and peer should accept, don't check again if we already checked the
	 * othercaps against the peer. */
	if (!peer_checked && otherpeer &&
	    !gst_pad_accept_caps (otherpeer, othercaps))
	{
		goto peer_no_accept;
	}

	DEBUG_CAPS ("Input caps were %s", caps);
	DEBUG_CAPS ("and got final caps %s", othercaps);

	GstCaps* incaps;
	GstCaps* outcaps;

	if (pad == self->sinkpad)
	{
		incaps = caps;
		outcaps = othercaps;
	}
	else
	{
		incaps = othercaps;
		outcaps = caps;
	}

	if (!(ret = gst_goo_video_filter_configure_caps (self, incaps, outcaps)))
	{
		goto failed_configure;
	}

	/* we know this will work, we implement the setcaps */
	gst_pad_set_caps (otherpad, othercaps);

done:
	if (otherpeer)
	{
		gst_object_unref (otherpeer);
	}
	if (othercaps)
	{
		gst_caps_unref (othercaps);
	}

	gst_object_unref (self);

	return ret;

/* ERRORS */
unfixed_caps:
	{
		DEBUG_CAPS ("caps are not fixed %s", caps);
		ret = FALSE;
		goto done;
	}
no_transform:
	{
		DEBUG_CAPS ("transform returned useless %s", othercaps);
		ret = FALSE;
		goto done;
	}
no_transform_possible:
	{
		DEBUG_CAPS ("transform could not transform in anything we support %s", caps);
		ret = FALSE;
		goto done;
	}
could_not_fixate:
	{
		DEBUG_CAPS ("FAILED to fixate %s", othercaps);
		ret = FALSE;
		goto done;
	}
peer_no_accept:
	{
		LOG_CAPS (otherpad, othercaps);
		ret = FALSE;
		goto done;
	}
failed_configure:
	{
		LOG_CAPS ("FAILED to configure caps %s", othercaps);
		ret = FALSE;
		goto done;
	}
}

/* this is a blatant copy of GstBaseTransform configure caps function */
static gboolean
gst_goo_video_filter_configure_caps (GstGooVideoFilter* self, GstCaps * in,
	GstCaps * out)
{
	gboolean ret = TRUE;
	GstGooVideoFilterClass *klass;

	klass = GST_GOO_VIDEO_FILTER_GET_CLASS (self);

	/* Configure the element with the caps */
	if (klass->set_caps)
	{
		GST_DEBUG_OBJECT (self, "Calling configure caps on the child");
		ret = klass->set_caps (self, in, out);
	}

	return ret;
}

