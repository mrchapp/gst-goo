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

#include <string.h>

#include <gst/video/video.h>
#include <goo-ti-jpegenc.h>

#include "gstgooencjpeg.h"
#include "gstgoobuffer.h"
#include "gstghostbuffer.h"
#include "gstgooutils.h"

GST_DEBUG_CATEGORY_STATIC (gst_goo_encjpeg_debug);
#define GST_CAT_DEFAULT gst_goo_encjpeg_debug

/* signals */
enum
{
	FRAME_ENCODED,
	LAST_SIGNAL
};

static guint gst_goo_encjpeg_signals[LAST_SIGNAL] = { 0 };

/* args */
enum
{
	PROP_0,
	PROP_QUALITY,
	PROP_COMMENT,
	PROP_THUMBNAIL
	/*PROP_NUM_INPUT_BUFFERS,
	PROP_NUM_OUTPUT_BUFFERS*/
};

#define GST_GOO_ENCJPEG_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_GOO_ENCJPEG, GstGooEncJpegPrivate))

struct _GstGooEncJpegPrivate
{
	guint num_input_buffers;
	guint num_output_buffers;

	guint incount;
	guint outcount;

	guint quality;
	guint width;
	guint height;
	gboolean APP0_Index;
	guint APP0_width;
	guint APP0_height;
	gboolean APP1_Index;
	guint APP1_width;
	guint APP1_height;
	gboolean APP13_Index;
	guint APP13_width;
	guint APP13_height;
	guint colorformat;
	guint omxbufsiz;
};

#define COMMENT_DEFAULT ""
#define THUMBNAIL_DEFAULT ""
#define QUALITY_DEFAULT 80
#define NUM_INPUT_BUFFERS_DEFAULT 1
#define NUM_OUTPUT_BUFFERS_DEFAULT 1
#define WIDTH_DEFAULT 1024
#define HEIGHT_DEFAULT 780
#define COLOR_FORMAT_DEFAULT OMX_COLOR_FormatCbYCrY

static const GstElementDetails details =
	GST_ELEMENT_DETAILS (
		"OpenMAX JPEG encoder",
		"Codec/Encoder/Image",
		"Encodes images in JPEG format with OpenMAX",
		"Texas Instrument"
		);

static GstStaticPadTemplate src_factory =
	GST_STATIC_PAD_TEMPLATE (
		"src",
		GST_PAD_SRC,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS ("image/jpeg, "
				 "width = (int) [ 16, MAX ], "
				 "height = (int) [ 16, MAX ], "
				 "framerate = (fraction) [ 0/1, MAX ]")
		);

static GstStaticPadTemplate sink_factory =
	GST_STATIC_PAD_TEMPLATE (
		"sink",
		GST_PAD_SINK,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS (GST_VIDEO_CAPS_YUV ("{ I420, UYVY }"))
		);

GST_BOILERPLATE (GstGooEncJpeg, gst_goo_encjpeg, GstElement, GST_TYPE_ELEMENT);

static guint
get_width_and_height (GObject* object, GValue* value)
{
	GstGooEncJpegPrivate* priv = GST_GOO_ENCJPEG_GET_PRIVATE (object);
	gchar* thumbnail = g_value_dup_string (value);
	guint error = 0;
	guint aux, number;

	for(aux = 0, number = 0; (thumbnail[aux] != 'x') && (thumbnail[aux] != 'X') && (strlen (thumbnail) > aux);aux++)
	{
		if((thumbnail[aux] >= 0x30) && (thumbnail[aux] <= 0x39))
		{
			number *= 10;
			number += (guint)(thumbnail[aux] & 0x0F);
			priv->width = number;
		}
		else
		{
			error = 1;
			aux = strlen (thumbnail);
			priv->width = 0;
			break;
		}
	}
	for(aux++, number = 0; (strlen (thumbnail)) > aux;aux++)
	{
		if((thumbnail[aux] >= 0x30) && (thumbnail[aux] <= 0x39))
		{
			number *= 10;
			number += (guint)(thumbnail[aux] & 0x0F);
			priv->height = number;
		}
		else
		{
			error = 1;
			aux = strlen (thumbnail);
			priv->height = 0;
			break;
		}
	}
	g_free(thumbnail);
	return error;
}

