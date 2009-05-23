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
#include "gstgooutils.h"
#include "gstghostbuffer.h"
#include "goo-semaphore.h"
#include <gst/gstghostpad.h>
#include <string.h>


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
static gboolean gst_goo_video_filter_codec_data_extra_buffer (GstGooVideoFilter* self, GstBuffer *buffer);

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

typedef struct _GstGooVideoFilterPrivate GstGooVideoFilterPrivate;
struct _GstGooVideoFilterPrivate
{
	guint num_input_buffers;
	guint num_output_buffers;

	guint incount;
	guint outcount;
	guint process_mode;

	GooSemaphore* input_sem;

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

	GstBuffer* gst_buffer = gst_buffer_new_and_alloc (buffer->nFilledLen);
	gst_buffer_set_caps (gst_buffer, GST_PAD_CAPS (self->srcpad));

	/* evil memcpy().. we need to do this because we need to release the
	 * buffer back to the OMX component ASAP, to avoid blocking the OMX
	 * component from blocking the next frame of data... there could be
	 * a gstqueue downstream of us (for example, in an AV record scenario)
	 * so it could be quite some time before the buffer is free again..
	 */
	memcpy (GST_BUFFER_DATA (gst_buffer), buffer->pBuffer + buffer->nOffset, buffer->nFilledLen);

	priv->outcount++;

	gst_goo_video_filter_timestamp_buffer (self, gst_buffer, buffer);
	GST_BUFFER_OFFSET (gst_buffer) = priv->outcount;

	gboolean inport_tunneled = goo_port_is_tunneled (self->inport);

	if (inport_tunneled)
	{
		g_assert (priv->input_sem);
		goo_semaphore_up (priv->input_sem);
	}

