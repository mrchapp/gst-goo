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

#include <gst/video/video.h>
#include <string.h>

#include <goo-ti-post-processor.h>

#include "gstgoosinkpp.h"
#include "gstgoobuffer.h"

#define GST2OMX_TIMESTAMP(ts) (OMX_S64) ts / 1000;
#define OMX2GST_TIMESTAMP(ts) (guint64) ts * 1000;

#define DEFAULT_INPUT_BUFFERS 4
#define DEFAULT_COLOR_FORMAT  OMX_COLOR_FormatCbYCrY
#define DEFAULT_WIDTH	      176
#define DEFAULT_HEIGHT	      144
#define DEFAULT_XSCALE	      100
#define DEFAULT_YSCALE	      100
#define DEFAULT_XPOS	      0
#define DEFAULT_YPOS	      0
#define DEFAULT_MIRROR	      FALSE
#define DEFAULT_BGD	      0x000000
#define DEFAULT_ROTATION      GOO_TI_POST_PROCESSOR_ROTATION_NONE
#define DEFAULT_FRAMERATE     30
#define DEFAULT_LAYER	      1
#define DEFAULT_OUTPUT	      GOO_TI_POST_PROCESSOR_OUTPUT_LCD
#define DEFAULT_OPACITY 255				//full opacity

GST_BOILERPLATE (GstGooSinkPP, gst_goo_sinkpp,
		 GstVideoSink, GST_TYPE_VIDEO_SINK);

GST_DEBUG_CATEGORY_STATIC (gst_goo_sinkpp_debug);
#define GST_CAT_DEFAULT gst_goo_sinkpp_debug

static gboolean gst_goo_sinkpp_setcaps (GstBaseSink *bsink, GstCaps *caps);

/* args */
enum _GstGooSinkPPProp
{
	PROP_0,
	PROP_NUM_INPUT_BUFFERS,
	PROP_DISPLAY_BGD,
	PROP_DISPLAY_MIRROR,
	PROP_DISPLAY_POS_X,
	PROP_DISPLAY_POS_Y,
	PROP_DISPLAY_SCALE_WIDTH,
	PROP_DISPLAY_SCALE_HEIGHT,
	PROP_DISPLAY_ROTATION,
	PROP_VIDEO_PIPELINE,
       	PROP_OUTPUT_DEVICE,
	PROP_OPACITY,
	PROP_DISPLAY_CROP_LEFT,
	PROP_DISPLAY_CROP_TOP,
	PROP_DISPLAY_CROP_WIDTH,
	PROP_DISPLAY_CROP_HEIGHT,
};

#define GST_GOO_SINKPP_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_GOO_SINKPP, GstGooSinkPPPrivate))

struct _GstGooSinkPPPrivate
{
	guint incount;
	guint num_input_buffers;
	guint background;
	guint rotation;
	guint framerate;
	guint color;
	guint video_pipeline;
        guint opacity;
};

static const GstElementDetails details =
	GST_ELEMENT_DETAILS (
		"OpenMAX Post Processor sink",
		"Sink/Video",
		"Output video via the display driver",
		"Texas Instrument"
		);

static GstStaticPadTemplate sink_factory =
	GST_STATIC_PAD_TEMPLATE (
		"sink",
		GST_PAD_SINK,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS	("video/x-raw-yuv, "
				 "format = (fourcc) { YUY2, UYVY }, "
				 "width = (int) [ 2, 1280 ],"
				 "height = (int) [ 2, 1024 ], "
				 "framerate = " GST_VIDEO_FPS_RANGE ";"
				 "video/x-raw-rgb, "			\
				 "bpp = (int) 16, "			\
				 "depth = (int) 16, "			\
				 "endianness = (int) BYTE_ORDER, "	\
				 "red_mask = (int) " GST_VIDEO_RED_MASK_16 ", "	\
				 "green_mask = (int) " GST_VIDEO_GREEN_MASK_16 ", " \
				 "blue_mask = (int) " GST_VIDEO_BLUE_MASK_16 ", " \
				 "width = (int) [ 2, 1280 ], "		\
				 "height = (int) [ 2, 1024 ], "		\
				 "framerate = " GST_VIDEO_FPS_RANGE)
		);

#define GST_GOO_SINKPP_ROTATION \
	(goo_ti_post_processor_rotation_get_type ())

#define GST_GOO_SINKPP_OUTPUT \
	(goo_ti_post_processor_output_get_type ())