static void
omx_sync (GstGooEncJpeg* self)
{
	GstGooEncJpegPrivate* priv = GST_GOO_ENCJPEG_GET_PRIVATE (self);

	GST_DEBUG ("configuring params");
	OMX_PARAM_PORTDEFINITIONTYPE* param;

	param = GOO_PORT_GET_DEFINITION (self->inport);
	param->format.image.nFrameWidth = priv->width;
	param->format.image.nFrameHeight = priv->height;
	param->format.image.eColorFormat = priv->colorformat;

	param = GOO_PORT_GET_DEFINITION (self->outport);
	param->format.image.nFrameWidth = priv->width;
	param->format.image.nFrameHeight = priv->height;
	param->format.image.eColorFormat = priv->colorformat;

	GOO_TI_JPEGENC_GET_PARAM (self->component)->nQFactor = priv->quality;

	g_object_set (self->inport,
		      "buffercount", priv->num_input_buffers, NULL);
	g_object_set (self->outport,
		      "buffercount", priv->num_output_buffers, NULL);

	return;
}

static gboolean
omx_start (GstGooEncJpeg* self)
{
	g_assert (self != NULL);
	g_assert (self->component != NULL);

	GST_OBJECT_LOCK (self);
	if (goo_component_get_state (self->component) == OMX_StateLoaded)
	{
		omx_sync (self);

		GST_INFO_OBJECT (self, "going to idle");
		goo_component_set_state_idle (self->component);
	}

	if (goo_component_get_state (self->component) == OMX_StateIdle ||
	    goo_component_get_state (self->component) == OMX_StatePause)
	{
		GST_INFO_OBJECT (self, "going to executing");
		goo_component_set_state_executing (self->component);
	}
	GST_OBJECT_UNLOCK (self);

	return TRUE;
}

static gboolean
omx_stop (GstGooEncJpeg* self)
{
	g_assert (self != NULL);
	g_assert (self->component != NULL);

	GST_OBJECT_LOCK (self);
	if (goo_component_get_state (self->component) == OMX_StateExecuting ||
	    goo_component_get_state (self->component) == OMX_StatePause)
	{
		GST_INFO ("going to idle");
		goo_component_set_state_idle (self->component);
	}

	if (goo_component_get_state (self->component) == OMX_StateIdle)
	{
		GST_INFO ("going to loaded");
		goo_component_set_state_loaded (self->component);
	}
	GST_OBJECT_UNLOCK (self);

	GST_DEBUG ("");

	return TRUE;
}