	if (inport_tunneled && (priv->outcount > priv->incount) )
	{
		GST_INFO ( "sending buffer with EOS flag");
		buffer->nFlags |= OMX_BUFFERFLAG_EOS;

		if ((buffer->nFlags & OMX_BUFFERFLAG_EOS) || goo_port_is_eos (port))
		{
			GST_INFO ("EOS flag in output buffer (%d)",
		  		buffer->nFilledLen);
			goo_component_set_done (self->component);
			GstEvent*   event = gst_event_new_eos();
			gst_pad_push_event (self->srcpad, event);
			gst_buffer_unref (gst_buffer);
		}

		goo_component_release_buffer (component, buffer);
	}
	else
	{
		gboolean is_done = FALSE;

		if ((buffer->nFlags & OMX_BUFFERFLAG_EOS) || goo_port_is_eos (port))
		{
			GST_INFO ("EOS flag found in output buffer (%d)",
			  	buffer->nFilledLen);
			is_done = TRUE;
		}

		goo_component_release_buffer (component, buffer);
		gst_pad_push (self->srcpad, gst_buffer);

		if(is_done)
		{
			goo_component_set_done (self->component);
		}
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
	GST_INFO ("%s", GST_EVENT_TYPE_NAME (event));

	GstGooVideoFilter* self = GST_GOO_VIDEO_FILTER (gst_pad_get_parent (pad));
	GstGooVideoFilterPrivate* priv = GST_GOO_VIDEO_FILTER_GET_PRIVATE (self);

	gboolean ret;

	g_assert (self->component != NULL);

	switch (GST_EVENT_TYPE (event))
	{
		case GST_EVENT_NEWSEGMENT:
			priv->incount = 0;
			priv->outcount = 0;
			priv->flag_start = TRUE;
			ret = gst_pad_push_event (self->srcpad, event);
			break;
		case GST_EVENT_EOS:
			/* if the outport is tunneled, that means we are pushing data
			 * to the decoder from the context of the sink (see _chain()/
			 * _chain2()).  Which also means that we need to wait until
			 * the sink receives the EOS event to inform OMX until we
			 * get the reverse EOS event back from the sink
			 */
			if (!goo_port_is_tunneled (self->outport))
			{
				gst_goo_video_filter_wait_for_done (self);
			}

			/* if inport is tunneled, then the EOS event gets pushed downstream
			 * only after we get OMX_BUFFERFLAG_EOS back from the OMX layer
			 *
			 * TODO: what if both inport and outport are tunneled?  Maybe
			 * we should move this logic to gst_goo_video_filter_src_event()
			 * when we receive the reverse-eos upstream event from the sink?
			 */
			if (!goo_port_is_tunneled (self->inport))
			{
				ret = gst_pad_push_event (self->srcpad, event);
			}
			else
			{
				/* need to make sure camera knows we are in EOS!  another
				 * big hack for OMX tunnels.  The issue is that gstgoocamera
				 * never finds out when it's src pad loop function finishes,
				 * and since the actual thread pushing the buffers forward
				 * down the pipe is in OMX camera, it needs to move OMX
				 * camera out of executing state.. which it can't do unless
				 * we notify it.
				 */
				gst_element_send_event (GST_ELEMENT (self),
						gst_goo_event_new_reverse_eos() );

				ret = TRUE;
			}
			break;
		case GST_EVENT_FLUSH_START:
			GST_INFO ("Flush Start Event");
			goo_component_set_state_pause(self->component);
			goo_component_flush_all_ports(self->component);
			ret = gst_pad_push_event (self->srcpad, event);
			break;
		case GST_EVENT_FLUSH_STOP:
			GST_INFO ("Flush Stop Event");
			goo_component_set_state_executing(self->component);
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
gst_goo_video_filter_src_event (GstPad *pad, GstEvent *event)
{
	GST_INFO ("%s", GST_EVENT_TYPE_NAME (event));

	GstGooVideoFilter* self = GST_GOO_VIDEO_FILTER (gst_pad_get_parent (pad));
	GstGooVideoFilterPrivate* priv = GST_GOO_VIDEO_FILTER_GET_PRIVATE (self);

	gboolean ret;

	g_assert (self->component != NULL);

	switch (GST_EVENT_TYPE (event))
	{
		case GST_EVENT_CUSTOM_UPSTREAM:
			if (gst_goo_event_is_reverse_eos (event) && goo_port_is_tunneled (self->outport))
			{
				gst_goo_video_filter_wait_for_done (self);
			}
			ret = gst_pad_push_event (self->sinkpad, event);
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

	GST_INFO ("");

	if (!GOO_IS_TI_VIDEO_DECODER(self->component))
	{
		GST_INFO ("Tunneling only implemented for goo video decoders");
		return FALSE;
	}

	peer_component = gst_goo_util_find_goo_component (GST_ELEMENT (self), GOO_TYPE_TI_POST_PROCESSOR);

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

		GOO_PORT_GET_DEFINITION (peer_port)->format.video.nFrameWidth =
			GOO_PORT_GET_DEFINITION (self->outport)->format.video.nFrameWidth;

		GOO_PORT_GET_DEFINITION (peer_port)->format.video.nFrameHeight =
			GOO_PORT_GET_DEFINITION (self->outport)->format.video.nFrameHeight;

		GOO_PORT_GET_DEFINITION (peer_port)->format.video.eColorFormat =
			GOO_PORT_GET_DEFINITION (self->outport)->format.video.eColorFormat;
		GOO_PORT_GET_DEFINITION (peer_port)->nBufferCountActual =
			GOO_PORT_GET_DEFINITION (self->outport)->nBufferCountActual;

	/** @Todo: Change this to find which port is actually tunneled **/
	goo_component_set_tunnel_by_name (self->component, "output0",
						  peer_component, "input0", OMX_BufferSupplyInput);


	/*Sinkpp is buffer supplier*/
	goo_component_set_supplier_port (peer_component, peer_port, OMX_BufferSupplyInput);

	g_object_unref (peer_port);

	GST_INFO ("Tunneled component successfully");

	g_object_unref (peer_component);

	return TRUE;

}

static GstFlowReturn
gst_goo_video_filter_chain2 (GstPad* pad, GstBuffer* buffer)
{
	GST_LOG ("");

	GstGooVideoFilter* self = GST_GOO_VIDEO_FILTER (gst_pad_get_parent (pad));
	GstGooVideoFilterPrivate* priv = GST_GOO_VIDEO_FILTER_GET_PRIVATE (self);
	GstGooAdapter* adapter = self->adapter;
	GstFlowReturn ret = GST_FLOW_OK;
	GstClockTime timestamp = GST_BUFFER_TIMESTAMP (buffer);

GST_DEBUG ("buffer=0x%08x (%"GST_TIME_FORMAT", %08x)", buffer, GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buffer)), GST_BUFFER_FLAGS (buffer));

	if (priv->incount == 0)
	{
		gst_goo_util_ensure_executing (self->component);
	}

	if (self->component->cur_state != OMX_StateExecuting)
	{
		ret = GST_FLOW_OK;
		goto done;
	}

	if (goo_port_is_eos (self->inport))
	{
		GST_INFO ("port is eos");
		ret = GST_FLOW_UNEXPECTED;
		goto done;
	}

	/*This is done for first frame or when de base time are modified*/
	if (GST_GOO_UTIL_IS_DISCONT (buffer))
	{
		gst_goo_adapter_clear (adapter);
	}

	gst_goo_adapter_push (adapter, buffer);

	int omxbufsiz;

	if (priv->process_mode == STREAMMODE)
		omxbufsiz = GOO_PORT_GET_DEFINITION (self->inport)->nBufferSize;
	else
		omxbufsiz = GST_BUFFER_SIZE (buffer);


	static gboolean if_first=TRUE;
	if (if_first)
	{
		if ( gst_goo_video_filter_codec_data_extra_buffer (self, buffer))
		GST_INFO (" ******  Codec_data_extra_buffer DONE !!!  ******");
		if_first=FALSE;
	}

	while (gst_goo_adapter_available (adapter) >= omxbufsiz &&
	       ret == GST_FLOW_OK && omxbufsiz != 0)
	{
		GST_DEBUG ("Adapter available=%d  omxbufsiz=%d", gst_goo_adapter_available (adapter), omxbufsiz);

		OMX_BUFFERHEADERTYPE* omx_buffer;
		omx_buffer = goo_port_grab_buffer (self->inport);
		GST_DEBUG ("memcpy to buffer %d bytes", omxbufsiz);
		gst_goo_adapter_peek (adapter, omxbufsiz, omx_buffer);
		omx_buffer->nFilledLen = omxbufsiz;
		gst_goo_adapter_flush (adapter, omxbufsiz);

		GST_DEBUG_OBJECT (self, "checking timestamp: time %" GST_TIME_FORMAT,
				GST_TIME_ARGS (timestamp));

		gst_goo_timestamp_gst2omx (omx_buffer, buffer, TRUE);

		priv->incount++;
		goo_component_release_buffer (self->component, omx_buffer);
GST_DEBUG_OBJECT (self, "released buffer..");

		ret = GST_FLOW_OK;
	}

done:
	gst_object_unref (self);

	return ret;
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


	if (goo_port_is_tunneled (self->inport))
	{
		GST_INFO ("port is tunneled");

		priv->incount++;
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

		/* we need to wait for gst_goo_video_filter_outport_buffer(): */
		g_assert (priv->input_sem);
		goo_semaphore_down (priv->input_sem, FALSE);

		goto fail;
	}

	// TODO maybe all access to 'adapter' needs to be moved to chain2()?  could be weird race
	// conditions between chain() and chain2() otherwise..

	/* note: use priv->flag_start here, rather than priv->incount==0, since
	 * the _chain() function can be called multiple times before _chain2()
	 * (where incount is incremented).. so here, priv->incount==0 is not
	 * a reliable way to determine if this is the first buffer
	 */
	if (priv->flag_start &&
	    goo_component_get_state (self->component) == OMX_StateLoaded)
	{
		GST_DEBUG_OBJECT (self, "potential header processing");

		/** Some video_filters require header processing,
			apended to the first buffer **/
		buffer = gst_goo_video_filter_codec_data_processing (self, GST_BUFFER (buffer));

		g_object_set (self->inport,
				"buffercount",
				priv->num_input_buffers, NULL);
		g_object_set (self->outport,
				"buffercount",
				priv->num_output_buffers, NULL);

		gst_goo_video_filter_setup_tunnel (self);

		/* note: don't go to idle here.. we need to do that in chain2() because
		 * it must happen after caps negotiation, as the state changes need to
		 * be coordinated between the decoder and sink elements
		 */

		priv->flag_start = FALSE;
	}

	/** Function to perform post buffering processing **/
	buffer = gst_goo_video_filter_extra_buffer_processing (self, GST_BUFFER (buffer));

	if (goo_port_is_tunneled (self->outport))
	{
		GstGhostBuffer *ghost_buffer = gst_ghost_buffer_new ();
		GST_BUFFER_TIMESTAMP (ghost_buffer) = GST_BUFFER_TIMESTAMP (buffer);
		GST_BUFFER_DURATION (ghost_buffer)  = GST_BUFFER_DURATION (buffer);
		GST_BUFFER_SIZE (ghost_buffer)      = 0;

		ghost_buffer->chain  = gst_goo_video_filter_chain2;
		ghost_buffer->pad    = gst_object_ref(pad);
		//GST_DEBUG ("buffer=0x%08x (%"GST_TIME_FORMAT", %08x)", buffer, GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buffer)), GST_BUFFER_FLAGS (buffer));
		ghost_buffer->buffer = gst_buffer_ref(buffer);

		if (self->src_caps)
		{
			GST_DEBUG ("setting caps on ghost buffer");
			GST_BUFFER_CAPS (ghost_buffer) = self->src_caps;
			/* we only need to set the caps when they change.. after
			 * that, we can leave them as null:
			 */
			self->src_caps = NULL;  // ?? do I need to unref after the gst_pad_push()?
		}
		gst_pad_push (self->srcpad, GST_BUFFER (ghost_buffer));
	}
	else
	{
		ret = gst_goo_video_filter_chain2 (pad, buffer);
	}

	goto done;

fail:
	GST_DEBUG ("fail");
	gst_goo_adapter_clear (adapter);
done:
	gst_buffer_unref (buffer);
	gst_object_unref (self);
	return ret;
}

GST_BOILERPLATE (GstGooVideoFilter, gst_goo_video_filter, GstElement, GST_TYPE_ELEMENT);

static GstStateChangeReturn
gst_goo_video_filter_change_state (GstElement* element, GstStateChange transition)
{
	GST_LOG ("transition=%d", transition);

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
	case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
		/* if we have tunneled input, we need to use the input_sem... so
		 * ensure that it is allocated:
		 */
		if (goo_port_is_tunneled (self->inport) && !priv->input_sem)
		{
			priv->input_sem = goo_semaphore_new (0);
		}
		break;
	case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
		/* goo_component_set_state_paused (self->component); */
		break;
	case GST_STATE_CHANGE_PAUSED_TO_READY:
		if (!(goo_port_is_tunneled (self->inport)))
		{
		GST_INFO ("going to idle");
		goo_component_set_state_idle (self->component);
		GST_INFO ("going to loaded");
		goo_component_set_state_loaded (self->component);
		}
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

	GST_DEBUG_OBJECT (self, "");

	if (gst_goo_timestamp_omx2gst (gst_buffer, buffer))
	{
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


static gboolean
gst_goo_video_filter_codec_data_extra_buffer_default (GstGooVideoFilter* self, GstBuffer *buffer)
{

	GST_DEBUG ("No need to extra codec data buffer");
	return TRUE;

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
	gst_c_klass->codec_data_extra_buffer_func = GST_DEBUG_FUNCPTR (gst_goo_video_filter_codec_data_extra_buffer_default);

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
	self->rate_numerator = 30;
	self->rate_denominator = 1;

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
	gst_pad_set_event_function (self->srcpad,
		GST_DEBUG_FUNCPTR (gst_goo_video_filter_src_event));
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

static gboolean
gst_goo_video_filter_codec_data_extra_buffer (GstGooVideoFilter* self, GstBuffer *buffer)
{
	GstGooVideoFilterClass* klass = GST_GOO_VIDEO_FILTER_GET_CLASS (self);

	gboolean retval = FALSE;
	if (klass->codec_data_extra_buffer_func != NULL)
	{
		retval = (klass->codec_data_extra_buffer_func) (self, buffer);
	}

	return retval;

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

	/** To get the value of framerate parameter and determine the timestamp correctly **/
	GstStructure *structure;
	structure = gst_caps_get_structure (caps, 0);
	const GValue *framerate;
	framerate = gst_structure_get_value (structure, "framerate");
	self->rate_numerator = gst_value_get_fraction_numerator (framerate);
	self->rate_denominator = gst_value_get_fraction_denominator (framerate);

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
	GstPad* otherpad;
	GstPad* otherpeer;
	GstCaps* othercaps = NULL;
	gboolean ret = TRUE;
	gboolean peer_checked = FALSE;

	self = GST_GOO_VIDEO_FILTER (gst_pad_get_parent (pad));
	klass = GST_GOO_VIDEO_FILTER_GET_CLASS (self);

	GST_DEBUG_OBJECT (self, "set caps");

	g_return_val_if_fail (GST_CAPS_IS_SIMPLE (caps), FALSE);

	otherpad = (pad == self->srcpad) ? self->sinkpad : self->srcpad;
	otherpeer = gst_pad_get_peer (otherpad);
#define DUMP_PAD(x) GST_DEBUG (#x "=0x%08x (%s)", (x), (x) ? GST_PAD_NAME (x) : "")
	DUMP_PAD(otherpad);
	DUMP_PAD(otherpeer);
	DUMP_PAD(self->srcpad);
	DUMP_PAD(self->sinkpad);

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
		DEBUG_CAPS ("FAILED to configure caps %s", othercaps);
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