static gboolean
gst_goo_sinkpp_sync (GstGooSinkPP *self)
{
	GstGooSinkPPPrivate* priv = GST_GOO_SINKPP_GET_PRIVATE (self);
	GObject* cobj = G_OBJECT (self->component);

	OMX_PARAM_PORTDEFINITIONTYPE* param;
	param = GOO_PORT_GET_DEFINITION (self->inport);
	g_assert (param != NULL);

	/* inport */
	{
		param->format.video.xFramerate = priv->framerate;

		g_return_val_if_fail (GST_VIDEO_SINK_WIDTH (self) > 0, FALSE);
		param->format.video.nFrameWidth = GST_VIDEO_SINK_WIDTH (self);

		g_return_val_if_fail (GST_VIDEO_SINK_HEIGHT (self) > 0, FALSE);
		param->format.video.nFrameHeight = GST_VIDEO_SINK_HEIGHT (self);

		g_return_val_if_fail (priv->color > 0, FALSE);
		param->format.video.eColorFormat = priv->color;
	}

	/* param */
	{
		GOO_TI_POST_PROCESSOR_GET_BACKGROUND (self->component)->nColor
			= priv->background;

		g_return_val_if_fail (priv->video_pipeline == 1 ||
				      priv->video_pipeline == 2, FALSE);
		GOO_TI_POST_PROCESSOR (self->component)->video_pipeline =
			priv->video_pipeline;
	}

	g_object_set (self->inport,
		      "buffercount", priv->num_input_buffers, NULL);


	GST_INFO_OBJECT (self, "going to idle");
	goo_component_set_state_idle (self->component);
	GST_INFO_OBJECT (self, "going to executing");
	goo_component_set_state_executing (self->component);

	return TRUE;
}

static gboolean
gst_goo_sinkpp_setcaps (GstBaseSink *bsink, GstCaps *caps)
{
	GstGooSinkPP *self = GST_GOO_SINKPP (bsink);
	GstGooSinkPPPrivate* priv = GST_GOO_SINKPP_GET_PRIVATE (self);

	GST_DEBUG_OBJECT (self, "");

	g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE);

	GstStructure *structure;
	structure = gst_caps_get_structure (caps, 0);

	{
		const GValue *framerate;
		guint framerate_int = 15;

		framerate = gst_structure_get_value (structure, "framerate");

		if (framerate != NULL)
		{
			priv->framerate = (guint)
				gst_value_get_fraction_numerator (framerate) /
				gst_value_get_fraction_denominator (framerate);
		}
		else
		{
			priv->framerate = DEFAULT_FRAMERATE;
		}

		GST_DEBUG_OBJECT (self, "framerate = %d", priv->framerate);
	}

	{
		gint width;
		gst_structure_get_int (structure, "width", &width);
		GST_VIDEO_SINK_WIDTH (self) = width;
	}

	{
		gint height;
		gst_structure_get_int (structure, "height", &height);
		GST_VIDEO_SINK_HEIGHT (self) = height;
	}

	GST_DEBUG_OBJECT (self, "width = %d, height = %d",
			  GST_VIDEO_SINK_WIDTH (self),
			  GST_VIDEO_SINK_HEIGHT (self));

	if (g_strrstr (gst_structure_get_name (structure), "video/x-raw-yuv"))
	{
		guint32 fourcc;

		if (gst_structure_get_fourcc (structure, "format", &fourcc))
		{
			switch (fourcc)
			{
			case GST_MAKE_FOURCC ('I', '4', '2', '0'):
				priv->color = OMX_COLOR_FormatYUV420PackedPlanar;
				break;
			case GST_MAKE_FOURCC ('Y', 'U', 'Y', '2'):
				priv->color = OMX_COLOR_FormatYCbYCr;
				break;
			case GST_MAKE_FOURCC ('U', 'Y', 'V', 'Y'):
				priv->color = OMX_COLOR_FormatCbYCrY;
				break;
			default:
				GST_WARNING_OBJECT (self,
						    "No valid color space");
				return FALSE;
			}
		}
		else
		{
			GST_WARNING_OBJECT (self, "No format specified");
			return FALSE;
		}
	}
	else if (g_strrstr (gst_structure_get_name (structure),
			    "video/x-raw-rgb"))
	{
		gint depth;

		if (gst_structure_get_int (structure, "depth", &depth))
		{
			switch (depth)
			{
			case 16:
				priv->color = OMX_COLOR_Format16bitRGB565;
				break;
			default:
				GST_WARNING_OBJECT (self,
						    "No valid color space");
				return FALSE;
			}
		}
		else
		{
			GST_WARNING_OBJECT (self, "No depth specified");
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}

	gboolean ret = FALSE;

	if (goo_component_get_state (self->component) == OMX_StateExecuting)
	{
		GST_INFO ("going to idle");
		goo_component_set_state_idle (self->component);
	}

	if (goo_component_get_state (self->component) == OMX_StateIdle)
	{
		GST_INFO ("going to loaded");
		goo_component_set_state_loaded (self->component);
	}

	if (goo_component_get_state (self->component) == OMX_StateLoaded)
	{
		GST_OBJECT_LOCK (self);
		ret &= gst_goo_sinkpp_sync (self);
		GST_OBJECT_UNLOCK (self);
	}

	return TRUE;
}