static gboolean
gst_goo_encjpeg_setcaps (GstPad* pad, GstCaps* caps)
{
	GstGooEncJpeg* self = GST_GOO_ENCJPEG (gst_pad_get_parent (pad));
	GstGooEncJpegPrivate* priv = GST_GOO_ENCJPEG_GET_PRIVATE (self);

	GstStructure* structure;
	const GValue* framerate;
	guint32 fourcc;
	GstPad* otherpad;
	GstCaps* othercaps;
	gboolean ret;

	otherpad = (pad == self->srcpad) ? self->sinkpad : self->srcpad;
	othercaps = gst_caps_copy (gst_pad_get_pad_template_caps (otherpad));

	structure = gst_caps_get_structure (caps, 0);

	gst_structure_get_int (structure, "width", &priv->width);
	gst_structure_get_int (structure, "height", &priv->height);
	gst_structure_get_fourcc (structure, "format", &fourcc);

	switch (fourcc)
	{
	case GST_MAKE_FOURCC ('U', 'Y', 'V', 'Y'):
		priv->colorformat = OMX_COLOR_FormatCbYCrY;
		break;
	case GST_MAKE_FOURCC ('I', '4', '2', '0'):
		priv->colorformat = OMX_COLOR_FormatYUV420PackedPlanar;
		break;
	default:
		GST_ERROR ("format not supported");
		return FALSE;
	}

	g_object_set (self->component, "width", priv->width, NULL);
	g_object_set (self->component, "height", priv->height, NULL);

	priv->omxbufsiz = (priv->colorformat == OMX_COLOR_FormatCbYCrY) ?
		priv->width * priv->height * 2 :
		priv->width * priv->height * 1.5;

	framerate = gst_structure_get_value (structure, "framerate");

	if (G_LIKELY (framerate))
	{
		gst_caps_set_simple
			(othercaps,
			 "width", G_TYPE_INT, priv->width,
			 "height", G_TYPE_INT, priv->height,
			 "framerate", GST_TYPE_FRACTION,
			 gst_value_get_fraction_numerator (framerate),
			 gst_value_get_fraction_denominator (framerate),
			 NULL);
	}
	else
	{
		gst_caps_set_simple
			(othercaps,
			 "width", G_TYPE_INT, priv->width,
			 "height", G_TYPE_INT, priv->height,
			 NULL);
	}

	ret = gst_pad_set_caps (self->srcpad, othercaps);
	gst_caps_unref (othercaps);

	if (GST_PAD_LINK_SUCCESSFUL (ret) &&
	    goo_component_get_state (self->component) == OMX_StateLoaded)
	{
		omx_start (self);
	}

	gst_object_unref (self);

	return ret;
}

static GstFlowReturn
gst_goo_encjpeg_buffer_alloc (GstPad *pad, guint64 offset, guint size,
			      GstCaps *caps, GstBuffer **buf)
{
	GstGooEncJpeg* self = GST_GOO_ENCJPEG (gst_pad_get_parent (pad));
	GstFlowReturn ret = GST_FLOW_OK;

	GST_DEBUG ("a buffer of %d bytes was requested with caps %"
		   GST_PTR_FORMAT " and offset %llu", size, caps, offset);

	if (self->component->cur_state == OMX_StateLoaded)
	{
		gst_goo_encjpeg_setcaps (pad, caps);
	}

	OMX_BUFFERHEADERTYPE* omx_buffer;
	omx_buffer = goo_port_grab_buffer (self->inport);
	GstBuffer* gst_buffer = GST_BUFFER (gst_goo_buffer_new ());
	gst_goo_buffer_set_data (gst_buffer, self->component, omx_buffer);
	gst_buffer_set_caps (gst_buffer, caps);

	*buf = gst_buffer;

	gst_object_unref (self);

	return ret;
}

