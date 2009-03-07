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

#include <string.h>

#include "gstgootransform.h"
#include "gstgoobuffer.h"

GST_DEBUG_CATEGORY_STATIC (gst_goo_transform_debug);
#define GST_CAT_DEFAULT gst_goo_transform_debug

#define NUM_INPUT_BUFFERS_DEFAULT 4
#define NUM_OUTPUT_BUFFERS_DEFAULT 4

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
	PROP_NUM_OUTPUT_BUFFERS
};

#define GST_GOO_TRANSFORM_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_GOO_TRANSFORM, GstGooTransformPrivate))

struct _GstGooTransformPrivate
{
	guint num_input_buffers;
	guint num_output_buffers;

	guint incount;
	guint outcount;
};

/**
 * gst_goo_transform_outport_buffer:
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
gst_goo_transform_outport_buffer (GooPort* port, OMX_BUFFERHEADERTYPE* buffer,
				  gpointer data)
{
	g_return_if_fail (buffer->nFlags != OMX_BUFFERFLAG_DATACORRUPT);

	GST_DEBUG ("");

	g_assert (GOO_IS_PORT (port));
	g_assert (buffer != NULL);
	g_assert (GOO_IS_COMPONENT (data));

	GooComponent* component = GOO_COMPONENT (data);
	GstGooTransform* self =
		GST_GOO_TRANSFORM (g_object_get_data (G_OBJECT (data), "gst"));
	g_assert (self != NULL);
	GstGooTransformPrivate* priv = GST_GOO_TRANSFORM_GET_PRIVATE (self);

	GstBuffer* gst_buffer = gst_goo_buffer_new ();
	gst_goo_buffer_set_data (gst_buffer, component, buffer);
	priv->outcount++;

	/** @todo timestamp the buffer */
	GST_BUFFER_OFFSET (gst_buffer) = priv->outcount;
	gst_buffer_set_caps (gst_buffer, GST_PAD_CAPS (self->srcpad));
	gst_pad_push (self->srcpad, gst_buffer);

	if (buffer->nFlags == OMX_BUFFERFLAG_EOS)
	{
		GST_INFO ("EOS flag found in output buffer (%d)",
			  buffer->nFilledLen);
		goo_component_set_done (self->component);
	}

	return;
}

GST_BOILERPLATE (GstGooTransform, gst_goo_transform, GstElement,
                 GST_TYPE_ELEMENT);