static GstFlowReturn
gst_goo_sinkpp_render (GstBaseSink *sink, GstBuffer *buffer)
{
	GST_LOG ("");

	GstGooSinkPP* self = GST_GOO_SINKPP (sink);
	GstFlowReturn ret = GST_FLOW_OK;
	GstGooSinkPPPrivate* priv = GST_GOO_SINKPP_GET_PRIVATE (self);

	if (goo_port_is_tunneled (self->inport))
	{
		/* shall we send a ghost buffer here ? */
		GST_INFO ("port is tunneled");
		ret = GST_FLOW_OK;
		goto done;
	}

	if (goo_port_is_eos (self->inport))
	{
		GST_INFO ("port is eos");
		ret = GST_FLOW_UNEXPECTED;
		goto done;
	}

	if (self->component->cur_state != OMX_StateExecuting)
	{
		ret = GST_FLOW_UNEXPECTED;
		goto done;
	}

	if (GST_BUFFER_SIZE (buffer) == 0)
	{
		GST_INFO ("zero sized frame");
		ret = GST_FLOW_OK;
		goto done;
	}

	int omxbufsiz;
	omxbufsiz = GOO_PORT_GET_DEFINITION (self->inport)->nBufferSize;

	if (GST_BUFFER_SIZE (buffer) != omxbufsiz)
	{
		GST_ELEMENT_ERROR (self, STREAM, FORMAT,
				   ("Frame is incomplete (%u!=%u)",
				    GST_BUFFER_SIZE (buffer), omxbufsiz),
				   ("Frame is incomplete (%u!=%u)",
				    GST_BUFFER_SIZE (buffer), omxbufsiz));
		ret = GST_FLOW_ERROR;
		goto done;
	}

	OMX_BUFFERHEADERTYPE* omx_buffer;

	if (GST_IS_GOO_BUFFER (buffer) &&
	    goo_port_is_my_buffer (self->inport,
				   GST_GOO_BUFFER (buffer)->omx_buffer))
	{
		omx_buffer = GST_GOO_BUFFER (buffer)->omx_buffer;
	}
	else
	{
		omx_buffer = goo_port_grab_buffer (self->inport);
		guint size = MIN (omxbufsiz, GST_BUFFER_SIZE (buffer));
		GST_DEBUG ("memcpy to buffer %d bytes", omxbufsiz);
		memcpy (omx_buffer->pBuffer, GST_BUFFER_DATA (buffer), size);
		omx_buffer->nFilledLen = size;
	}

	omx_buffer->nTimeStamp =
		GST2OMX_TIMESTAMP (GST_BUFFER_TIMESTAMP (buffer));

	priv->incount++;
	goo_component_release_buffer (self->component, omx_buffer);
	ret = GST_FLOW_OK;

done:
	return ret;
}

static gboolean
gst_goo_sinkpp_event (GstBaseSink *bsink, GstEvent* event)
{
	GST_INFO ("%s", GST_EVENT_TYPE_NAME (event));

	GstGooSinkPP* self = GST_GOO_SINKPP (bsink);

	gboolean ret;

	g_assert (self->component != NULL);

	switch (GST_EVENT_TYPE (event))
	{
	case GST_EVENT_EOS:
		if (goo_component_get_state (self->component) ==
		    OMX_StateExecuting)
		{
			if (!goo_port_is_tunneled (self->inport))
			{
				goo_component_send_eos (self->component);
			}

			goo_component_wait_for_done (self->component);
		}
		ret = TRUE;
		break;
	case GST_EVENT_NEWSEGMENT:
		{
			/* Set config start time to OMX clock*/
			GstFormat fmt;
      		gboolean is_update;
			gint64 start, end, base;
	   		gdouble rate;

      		gst_event_parse_new_segment (event, &is_update, &rate, &fmt, &start,&end, &base);
      		if (fmt == GST_FORMAT_TIME) {
				GST_DEBUG ("Gstreamer start time: %lld\n", start*1000);
				goo_ti_post_processor_set_starttime (self->component, start);
			}
		}
        break;

	case GST_EVENT_TAG:
		ret = TRUE;
		break;
	case GST_EVENT_FLUSH_START:
		ret = TRUE;
		break;
	case GST_EVENT_FLUSH_STOP:
		ret = TRUE;
		break;
	default:
		ret = gst_pad_event_default (GST_BASE_SINK_PAD (bsink), event);
		break;
	}

	return ret;
}