static GstFlowReturn
gst_goo_encjpeg_chain (GstPad* pad, GstBuffer* buffer)
{
	GST_LOG ("");

	GstGooEncJpeg* self = GST_GOO_ENCJPEG (gst_pad_get_parent (pad));
	GstGooEncJpegPrivate* priv = GST_GOO_ENCJPEG_GET_PRIVATE (self);
	GstFlowReturn ret = GST_FLOW_OK;
	GstGooAdapter* adapter = self->adapter;
	OMX_BUFFERHEADERTYPE* omx_buffer = NULL;

	GstClockTime timestamp, duration;
	guint64 offset, offsetend;
	GstBuffer* outbuf = NULL;

	if (goo_port_is_tunneled (self->inport))
	{
		GST_INFO ("Inport is tunneled");
		ret = GST_FLOW_OK;
		priv->incount++;
		goto process_output;
	}

	if (goo_port_is_eos (self->inport))
	{
		GST_INFO ("port is eos");
		ret = GST_FLOW_UNEXPECTED;
		goto fail;
	}

	if (self->component->cur_state != OMX_StateExecuting)
	{
		goto fail;
	}

	/* let's copy the timestamp meta data */
	timestamp = GST_BUFFER_TIMESTAMP (buffer);
	duration  = GST_BUFFER_DURATION (buffer);
	offset	  = GST_BUFFER_OFFSET (buffer);
	offsetend = GST_BUFFER_OFFSET_END (buffer);

	if (GST_IS_GOO_BUFFER (buffer) &&
	    goo_port_is_my_buffer (self->inport,
				   GST_GOO_BUFFER (buffer)->omx_buffer))
	{
		GST_INFO ("My own OMX buffer");
		priv->incount++;
		gst_buffer_unref (buffer); /* let's push the buffer to omx */
		ret = GST_FLOW_OK;
	}
	else if (GST_IS_GOO_BUFFER (buffer) &&
		 !goo_port_is_my_buffer (self->inport,
					 GST_GOO_BUFFER (buffer)->omx_buffer))
	{
		GST_INFO ("Other OMX buffer");

		if (GST_BUFFER_SIZE (buffer) != priv->omxbufsiz)
		{
			GST_ELEMENT_ERROR (self, STREAM, FORMAT,
					   ("Frame is incomplete (%u!=%u)",
					    GST_BUFFER_SIZE (buffer),
					    priv->omxbufsiz),
					   ("Frame is incomplete (%u!=%u)",
					    GST_BUFFER_SIZE (buffer),
					    priv->omxbufsiz));
			ret = GST_FLOW_ERROR;
		}

		omx_buffer = goo_port_grab_buffer (self->inport);
		memcpy (omx_buffer->pBuffer, GST_BUFFER_DATA (buffer),
			priv->omxbufsiz);
		omx_buffer->nFilledLen = priv->omxbufsiz;
		priv->incount++;
		goo_component_release_buffer (self->component, omx_buffer);
		gst_buffer_unref (buffer);
		ret = GST_FLOW_OK;
	}
	else
	{
		if (GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DISCONT))
		{
			gst_goo_adapter_clear (adapter);
		}

		GST_LOG ("size = %d bytes", GST_BUFFER_SIZE (buffer));
		gst_goo_adapter_push (adapter, buffer);

		guint tmp = priv->incount;
		while (gst_goo_adapter_available (adapter) >= priv->omxbufsiz &&
		       ret == GST_FLOW_OK)
		{
			GST_DEBUG ("Pushing data to OMX");
			OMX_BUFFERHEADERTYPE* omx_buffer;
			omx_buffer = goo_port_grab_buffer (self->inport);
			gst_goo_adapter_peek (adapter, priv->omxbufsiz,
					      omx_buffer);
			omx_buffer->nFilledLen = priv->omxbufsiz;
			gst_goo_adapter_flush (adapter, priv->omxbufsiz);
			priv->incount++;
			goo_component_release_buffer (self->component,
						      omx_buffer);
			ret = GST_FLOW_OK;
		}

		if (tmp == priv->incount)
		{
			goto done;
		}
	}