static void
gst_goo_transform_set_property (GObject* object, guint prop_id,
				const GValue* value, GParamSpec* pspec)
{
	GstGooTransformPrivate* priv = GST_GOO_TRANSFORM_GET_PRIVATE (object);

	switch (prop_id)
	{
	case PROP_NUM_INPUT_BUFFERS:
		priv->num_input_buffers = g_value_get_uint (value);
		break;
	case PROP_NUM_OUTPUT_BUFFERS:
		priv->num_output_buffers = g_value_get_uint (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_transform_get_property (GObject* object, guint prop_id,
				GValue* value, GParamSpec* pspec)
{
	GstGooTransformPrivate* priv = GST_GOO_TRANSFORM_GET_PRIVATE (object);

	switch (prop_id)
	{
	case PROP_NUM_INPUT_BUFFERS:
		g_value_set_uint (value, priv->num_input_buffers);
		break;
	case PROP_NUM_OUTPUT_BUFFERS:
		g_value_set_uint (value, priv->num_output_buffers);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_transform_dispose (GObject* object)
{
        GstGooTransform* me;

        G_OBJECT_CLASS (parent_class)->dispose (object);

        me = GST_GOO_TRANSFORM (object);

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
		GST_DEBUG ("unrefing component");
                g_object_unref (me->component);
        }

	if (G_LIKELY (me->factory))
	{
		GST_DEBUG ("unrefing factory");
		g_object_unref (me->factory);
	}

        return;
}

static GstStateChangeReturn
gst_goo_transform_change_state (GstElement* element, GstStateChange transition)
{
	GST_LOG ("");

	GstGooTransform* self = GST_GOO_TRANSFORM (element);
	GstStateChangeReturn result;
	GstGooTransformPrivate* priv = GST_GOO_TRANSFORM_GET_PRIVATE (self);

	g_assert (self->component != NULL);
	g_assert (self->inport != NULL);
	g_assert (self->outport != NULL);

	switch (transition)
	{
	case GST_STATE_CHANGE_NULL_TO_READY:
		break;
	case GST_STATE_CHANGE_READY_TO_PAUSED:
		/* GST_STATE_PAUSED is the state in which an element is ready
		   to accept and handle data. For most elements this state is
		   the same as PLAYING. */
GST_INFO_OBJECT (self, "num_input_buffers=%d, num_output_buffers=%d", priv->num_input_buffers, priv->num_output_buffers);

		g_object_set (self->inport,
			      "buffercount", priv->num_input_buffers, NULL);
		g_object_set (self->outport,
			      "buffercount", priv->num_output_buffers, NULL);

		g_object_set_data (G_OBJECT (self->component), "gst", self);
		g_object_set_data (G_OBJECT (self), "goo", self->component);

		GST_INFO ("going to idle");
		goo_component_set_state_idle (self->component);
		GST_INFO ("going to executing");
		goo_component_set_state_executing (self->component);
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
gst_goo_transform_wait_for_done (GstGooTransform* self)
{
	g_assert (self != NULL);
	GstGooTransformPrivate* priv = GST_GOO_TRANSFORM_GET_PRIVATE (self);

	GST_INFO ("Sending EOS buffer");
	/* flushing the last buffers in adapter */

	OMX_BUFFERHEADERTYPE* omx_buffer;
	OMX_PARAM_PORTDEFINITIONTYPE* param =
		GOO_PORT_GET_DEFINITION (self->inport);
	GstAdapter* adapter = self->adapter;
	int omxbufsiz = param->nBufferSize;
	int avail = gst_adapter_available (adapter);

	if (avail < omxbufsiz && avail > 0)
	{
		omx_buffer = goo_port_grab_buffer (self->inport);
		GST_DEBUG ("memcpy to buffer %d bytes", avail);
		memcpy (omx_buffer->pBuffer,
			gst_adapter_peek (adapter, avail), avail);
		omx_buffer->nFilledLen = avail;
		/* let's send the EOS flag right now */
		omx_buffer->nFlags = OMX_BUFFERFLAG_EOS;
		/** @todo timestamp the buffers */
		priv->incount++;
		goo_component_release_buffer (self->component, omx_buffer);
	}
	else if (avail == 0)
	{
		goo_component_send_eos (self->component);
	}
	else
	{
		/* por alguna razon el while del chain no extrajo todos los
		   buffers posibles */
		GST_ERROR ("Adapter algorithm error!");
		goo_component_send_eos (self->component);
	}

	gst_adapter_clear (adapter);

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
gst_goo_transform_sink_event (GstPad* pad, GstEvent* event)
{
	GST_LOG ("");

	GstGooTransform* self = GST_GOO_TRANSFORM (gst_pad_get_parent (pad));

	gboolean ret;

	g_assert (self->component != NULL);

	switch (GST_EVENT_TYPE (event))
	{
	case GST_EVENT_EOS:
		GST_INFO ("EOS event");
		gst_goo_transform_wait_for_done (self);
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
gst_goo_transform_chain (GstPad* pad, GstBuffer* buffer)
{
	GST_LOG ("");

	GstGooTransform* self = GST_GOO_TRANSFORM (gst_pad_get_parent (pad));
	GstGooTransformPrivate* priv = GST_GOO_TRANSFORM_GET_PRIVATE (self);
	GstAdapter* adapter = self->adapter;
	GstFlowReturn ret = GST_FLOW_OK;

	if (goo_port_is_tunneled (self->inport))
	{
		/* shall we send a ghost buffer here ? */
		GST_INFO ("port is tunneled");
		ret = GST_FLOW_OK;
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
		gst_adapter_clear (adapter);
	}

	gst_adapter_push (adapter, buffer);
	int omxbufsiz = GOO_PORT_GET_DEFINITION (self->inport)->nBufferSize;
	while (gst_adapter_available (adapter) >= omxbufsiz &&
	       ret == GST_FLOW_OK)
	{
		OMX_BUFFERHEADERTYPE* omx_buffer;
		omx_buffer = goo_port_grab_buffer (self->inport);
		GST_DEBUG ("memcpy to buffer %d bytes", omxbufsiz);
		memcpy (omx_buffer->pBuffer,
			gst_adapter_peek (adapter, omxbufsiz), omxbufsiz);
		omx_buffer->nFilledLen = omxbufsiz;
		gst_adapter_flush (adapter, omxbufsiz);
		/** @todo timestamp the buffers */
		priv->incount++;
		goo_component_release_buffer (self->component, omx_buffer);
		ret = GST_FLOW_OK;
	}

	if (goo_port_is_tunneled (self->outport))
	{
		/** @todo send a ghost buffer */
	}

	goto done;

fail:
	gst_buffer_unref (buffer);
done:
	gst_object_unref (self);
	return ret;
}

static gboolean
gst_goo_transform_setcaps (GstPad* pad, GstCaps* caps)
{
	GstGooTransform* self = GST_GOO_TRANSFORM (gst_pad_get_parent (pad));
	gboolean ret;
	GST_INFO_OBJECT (self, "");

	GstGooTransformClass* klass = GST_GOO_TRANSFORM_GET_CLASS (self);

	if (GST_GOO_TRANSFORM_CLASS (klass)->setcaps_func != NULL)
	{
		ret = GST_GOO_TRANSFORM_CLASS (klass)->setcaps_func (pad,
								     caps);
	}
	else
	{
		ret = TRUE;
	}

	gst_object_unref (self);

	if (ret == FALSE)
	{
		return ret;
	}

	return gst_pad_set_caps (pad, caps);
}

static gboolean
gst_goo_transform_setcaps_default (GstPad *pad, GstCaps* caps)
{
	return TRUE;
}

static void
gst_goo_transform_base_init (gpointer g_klass)
{
        GST_DEBUG_CATEGORY_INIT (gst_goo_transform_debug, "gootransform", 0,
                                 "gootransform element");
}

static void
gst_goo_transform_class_init (GstGooTransformClass* klass)
{
        GObjectClass* g_klass;
	GParamSpec* pspec;
        GstElementClass* gst_klass;

	/* gobject */
        g_klass = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GstGooTransformPrivate));

        g_klass->set_property =
                GST_DEBUG_FUNCPTR (gst_goo_transform_set_property);
        g_klass->get_property =
                GST_DEBUG_FUNCPTR (gst_goo_transform_get_property);
        g_klass->dispose = GST_DEBUG_FUNCPTR (gst_goo_transform_dispose);

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

	/* GST */
	gst_klass = GST_ELEMENT_CLASS (klass);

        gst_klass->change_state =
                GST_DEBUG_FUNCPTR (gst_goo_transform_change_state);

	klass->setcaps_func = GST_DEBUG_FUNCPTR (gst_goo_transform_setcaps_default);

        return;
}

static void
gst_goo_transform_init (GstGooTransform* self, GstGooTransformClass* klass)
{
        GstPadTemplate* pad_template;

        GST_INFO ("");

        pad_template =
                gst_element_class_get_pad_template (GST_ELEMENT_CLASS (klass),
                                                    GST_GOO_TRANSFORM_SINK_NAME);
        g_return_if_fail (pad_template != NULL);

        self->sinkpad =
		gst_pad_new_from_template (pad_template,
					   GST_GOO_TRANSFORM_SINK_NAME);
        gst_pad_set_event_function (self->sinkpad,
                                    GST_DEBUG_FUNCPTR (gst_goo_transform_sink_event));
        gst_pad_set_chain_function (self->sinkpad,
                                    GST_DEBUG_FUNCPTR (gst_goo_transform_chain));
	gst_pad_set_setcaps_function (self->sinkpad,
				      GST_DEBUG_FUNCPTR (gst_goo_transform_setcaps));
	/* gst_pad_use_fixed_caps (self->sinkpad); */

	gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);


        pad_template =
                gst_element_class_get_pad_template (GST_ELEMENT_CLASS (klass),
                                                    GST_GOO_TRANSFORM_SRC_NAME);
        g_return_if_fail (pad_template != NULL);
        self->srcpad = gst_pad_new_from_template (pad_template,
                                                  GST_GOO_TRANSFORM_SRC_NAME);
	gst_pad_set_setcaps_function (self->srcpad,
				      GST_DEBUG_FUNCPTR (gst_pad_proxy_setcaps));
	/* gst_pad_use_fixed_caps (self->srcad); */
	gst_element_add_pad (GST_ELEMENT (self), self->srcpad);

	self->adapter = gst_adapter_new ();

	/* GOO */
	self->factory = NULL;
	self->component = NULL;
	self->inport = self->outport = NULL;

	GstGooTransformPrivate* priv = GST_GOO_TRANSFORM_GET_PRIVATE (self);
	priv->num_input_buffers = NUM_INPUT_BUFFERS_DEFAULT;
	priv->num_output_buffers = NUM_OUTPUT_BUFFERS_DEFAULT;
	priv->incount = 0;
	priv->outcount = 0;

	return;
}