static gboolean
gst_goo_sinkpp_stop (GstBaseSink* bsink)
{
	GstGooSinkPP* self = GST_GOO_SINKPP (bsink);

	g_assert (self->component != NULL);

	if (goo_component_get_state (self->component) == OMX_StateLoaded)
	{
		goto done;
	}

	if (goo_component_get_state (self->component) == OMX_StateExecuting)
	{
		GST_INFO_OBJECT (self, "going to idle");
		GST_OBJECT_LOCK (self);
		goo_component_set_state_idle (self->component);
		GST_OBJECT_UNLOCK (self);
	}

	if (goo_component_get_state (self->component) == OMX_StateIdle)
	{
		GST_INFO_OBJECT (self, "going to loaded");
		GST_OBJECT_LOCK (self);
		goo_component_set_state_loaded (self->component);
		GST_OBJECT_UNLOCK (self);
	}

	done:
	return TRUE;
}

static GstFlowReturn
gst_goo_sinkpp_buffer_alloc (GstBaseSink *bsink, guint64 offset, guint size,
			     GstCaps *caps, GstBuffer **buf)
{
	GstGooSinkPP* self = GST_GOO_SINKPP (bsink);
	GstFlowReturn ret = GST_FLOW_OK;

	GST_DEBUG ("a buffer of %d bytes was requested with caps %"
		   GST_PTR_FORMAT " and offset %llu", size, caps, offset);

	if (self->component->cur_state == OMX_StateLoaded)
	{
		gst_goo_sinkpp_setcaps (bsink, caps);
	}

	OMX_BUFFERHEADERTYPE* omx_buffer;
	omx_buffer = goo_port_grab_buffer (self->inport);
	GstBuffer* gst_buffer = GST_BUFFER (gst_goo_buffer_new ());
	gst_goo_buffer_set_data (gst_buffer, self->component, omx_buffer);
	gst_buffer_set_caps (gst_buffer, caps);

	*buf = gst_buffer;

	return ret;
}