process_output:
	if (goo_port_is_tunneled (self->outport))
	{
		outbuf = GST_BUFFER (gst_ghost_buffer_new ());
		GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_READONLY);
		GST_BUFFER_DATA (outbuf)       = NULL;
		GST_BUFFER_SIZE (outbuf)       = 0;
		GST_BUFFER_TIMESTAMP (outbuf)  = timestamp;
		GST_BUFFER_DURATION (outbuf)   = duration;
		GST_BUFFER_OFFSET (outbuf)     = offset;
		GST_BUFFER_OFFSET_END (outbuf) = offsetend;
		gst_buffer_set_caps (outbuf, GST_PAD_CAPS (self->srcpad));
		gst_pad_push (self->srcpad, outbuf);
		goto done;
	}

	GST_DEBUG ("Poping out buffer from OMX");
	omx_buffer = goo_port_grab_buffer (self->outport);

	if (omx_buffer->nFilledLen <= 0)
	{
		ret = GST_FLOW_ERROR;
		goto done;
	}

	if (gst_pad_alloc_buffer (self->srcpad, priv->outcount,
				  omx_buffer->nFilledLen,
				  GST_PAD_CAPS (self->srcpad),
				  &outbuf) == GST_FLOW_OK)
	{
		priv->outcount++;

		/* if the buffer is a goo buffer of the peer element */
		if (GST_IS_GOO_BUFFER (outbuf))
		{
			GST_INFO ("It is a OMX buffer!");
			memcpy (GST_GOO_BUFFER (outbuf)->omx_buffer->pBuffer,
				omx_buffer->pBuffer, omx_buffer->nFilledLen);
			GST_GOO_BUFFER (outbuf)->omx_buffer->nFilledLen =
				omx_buffer->nFilledLen;
			GST_GOO_BUFFER (outbuf)->omx_buffer->nFlags =
				omx_buffer->nFlags;
			GST_GOO_BUFFER (outbuf)->omx_buffer->nTimeStamp =
				GST2OMX_TIMESTAMP (timestamp);
			goo_component_release_buffer (self->component,
						      omx_buffer);
		}
		else
		{
			/* @fixme! */
			/* we do this because there is a buffer extarbation
			 * when a filesink is used.
			 * Maybe using multiple buffers it could be solved.
			 */
			memcpy (GST_BUFFER_DATA (outbuf),
				omx_buffer->pBuffer, omx_buffer->nFilledLen);
			goo_component_release_buffer (self->component,
						      omx_buffer);

/* 			gst_buffer_unref (outbuf); */
/* 			outbuf = GST_BUFFER (gst_goo_buffer_new ()); */
/* 			gst_goo_buffer_set_data (outbuf, */
/* 						 self->component, */
/* 						 omx_buffer); */
		}

		GST_BUFFER_TIMESTAMP (outbuf)  = timestamp;
		GST_BUFFER_DURATION (outbuf)   = duration;
		GST_BUFFER_OFFSET (outbuf)     = offset;
		GST_BUFFER_OFFSET_END (outbuf) = offsetend;

		gst_buffer_set_caps (outbuf, GST_PAD_CAPS (self->srcpad));
		g_signal_emit (G_OBJECT (self),
			       gst_goo_encjpeg_signals[FRAME_ENCODED], 0);

		ret = gst_pad_push (self->srcpad, outbuf);
		if (omx_buffer->nFlags & OMX_BUFFERFLAG_EOS ||
		   	goo_port_is_eos (self->outport))
		{
			GST_INFO ("EOS flag found in output buffer (%d)",
			  	omx_buffer->nFilledLen);
			goo_component_set_done (self->component);

		}

		goto done;
	}
	else
	{
		ret = GST_FLOW_ERROR;
		goto done;
	}

fail:
	gst_goo_adapter_clear (adapter);

done:
	gst_object_unref (self);
	gst_buffer_unref (buffer);
	return ret;

}

static GstStateChangeReturn
gst_goo_encjpeg_change_state (GstElement* element, GstStateChange transition)
{
	GST_LOG ("");

	GstGooEncJpeg* self = GST_GOO_ENCJPEG (element);
	GstStateChange result;

	g_assert (self->component != NULL);
	g_assert (self->inport != NULL);
	g_assert (self->outport != NULL);

	switch (transition)
	{
	case GST_STATE_CHANGE_NULL_TO_READY:
		break;
	case GST_STATE_CHANGE_READY_TO_PAUSED:
	{
		GstGooEncJpegPrivate* priv =
			GST_GOO_ENCJPEG_GET_PRIVATE (self);
		priv->num_input_buffers = NUM_INPUT_BUFFERS_DEFAULT;
		priv->num_output_buffers = NUM_OUTPUT_BUFFERS_DEFAULT;
		priv->incount = 0;
		priv->outcount = 0;
		priv->quality = QUALITY_DEFAULT;
		priv->colorformat = COLOR_FORMAT_DEFAULT;
		priv->width = WIDTH_DEFAULT;
		priv->height = HEIGHT_DEFAULT;
		priv->omxbufsiz = 0;
		break;
	}
	case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
		GST_OBJECT_LOCK (self);
		if (goo_component_get_state (self->component) ==
		   	OMX_StatePause)
		{
			goo_component_set_state_executing (self->component);
		}
		GST_OBJECT_UNLOCK (self);
		break;
	default:
		break;
	}

	result = GST_ELEMENT_CLASS (parent_class)->change_state (element,
								 transition);

	switch (transition)
	{
	case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
		GST_OBJECT_LOCK (self);
		goo_component_set_state_pause (self->component);
		GST_OBJECT_UNLOCK (self);
		break;
	case GST_STATE_CHANGE_PAUSED_TO_READY:
		if ( ! (goo_port_is_tunneled (self->inport)) )
		{
		omx_stop (self);
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
gst_goo_encjpeg_set_property (GObject* object, guint prop_id,
			      const GValue* value, GParamSpec* pspec)
{
	GstGooEncJpegPrivate* priv = GST_GOO_ENCJPEG_GET_PRIVATE (object);
	GstGooEncJpeg* self = GST_GOO_ENCJPEG (object);

	switch (prop_id)
	{
/*	case PROP_NUM_INPUT_BUFFERS: */
/*		priv->num_input_buffers = g_value_get_uint (value); */
/*		break; */
/*	case PROP_NUM_OUTPUT_BUFFERS: */
/*		priv->num_output_buffers = g_value_get_uint (value); */
/*		break; */
	case PROP_QUALITY:
		priv->quality = g_value_get_uint (value);
		break;
	case PROP_COMMENT:
	{
		gchar* comment = g_value_dup_string (value);
		g_object_set (self->component, "comment", comment, NULL);
		g_free (comment);
		break;
	}
	case PROP_THUMBNAIL:
	{
		if(!get_width_and_height(object, (GValue*)value))
		{
			priv->APP0_Index = (gboolean)TRUE;
			priv->APP1_Index = (gboolean)TRUE;
			priv->APP13_Index = (gboolean)TRUE;
			priv->APP0_width = priv->width;
			priv->APP0_height = priv->height;
			priv->APP1_width = priv->width;
			priv->APP1_height = priv->height;
			priv->APP13_width = priv->width;
			priv->APP13_height = priv->height;
			g_object_set (self->component, "app0i", priv->APP0_Index, NULL);
			g_object_set (self->component, "app0w", priv->APP0_width, NULL);
			g_object_set (self->component, "app0h", priv->APP0_height, NULL);
			g_object_set (self->component, "app1i", priv->APP1_Index, NULL);
			g_object_set (self->component, "app1w", priv->APP1_width, NULL);
			g_object_set (self->component, "app1h", priv->APP1_height, NULL);
			//g_object_set (self->component, "app13i", priv->APP13_Index, NULL);
			//g_object_set (self->component, "app13w", priv->APP13_width, NULL);
			//g_object_set (self->component, "app13h", priv->APP13_height, NULL);
		}
		break;
	}
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;

	}

	return;
}

static void
gst_goo_encjpeg_get_property (GObject* object, guint prop_id,
			      GValue* value, GParamSpec* pspec)
{
	GstGooEncJpegPrivate* priv = GST_GOO_ENCJPEG_GET_PRIVATE (object);
	GstGooEncJpeg* self = GST_GOO_ENCJPEG (object);

	switch (prop_id)
	{
/*	case PROP_NUM_INPUT_BUFFERS: */
/*		g_value_set_uint (value, priv->num_input_buffers); */
/*		break; */
/*	case PROP_NUM_OUTPUT_BUFFERS: */
/*		g_value_set_uint (value, priv->num_output_buffers); */
/*		break; */
	case PROP_QUALITY:
		g_value_set_uint (value, priv->quality);
		break;
/*	   case PROP_COMMENT: */
/*		   gchar* comment; */
/*		   g_object_get (self->component, "comment", comment, NULL); */
/*		   g_value_set_string (value, comment); */
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;

	}

	return;
}

static void
gst_goo_encjpeg_dispose (GObject* object)
{
	G_OBJECT_CLASS (parent_class)->dispose (object);

	GstGooEncJpeg* me = GST_GOO_ENCJPEG (object);

	if (G_LIKELY (me->adapter))
	{
		GST_DEBUG ("unrefing adapter");
		g_object_unref (me->adapter);
	}

	if (G_LIKELY (me->inport))
	{
		GST_DEBUG ("unrefing outport");
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
		G_OBJECT(me->component)->ref_count = 1;
		g_object_unref (me->component);
	}

	if (G_LIKELY (me->factory))
	{
		GST_DEBUG ("unrefing factory");
		g_object_unref (me->factory);
	}

	return;
}

static void
gst_goo_encjpeg_base_init (gpointer g_klass)
{
	GST_DEBUG_CATEGORY_INIT (gst_goo_encjpeg_debug, "gooencjpeg", 0,
				 "OpenMAX JPEG encoder element");

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

static void
gst_goo_encjpeg_class_init (GstGooEncJpegClass* klass)
{
	GObjectClass* g_klass;
	GParamSpec* pspec;
	GstElementClass* gst_klass;

	g_klass = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GstGooEncJpegPrivate));

	g_klass->set_property = GST_DEBUG_FUNCPTR (gst_goo_encjpeg_set_property);
	g_klass->get_property = GST_DEBUG_FUNCPTR (gst_goo_encjpeg_get_property);
	g_klass->dispose = GST_DEBUG_FUNCPTR (gst_goo_encjpeg_dispose);

/*	pspec = g_param_spec_uint ("input-buffers", "Input buffers", */
/*				   "The number of OMX input buffers", */
/*				   1, 10, NUM_INPUT_BUFFERS_DEFAULT, */
/*				   G_PARAM_READWRITE); */
/*	g_object_class_install_property (g_klass, PROP_NUM_INPUT_BUFFERS, */
/*					 pspec); */

/*	pspec = g_param_spec_uint ("output-buffers", "Output buffers", */
/*				   "The number of OMX output buffers", */
/*				   1, 10, NUM_OUTPUT_BUFFERS_DEFAULT, */
/*				   G_PARAM_READWRITE); */
/*	g_object_class_install_property (g_klass, PROP_NUM_OUTPUT_BUFFERS, */
/*					 pspec); */
	/*Quality*/
	pspec = g_param_spec_uint ("quality", "Quality","Quality of encoding",
								0, 100, QUALITY_DEFAULT, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_QUALITY, pspec);
	/*Comment*/
	pspec = g_param_spec_string ("comment", "Comment", "Image comment",
								COMMENT_DEFAULT, G_PARAM_WRITABLE);
	g_object_class_install_property (g_klass, PROP_COMMENT, pspec);
	/*Thumbnail*/
	pspec = g_param_spec_string ("thumbnail", "Thumbnail", "Image thumbnail",
								THUMBNAIL_DEFAULT, G_PARAM_WRITABLE);
	g_object_class_install_property (g_klass, PROP_THUMBNAIL, pspec);

	gst_goo_encjpeg_signals[FRAME_ENCODED] =
		g_signal_new ("frame-encoded", G_TYPE_FROM_CLASS (klass),
			    G_SIGNAL_RUN_LAST,
			    G_STRUCT_OFFSET (GstGooEncJpegClass,
				frame_encoded),
				NULL,
				NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0);

	/* GST */
	gst_klass = GST_ELEMENT_CLASS (klass);
	gst_klass->change_state =
		GST_DEBUG_FUNCPTR (gst_goo_encjpeg_change_state);

	return;
}

static void
gst_goo_encjpeg_init (GstGooEncJpeg* self, GstGooEncJpegClass* klass)
{
	GST_DEBUG ("");

	self->factory = goo_ti_component_factory_get_instance ();
	self->component = goo_component_factory_get_component(self->factory, GOO_TI_JPEG_ENCODER);

	/* param */
	{
		GOO_TI_JPEGENC_GET_PARAM (self->component)->nQFactor =
			QUALITY_DEFAULT;
	}

	/* inport */
	{
		self->inport = goo_component_get_port (self->component,
						       "input0");
		g_assert (self->inport != NULL);

		OMX_PARAM_PORTDEFINITIONTYPE* param;
		param = GOO_PORT_GET_DEFINITION (self->inport);
		param->format.image.nFrameWidth = WIDTH_DEFAULT;
		param->format.image.nFrameHeight = HEIGHT_DEFAULT;
		param->format.image.eColorFormat = COLOR_FORMAT_DEFAULT;

		g_object_set (self->inport,
			      "buffercount", NUM_INPUT_BUFFERS_DEFAULT, NULL);
	}

	/* outport */
	{
		self->outport = goo_component_get_port (self->component,
							"output0");
		g_assert (self->outport != NULL);

		OMX_PARAM_PORTDEFINITIONTYPE* param;
		param = GOO_PORT_GET_DEFINITION (self->outport);
		param->format.image.nFrameWidth = WIDTH_DEFAULT;
		param->format.image.nFrameHeight = HEIGHT_DEFAULT;
		param->format.image.eColorFormat = COLOR_FORMAT_DEFAULT;

		g_object_set (self->outport, "buffercount", NUM_INPUT_BUFFERS_DEFAULT, NULL);
	}

	GstGooEncJpegPrivate* priv = GST_GOO_ENCJPEG_GET_PRIVATE (self);
	priv->num_input_buffers = NUM_INPUT_BUFFERS_DEFAULT;
	priv->num_output_buffers = NUM_OUTPUT_BUFFERS_DEFAULT;
	priv->incount = 0;
	priv->outcount = 0;
	priv->quality = QUALITY_DEFAULT;
	priv->colorformat = COLOR_FORMAT_DEFAULT;
	priv->width = WIDTH_DEFAULT;
	priv->height = HEIGHT_DEFAULT;
	priv->omxbufsiz = 0;

	/* GST */
	GstPadTemplate* pad_template;

	pad_template = gst_element_class_get_pad_template
		(GST_ELEMENT_CLASS (klass), "sink");
	g_return_if_fail (pad_template != NULL);
	self->sinkpad = gst_pad_new_from_template (pad_template, "sink");
	gst_pad_set_chain_function
		(self->sinkpad, GST_DEBUG_FUNCPTR (gst_goo_encjpeg_chain));
	gst_pad_set_setcaps_function
		(self->sinkpad, GST_DEBUG_FUNCPTR (gst_goo_encjpeg_setcaps));
/*	gst_pad_set_bufferalloc_function
		(self->sinkpad, GST_DEBUG_FUNCPTR (gst_goo_encjpeg_buffer_alloc));   */
	gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);

	pad_template = gst_element_class_get_pad_template
		(GST_ELEMENT_CLASS (klass), "src");
	g_return_if_fail (pad_template != NULL);
	self->srcpad = gst_pad_new_from_template (pad_template, "src");
	gst_element_add_pad (GST_ELEMENT (self), self->srcpad);

	self->adapter = gst_goo_adapter_new ();

	g_object_set_data (G_OBJECT (self->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", self->component);

	return;
}