static void
gst_goo_sinkpp_set_property (GObject* object, guint prop_id,
			    const GValue* value, GParamSpec* pspec)
{
	GstGooSinkPPPrivate* priv = GST_GOO_SINKPP_GET_PRIVATE (object);
	GstGooSinkPP* self = GST_GOO_SINKPP (object);
	GObject* cobj = G_OBJECT (self->component);

	switch (prop_id)
	{
	case PROP_NUM_INPUT_BUFFERS:
		priv->num_input_buffers = g_value_get_uint (value);
		break;
	case PROP_OPACITY:
		g_object_set_property (cobj, "opacity", value);
		break;
	case PROP_DISPLAY_SCALE_WIDTH:
		g_object_set_property (cobj, "x-scale", value);
		break;
	case PROP_DISPLAY_SCALE_HEIGHT:
		g_object_set_property (cobj, "y-scale", value);
		break;
	case PROP_DISPLAY_MIRROR:
		g_object_set_property (cobj, "mirror", value);
		break;
	case PROP_DISPLAY_ROTATION:
		g_object_set_property (cobj, "rotation", value);
		break;
	case PROP_DISPLAY_BGD:
		priv->background = g_value_get_uint (value);
		break;
	case PROP_VIDEO_PIPELINE:
		priv->video_pipeline = g_value_get_uint (value);
		break;
	case PROP_DISPLAY_POS_X:
		g_object_set_property (cobj, "x-pos", value);
		break;
	case PROP_DISPLAY_POS_Y:
		g_object_set_property (cobj, "y-pos", value);
		break;
	case PROP_OUTPUT_DEVICE:
		g_object_set_property (cobj, "out-device", value);
		break;
	case PROP_DISPLAY_CROP_LEFT:
		g_object_set_property (cobj, "crop-left", value);
		break;
	case PROP_DISPLAY_CROP_TOP:
		g_object_set_property (cobj, "crop-top", value);
		break;
	case PROP_DISPLAY_CROP_WIDTH:
		g_object_set_property (cobj, "crop-width", value);
		break;
	case PROP_DISPLAY_CROP_HEIGHT:
		g_object_set_property (cobj, "crop-height", value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_sinkpp_get_property (GObject* object, guint prop_id,
			    GValue* value, GParamSpec* pspec)
{
	GstGooSinkPP* self = GST_GOO_SINKPP (object);
	GstGooSinkPPPrivate* priv = GST_GOO_SINKPP_GET_PRIVATE (object);
	GObject* cobj = G_OBJECT (self->component);

	switch (prop_id)
	{
	case PROP_NUM_INPUT_BUFFERS:
		g_value_set_uint (value, priv->num_input_buffers);
		break;
        case PROP_OPACITY:
		g_object_get_property (cobj, "opacity", value);
		break;
	case PROP_DISPLAY_SCALE_WIDTH:
		g_object_get_property (cobj, "x-scale", value);
		break;
	case PROP_DISPLAY_SCALE_HEIGHT:
		g_object_get_property (cobj, "y-scale", value);
		break;
	case PROP_DISPLAY_MIRROR:
		g_object_get_property (cobj, "mirror", value);
		break;
	case PROP_DISPLAY_ROTATION:
		g_object_get_property (cobj, "rotation", value);
		break;
	case PROP_DISPLAY_BGD:
		g_value_set_uint (value, priv->background);
		break;
	case PROP_VIDEO_PIPELINE:
		g_value_set_uint (value, priv->video_pipeline);
		break;
	case PROP_DISPLAY_POS_X:
		g_object_get_property (cobj, "x-pos", value);
		break;
	case PROP_DISPLAY_POS_Y:
		g_object_get_property (cobj, "y-pos", value);
		break;
	case PROP_OUTPUT_DEVICE:
		g_object_get_property (cobj, "out-device", value);
		break;
	case PROP_DISPLAY_CROP_LEFT:
		g_object_get_property (cobj, "crop-left", value);
		break;
	case PROP_DISPLAY_CROP_TOP:
		g_object_get_property (cobj, "crop-top", value);
		break;
	case PROP_DISPLAY_CROP_WIDTH:
		g_object_get_property (cobj, "crop-width", value);
		break;
	case PROP_DISPLAY_CROP_HEIGHT:
		g_object_get_property (cobj, "crop-height", value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_sinkpp_dispose (GObject* object)
{
	GST_DEBUG ("");

	GstGooSinkPP* me = GST_GOO_SINKPP (object);

	G_OBJECT_CLASS (parent_class)->dispose (object);

	if (G_LIKELY (me->inport))
	{
		GST_DEBUG ("unrefing inport");
		g_object_unref (me->inport);
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

}

static void
gst_goo_sinkpp_base_init (gpointer g_klass)
{
	GST_DEBUG_CATEGORY_INIT (gst_goo_sinkpp_debug, "gooppsink", 0,
				 "TI Posst Processor sink element");

	GstElementClass* e_klass = GST_ELEMENT_CLASS (g_klass);

	gst_element_class_add_pad_template (e_klass,
					    gst_static_pad_template_get
					    (&sink_factory));

	gst_element_class_set_details (e_klass, &details);

	return;
}

static void
gst_goo_sinkpp_class_init (GstGooSinkPPClass* klass)
{
	GObjectClass* g_klass;
	GParamSpec* pspec;
	GstBaseSinkClass* gst_klass;

	/* gobject */
	g_klass = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GstGooSinkPPPrivate));

	g_klass->set_property =
		GST_DEBUG_FUNCPTR (gst_goo_sinkpp_set_property);
	g_klass->get_property =
		GST_DEBUG_FUNCPTR (gst_goo_sinkpp_get_property);
	g_klass->dispose = GST_DEBUG_FUNCPTR (gst_goo_sinkpp_dispose);

	pspec = g_param_spec_uint ("input-buffers", "Input buffers",
				   "The number of OMX input buffers",
				   1, 4, DEFAULT_INPUT_BUFFERS,
				   G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_NUM_INPUT_BUFFERS,
					 pspec);

	pspec = g_param_spec_uint ("opacity", "opacity",
				  "Modify the opacity",
				  0,255, DEFAULT_OPACITY,
				  G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_OPACITY, pspec);


	pspec = g_param_spec_uint ("x-scale", "Width scale",
				   "Image width scale",
				   25, 800, DEFAULT_XSCALE, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_DISPLAY_SCALE_WIDTH,
					 pspec);

	pspec = g_param_spec_uint ("y-scale", "Height scale",
				   "Image height scale",
				   25, 800, DEFAULT_YSCALE, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_DISPLAY_SCALE_HEIGHT,
					 pspec);

	pspec = g_param_spec_enum ("rotation", "Rotation",
				   "Image rotation",
				   GST_GOO_SINKPP_ROTATION,
				   DEFAULT_ROTATION, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_DISPLAY_ROTATION,
					 pspec);

	pspec = g_param_spec_uint ("background", "Background color",
				   "Background color",
				   0x000000, 0xffffff, DEFAULT_BGD,
				   G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_DISPLAY_BGD, pspec);

	pspec = g_param_spec_uint ("videolayer", "VideoPipeline",
				   "Video pipeline/layer",
				   1, 2, DEFAULT_LAYER,
				   G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_VIDEO_PIPELINE, pspec);

	pspec = g_param_spec_boolean ("mirror", "Mirror efect",
				      "Mirror efect", FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_DISPLAY_MIRROR, pspec);

	pspec = g_param_spec_uint ("x-pos", "X position",
				   "The position in X axis",
				   0, G_MAXUINT, DEFAULT_XPOS,
				   G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_DISPLAY_POS_X, pspec);

	pspec = g_param_spec_uint ("y-pos", "Y position",
				   "The position in Y axis",
				   0, G_MAXUINT, DEFAULT_YPOS,
				   G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_DISPLAY_POS_Y, pspec);

	pspec = g_param_spec_enum ("outdev", "Output device",
				   "The output device",
				   GST_GOO_SINKPP_OUTPUT,
				   DEFAULT_OUTPUT, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_OUTPUT_DEVICE, pspec);



	pspec = g_param_spec_uint ("crop-left", "Left cropping",
				   "The number of pixels to crop from left",
				   0, G_MAXUINT, 0,
				   G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_DISPLAY_CROP_LEFT, pspec);

	pspec = g_param_spec_uint ("crop-top", "Top cropping",
				   "The number of pixels to crop from top",
				   0, G_MAXUINT, 0,
				   G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_DISPLAY_CROP_TOP, pspec);

	pspec = g_param_spec_uint ("crop-width", "Cropped width",
				   "The width of cropped display",
				   0, G_MAXUINT, 0,
				   G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_DISPLAY_CROP_WIDTH, pspec);

	pspec = g_param_spec_uint ("crop-height", "Cropped height",
				   "The height of cropped display",
				   0, G_MAXUINT, 0,
				   G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_DISPLAY_CROP_HEIGHT, pspec);




	/* GST */
	gst_klass = GST_BASE_SINK_CLASS (klass);

	gst_klass->preroll	= GST_DEBUG_FUNCPTR (gst_goo_sinkpp_render);
	gst_klass->render	= GST_DEBUG_FUNCPTR (gst_goo_sinkpp_render);
	gst_klass->set_caps	= GST_DEBUG_FUNCPTR (gst_goo_sinkpp_setcaps);
	gst_klass->stop		= GST_DEBUG_FUNCPTR (gst_goo_sinkpp_stop);
	gst_klass->event	= GST_DEBUG_FUNCPTR (gst_goo_sinkpp_event);
	gst_klass->buffer_alloc =
		GST_DEBUG_FUNCPTR (gst_goo_sinkpp_buffer_alloc);

	return;
}

static void
gst_goo_sinkpp_init (GstGooSinkPP* self, GstGooSinkPPClass* klass)
{
	GST_DEBUG ("");

	GstGooSinkPPPrivate* priv = GST_GOO_SINKPP_GET_PRIVATE (self);
	priv->num_input_buffers = DEFAULT_INPUT_BUFFERS;
	priv->incount		= 0;
	priv->background	= DEFAULT_BGD;
	priv->framerate		= DEFAULT_FRAMERATE;
	priv->color		= DEFAULT_COLOR_FORMAT;
	priv->video_pipeline	= DEFAULT_LAYER;

	self->factory = goo_ti_component_factory_get_instance ();
	self->component =
		goo_component_factory_get_component(self->factory,
						    GOO_TI_POST_PROCESSOR);

	GooComponent* component = self->component;
	self->inport = goo_component_get_port (component, "input0");
	g_assert (self->inport != NULL);

	g_object_set_data (G_OBJECT (self->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", self->component);

	return;
}
