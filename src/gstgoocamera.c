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

#include <gst/interfaces/colorbalance.h>
#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>

#include <goo-ti-camera.h>

#include "gstgoocamera.h"
#include "gstghostbuffer.h"
#include "gstgoobuffer.h"
#include "gstgooutils.h"
#include "string.h"

GST_DEBUG_CATEGORY_STATIC (gst_goo_camera_debug);
#define GST_CAT_DEFAULT gst_goo_camera_debug

/* signals */
enum
{
	LAST_SIGNAL
};
enum
{
    VIDEO_MODE                  = 0,
    HIGH_PERFORMANCE_MODE       = 1,
    HIGH_QUALITY_MODE           = 2,
};

/* args */
enum
{
	PROP_0,
	PROP_NUM_OUTPUT_BUFFERS,
	PROP_PREVIEW,
	PROP_DISPLAY_WIDTH,
	PROP_DISPLAY_HEIGHT,
	PROP_DISPLAY_POS_X,
	PROP_DISPLAY_POS_Y,
	PROP_DISPLAY_ROTATION,
	PROP_DISPLAY_VIDEOPIPELINE,
	PROP_OUTPUT_DEVICE,
	PROP_CONTRAST,
	PROP_BRIGHTNESS,
	PROP_ZOOM,
	PROP_BALANCE,
	PROP_EXPOSURE,
	PROP_VSTAB,
	PROP_FOCUS,
	PROP_EFFECTS,
	PROP_IPP,
	PROP_CAPMODE
};

#define GST_GOO_CAMERA_GET_PRIVATE(obj) (GST_GOO_CAMERA (obj)->priv)

struct _GstGooCameraPrivate
{
	gboolean preview;
	guint video_pipeline;
	guint outcount;
	guint display_height;
	guint display_width;
	guint capture;
	gint fps_n, fps_d;
	gboolean vstab;
	gint zoom;
	gint focus;
	gint effects;
	gint exposure;
	gboolean ipp;
	guint capmode;
	guint output_buffers;
	gint64 last_pause_time;
};

static const GstElementDetails details =
	GST_ELEMENT_DETAILS (
		"OpenMAX camera source",
		"Source/Video",
		"Capture raw frames through the OpenMAX camera component",
		"Texas Instrument"
		);

static GstStaticPadTemplate src_factory =
	GST_STATIC_PAD_TEMPLATE (
		"src",
		GST_PAD_SRC,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS (GST_VIDEO_CAPS_YUV ("{ YUY2, UYVY  }") ";"
				 GST_VIDEO_CAPS_RGB_16)
		);

/* the max resolution of the postprocessor */
ResolutionInfo maxres;

#define NUM_OUTPUT_BUFFERS_DEFAULT 6
#define PREVIEW_DEFAULT		       TRUE
#define OUTPUT_DEFAULT	      	   GOO_TI_POST_PROCESSOR_OUTPUT_LCD
#define VIDEOPIPELINE_DEFAULT      2
#define DISPLAY_WIDTH_DEFAULT      320
#define DISPLAY_HEIGHT_DEFAULT     240
#define DISPLAY_POSX_DEFAULT       0
#define DISPLAY_POSY_DEFAULT       0
#define COLOR_DEFAULT              OMX_COLOR_FormatYCbYCr
#define BALANCE_DEFAULT            OMX_WhiteBalControlAuto
#define EXPOSURE_DEFAULT   		   OMX_ExposureControlAuto
#define ZOOM_DEFAULT               GOO_TI_CAMERA_ZOOM_1X
#define CONTRAST_DEFAULT	       0
#define BRIGHTNESS_DEFAULT	       50
#define DISPLAY_ROTATION_DEFAULT   GOO_TI_POST_PROCESSOR_ROTATION_NONE
#define VSTAB_DEFAULT              FALSE
#define FOCUS_DEFAULT              OMX_CameraConfigFocusStopFocus
#define EFFECTS_DEFAULT			   OMX_CameraConfigEffectsNormal
#define CONTRAST_LABEL		   "Contrast"
#define BRIGHTHNESS_LABEL	   "Brightness"
#define IPP_DEFAULT		   FALSE
#define CAPMODE_DEFAULT            VIDEO_MODE
#define DEFAULT_START_TIME 0

/* use the base clock to timestamp the frame */
#define BASECLOCK
/* #undef BASECLOCK */

/* use gst_pad_alloc_buffer */
/* #define PADALLOC */
#undef PADALLOC

#define GOO_IS_TI_ANY_VIDEO_ENCODER(obj) \
	(GOO_IS_TI_VIDEO_ENCODER(obj) || GOO_IS_TI_VIDEO_ENCODER720P(obj))

#define GST_GOO_CAMERA_CAPTURE_MODE (gst_goo_camera_capmode_get_type ())
static GType
gst_goo_camera_capmode_get_type (void)
{
    static GType type = 0;

    if (!type)
    {
        static GEnumValue vals[] =
        {
            {VIDEO_MODE,            "Video",  "Video capture mode"},
            {HIGH_PERFORMANCE_MODE, "HP",     "High performance image capture"},
            {HIGH_QUALITY_MODE,     "HQ",     "High quality image capture"},
            {0, NULL, NULL},
        };

        type = g_enum_register_static ("GstGooCameraCaptureMode", vals);
    }

    return type;
}


#define GST_GOO_CAMERA_ROTATION (gst_goo_camera_rotation_get_type ())

static GType
gst_goo_camera_rotation_get_type ()
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
	{
		const static GEnumValue rotation[] = {
			{ GOO_TI_POST_PROCESSOR_ROTATION_NONE,
			  "0",	 "None Rotation" },
			{ GOO_TI_POST_PROCESSOR_ROTATION_90,
			  "90",	 "90 degree rotation" },
			{ GOO_TI_POST_PROCESSOR_ROTATION_180,
			  "180", "180 degrees rotation" },
			{ GOO_TI_POST_PROCESSOR_ROTATION_270,
			  "270", "270 degrees rotation" },
			{ 0, NULL, NULL },
		};

		type = g_enum_register_static ("GstGooCameraRotation",
					       rotation);
	}

	return type;
}

#define GST_GOO_CAMERA_OUTPUT (goo_ti_post_processor_output_get_type ())

GType
gst_goo_camera_output_get_type ()
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
	{
		static GEnumValue outdev[] = {
			{ GOO_TI_POST_PROCESSOR_OUTPUT_LCD,
			  "LCD", "LCD display output" },
			{ GOO_TI_POST_PROCESSOR_OUTPUT_TV,
			  "TV", "TV display output" },
			{ GOO_TI_POST_PROCESSOR_OUTPUT_HDMI,
			  "HDMI", "HDMI output" },
			{ GOO_TI_POST_PROCESSOR_OUTPUT_LCD_TV,
			  "TV & LCD", "Simultaneous display output" },
			{ 0, NULL, NULL }
		};

		type = g_enum_register_static
			("GstGooCameraOutput", outdev);
	}

	return type;
}

static gboolean
gst_goo_camera_interface_supported (GstImplementsInterface* iface,
				    GType iface_type)
{
	g_return_val_if_fail (iface_type == GST_TYPE_COLOR_BALANCE, FALSE);

	return TRUE;
}

static void
gst_goo_camera_implements_interface_init (gpointer g_iface,
					  gpointer iface_data)
{
	GstImplementsInterfaceClass* iface =
		(GstImplementsInterfaceClass*) g_iface;

	iface->supported =
		GST_DEBUG_FUNCPTR (gst_goo_camera_interface_supported);

	return;
}

static const GList*
gst_goo_camera_color_balance_list_channels (GstColorBalance* balance)
{
	GstGooCamera* self = GST_GOO_CAMERA (balance);

	return (const GList *) self->channels;
}

static void
gst_goo_camera_color_balance_set_value (GstColorBalance* balance,
					GstColorBalanceChannel* channel,
					gint value)
{
	GstGooCamera* self = GST_GOO_CAMERA (balance);
	GstGooCameraPrivate* priv = GST_GOO_CAMERA_GET_PRIVATE (self);
	g_return_if_fail (GOO_IS_TI_CAMERA (self->camera));

	if (g_ascii_strcasecmp (channel->label, CONTRAST_LABEL) == 0)
	{
		GST_DEBUG ("contrast = %d", value);
		g_object_set (self->camera, "contrast", value, NULL);
	}
	else if (g_ascii_strcasecmp (channel->label, BRIGHTHNESS_LABEL) == 0)
	{
		GST_DEBUG ("brightness = %d", value);
		g_object_set (self->camera, "brightness", value, NULL);
	}

	return;
}

static gint
gst_goo_camera_color_balance_get_value (GstColorBalance* balance,
					GstColorBalanceChannel* channel)
{
	GstGooCamera* self = GST_GOO_CAMERA (balance);
	GstGooCameraPrivate* priv = GST_GOO_CAMERA_GET_PRIVATE (self);
	g_return_if_fail (GOO_IS_TI_CAMERA (self->camera));

	if (g_ascii_strcasecmp (channel->label, CONTRAST_LABEL) == 0)
	{
		gint contrast;
		g_object_get (self->camera, "contrast", &contrast, NULL);
		return contrast;
	}
	else if (g_ascii_strcasecmp (channel->label, BRIGHTHNESS_LABEL) == 0)
	{
		gint brightness;
		g_object_get (self->camera, "brightness", &brightness, NULL);
	}

	return 0;
}

static void
gst_goo_camera_colorbalance_interface_init (gpointer g_iface,
					    gpointer iface_data)
{
	GstColorBalanceClass* klass = (GstColorBalanceClass*) g_iface;

	GST_COLOR_BALANCE_TYPE (klass) = GST_COLOR_BALANCE_HARDWARE;

	klass->list_channels =
		GST_DEBUG_FUNCPTR (gst_goo_camera_color_balance_list_channels);
	klass->set_value =
		GST_DEBUG_FUNCPTR (gst_goo_camera_color_balance_set_value);
	klass->get_value =
		GST_DEBUG_FUNCPTR (gst_goo_camera_color_balance_get_value);

	return;
}

void
_do_init (GType camera_type)
{
	static const GInterfaceInfo implements_interface_info =
		{
			gst_goo_camera_implements_interface_init,
			NULL,
			NULL
		};

	static const GInterfaceInfo colorbalance_interface_info =
		{
			gst_goo_camera_colorbalance_interface_init,
			NULL,
			NULL
		};

	g_type_add_interface_static (camera_type,
				     GST_TYPE_IMPLEMENTS_INTERFACE,
				     &implements_interface_info);

	g_type_add_interface_static (camera_type,
				     GST_TYPE_COLOR_BALANCE,
				     &colorbalance_interface_info);

	return;
}

GST_BOILERPLATE_FULL (GstGooCamera, gst_goo_camera,
		      GstPushSrc, GST_TYPE_PUSH_SRC, _do_init);


static gboolean
gst_goo_camera_start (GstBaseSrc* self)
{
	GstGooCamera* me = GST_GOO_CAMERA (self);
	GstGooCameraPrivate* priv = GST_GOO_CAMERA_GET_PRIVATE (self);

	priv->outcount = 0;

	return TRUE;
}


static gboolean
gst_goo_camera_stop (GstBaseSrc* self)
{
	GstGooCamera* me = GST_GOO_CAMERA (self);
	GstGooCameraPrivate* priv = GST_GOO_CAMERA_GET_PRIVATE (self);

	/* this function could get called twice (see gst_goo_camera_src_event())
	 * so make sure we haven't already moved me->camera back to loaded state:
	 */
	if( goo_component_get_state (me->camera) == OMX_StateLoaded )
	{
		return TRUE;
	}

	GST_DEBUG_OBJECT (self, "");

	if (priv->capture != CAPMODE_DEFAULT)
	{
		GST_INFO_OBJECT (self, "Stop");
		g_object_set (me->camera, "capture", CAPMODE_DEFAULT, NULL);
		priv->capture != CAPMODE_DEFAULT;
	}

	GST_INFO_OBJECT (self, "going to idle");
	GST_OBJECT_LOCK (self);
	goo_component_set_state_idle (me->camera);
	GST_OBJECT_UNLOCK (self);

	if (me->clock)
	{
		GST_INFO_OBJECT (self, "moving clock to to idle");
		goo_component_set_state_idle (me->clock);
	}

	GST_INFO_OBJECT (self, "going to loaded");
	GST_OBJECT_LOCK (self);
	goo_component_set_state_loaded (me->camera);
	GST_OBJECT_UNLOCK (self);


	if (me->clock)
	{
		goo_component_set_state_loaded (me->clock);
	}

	return TRUE;
}


static gboolean
gst_goo_camera_src_event (GstPad *pad, GstEvent *event)
{
	GST_INFO ("camera %s", GST_EVENT_TYPE_NAME (event));

	GstGooCamera* self = GST_GOO_CAMERA (gst_pad_get_parent (pad));

	gboolean ret;

	switch (GST_EVENT_TYPE (event))
	{
		case GST_EVENT_CUSTOM_UPSTREAM:
			if (gst_goo_event_is_reverse_eos (event) && goo_port_is_tunneled (self->captureport))
			{
				/* in case of a tunnel, we never find out about the EOS.. so
				 * the custom-upstream event from the videoencoder is a hack
				 * to ensure OMX camera gets stopped..
				 */
				gst_goo_camera_stop (GST_BASE_SRC (self));
			}
			ret = TRUE;
			break;
		default:
			ret = gst_pad_event_default (pad, event);
			break;
	}

	gst_object_unref (self);
	return ret;
}

static void
gst_goo_camera_cb_PPM_omx_events (GooTiCamera* component, GooTiCameraOmxDataEvent* OmxEventData, GstGooCamera* self)
{
	GstMessage *msg = NULL;
	GstStructure *stru = NULL;
	gboolean ret = TRUE;
	gchar* structure_name = "PPM_OMX_event";

	stru = gst_structure_new (structure_name,
	                          "eEvent", G_TYPE_UINT, OmxEventData->eEvent,
	                          "nData1", G_TYPE_ULONG, OmxEventData->nData1,
	                          "nData2", G_TYPE_ULONG, OmxEventData->nData2,
	                          NULL);

	/* Create the message and send it to the bus */
	msg = gst_message_new_custom (GST_MESSAGE_ELEMENT, GST_OBJECT (self), stru);

	ret = gst_element_post_message (GST_ELEMENT (self), msg);
	if (ret != TRUE)
		GST_WARNING_OBJECT (self, "Messsage could not be posted: PPM_OMX_event");

	return;
}

static void
gst_goo_camera_cb_PPM_focus_start (GooTiCamera* component, GTimeVal* timestamp, GstGooCamera* self)
{
	gst_goo_util_post_message ( GST_ELEMENT (self), "focus-startpoint", timestamp);
}

static void
gst_goo_camera_cb_PPM_focus_end (GooTiCamera* component, GTimeVal* timestamp, GstGooCamera* self)
{
	gst_goo_util_post_message ( GST_ELEMENT (self), "focus-endpoint", timestamp);
}

/* this function is a bit of a last resort */
static void
gst_goo_camera_fixate (GstBaseSrc* self, GstCaps* caps)
{
	GstStructure *structure;
	gint i;
	GST_DEBUG_OBJECT (self, "fixating caps %" GST_PTR_FORMAT, caps);
	ResolutionInfo defres = goo_get_resolution ("cif");

	for (i = 0; i < gst_caps_get_size (caps); ++i)
	{
		structure = gst_caps_get_structure (caps, i);
		const GValue *v;

		/* FIXME such sizes? we usually fixate to something in the
		 * cif range... */

		gst_structure_fixate_field_nearest_int (structure, "width",
							defres.width);
		gst_structure_fixate_field_nearest_int (structure, "height",
							defres.height);
		gst_structure_fixate_field_nearest_fraction (structure,
							     "framerate",
							     15, 1);

		v = gst_structure_get_value (structure, "format");

		if (v && G_VALUE_TYPE (v) != GST_TYPE_FOURCC)
		{
			guint32 fourcc;

			g_return_if_fail (G_VALUE_TYPE (v) == GST_TYPE_LIST);

			fourcc = gst_value_get_fourcc
				(gst_value_list_get_value (v, 0));
			gst_structure_set (structure, "format",
					   GST_TYPE_FOURCC, fourcc, NULL);
		}
	}

}

static OMX_COLOR_FORMATTYPE
gst_goo_camera_color_from_structure (GstStructure* structure)
{
	OMX_COLOR_FORMATTYPE color = OMX_COLOR_FormatUnused;

	const gchar* mimetype = gst_structure_get_name (structure);

	if (g_strrstr (mimetype, "video/x-raw-yuv"))
	{
		guint32 fourcc;
		if (gst_structure_get_fourcc (structure, "format", &fourcc))
		{
			switch (fourcc)
			{
			case GST_MAKE_FOURCC ('Y', 'U', 'Y', '2'):
				color = OMX_COLOR_FormatYCbYCr;
				break;
			case GST_MAKE_FOURCC ('U', 'Y', 'V', 'Y'):
				color = OMX_COLOR_FormatCbYCrY;
				break;
			case GST_MAKE_FOURCC ('I', '4', '2', '0'):
				color = OMX_COLOR_FormatYUV420PackedPlanar;
				break;
			}
		}
	}
	else if (g_strrstr (mimetype, "video/x-raw-rgb"))
	{
		gint depth;

		gst_structure_get_int (structure, "depth", &depth);

		switch (depth)
		{
		case 16:
			color = OMX_COLOR_Format16bitRGB565;
			break;
		}
	}

	GST_INFO ("Extracted color: %s", goo_strcolor (color));
	return color;
}

static void
gst_goo_postproc_config (GstGooCamera* self, gint width, gint height, guint32 color)
{
	GstGooCameraPrivate* priv = GST_GOO_CAMERA_GET_PRIVATE (self);

	if (priv->capmode == VIDEO_MODE)
	{
		priv->display_width  = width;
		priv->display_height = height;
	}
	else if (priv->display_width == 0 && priv->display_height == 0)
	{
		priv->display_width = MIN (maxres.width, width);
		priv->display_height = MIN (maxres.height, height);
	}
	else
	{
		if ((priv->display_width > maxres.width) | (priv->display_height > maxres.height))
		{
			priv->display_height = DISPLAY_HEIGHT_DEFAULT;
			priv->display_width = DISPLAY_WIDTH_DEFAULT;
		}
	}

	GST_INFO_OBJECT (self, " preview dwidth = %d | preview dheight = %d",
						 priv->display_width,
						 priv->display_height);

	/* postroc input port */
	{
		GooTiPostProcessor* pp;
		pp = GOO_TI_POST_PROCESSOR (self->postproc);
		pp->video_pipeline = priv->video_pipeline;

		GooPort* port =	goo_component_get_port (self->postproc,
					"input0");
		g_assert (port != NULL);

		OMX_PARAM_PORTDEFINITIONTYPE* param;
		param = GOO_PORT_GET_DEFINITION (port);

		param->format.video.nFrameWidth = priv->display_width;
		param->format.video.nFrameHeight =priv->display_height;
		param->format.video.eColorFormat = color;
		param->nBufferCountActual = priv->output_buffers;

		g_object_unref (port);
	}
	/* postproc properties */
	{
		g_object_set (G_OBJECT (self->postproc),
		      "x-scale", 100,
		      "y-scale", 100,
		      "mirror", FALSE,
		      NULL);
	}
}

static void
gst_goo_jpegenc_config (GooComponent *component, GstGooCamera* self, gint width, gint height,
	     guint32 color, guint fps_n, guint fps_d)
{
	/* input port */
	{
		GooPort *peer_port = goo_component_get_port (component, "input0");
		g_assert (peer_port != NULL);
		OMX_PARAM_PORTDEFINITIONTYPE* param = GOO_PORT_GET_DEFINITION (peer_port);

		param->format.image.nFrameWidth = width;
		param->format.image.nFrameHeight = height;
		param->format.image.eColorFormat = color;
		g_object_unref (peer_port);
	}
	/* output port */
	{
		GooPort* port = goo_component_get_port (component, "output0");
		g_assert (port != NULL);
		OMX_PARAM_PORTDEFINITIONTYPE* param_out = GOO_PORT_GET_DEFINITION (port);

		param_out->format.image.nFrameWidth = width;
		param_out->format.image.nFrameHeight = height;
		param_out->format.image.eColorFormat = color;

		GST_INFO_OBJECT (self, "setting up tunnel with jpeg encoder");
		goo_component_set_tunnel_by_name (self->camera, "output1",
				component, "input0", OMX_BufferSupplyInput);
		g_object_unref (port);
	}
}

static void
gst_goo_videoenc_config (GooComponent *component, GstGooCamera* self, gint width, gint height,
	     guint32 color, guint fps_n, guint fps_d)
{
	GstGooCameraPrivate* priv = GST_GOO_CAMERA_GET_PRIVATE (self);
	GST_INFO_OBJECT (self, "There is a video encoder");
	/* input port */
	{
		GooPort *peer_port = goo_component_get_port (component, "input0");
		g_assert (peer_port != NULL);
		OMX_PARAM_PORTDEFINITIONTYPE* param = GOO_PORT_GET_DEFINITION (peer_port);

		param->format.video.xFramerate =   (fps_n / fps_d) << 16;
		param->format.video.nFrameWidth =  width;
		param->format.video.nFrameHeight = height;
		param->format.video.eColorFormat = color;
		param->nBufferCountActual =        priv->output_buffers;
		g_object_unref (peer_port);
	}
	/* output port */
	{
		GooPort* port = goo_component_get_port (component, "output0");
		g_assert (port != NULL);
		OMX_PARAM_PORTDEFINITIONTYPE* param_out = GOO_PORT_GET_DEFINITION (port);

		param_out->format.video.nFrameWidth =   width;
		param_out->format.video.nFrameHeight =	height;
		param_out->format.video.xFramerate =    (fps_n / fps_d) << 16;

		GST_INFO_OBJECT (self, "setting up tunnel with video encoder");
		goo_component_set_tunnel_by_name (self->camera, "output1",
					component, "input0", OMX_BufferSupplyInput);
		g_object_unref (port);
	}
}

static void
gst_goo_camera_sync (GstGooCamera* self, gint width, gint height,
		     guint32 color, guint fps_n, guint fps_d)
{
	GstGooCameraPrivate* priv = GST_GOO_CAMERA_GET_PRIVATE (self);
	GValue* value;
	GooComponent *component=NULL;
	GST_DEBUG_OBJECT (self, "");

	/* sensor configuration */
	{
		OMX_PARAM_SENSORMODETYPE* sensor = GOO_TI_CAMERA_GET_PARAM (self->camera);
		gint num_buff;
		g_object_get (self, "num-buffers", &num_buff, NULL);
		/** If only one image will be captured **/
		if ((num_buff == 1) && (priv->capmode != VIDEO_MODE))
		{
			sensor->bOneShot = TRUE;
			sensor->nFrameRate = 0;
		}
		else
		{
			/**More than one image will be captured**/
			sensor->bOneShot = FALSE;
			if (fps_d != 0)
				sensor->nFrameRate = fps_n / fps_d;
			else
				sensor->nFrameRate = 15;
		}
		/* Info needed at goo-ti for burst mode configuration */
		if (num_buff != G_MAXUINT)
			g_object_set (self->camera, "shots", num_buff, NULL);
		GST_INFO_OBJECT (self, "Oneshot mode = %d", sensor->bOneShot);

	}

	if (priv->preview == TRUE)
	{
		/* display configuration */
		gst_goo_postproc_config (self, width, height, color);

		/* camera viewfinding port */
		{
			GooPort* port =
				goo_component_get_port (self->camera,
							"output0");
			g_assert (port != NULL);

			OMX_PARAM_PORTDEFINITIONTYPE* param;
			param = GOO_PORT_GET_DEFINITION (port);

			param->format.video.nFrameWidth =   priv->display_width;
			param->format.video.nFrameHeight =	priv->display_height;
			param->format.video.eColorFormat = color;
			param->nBufferCountActual = priv->output_buffers;

			g_object_unref (port);
		}
	}

	else
	{
		GooPort* port =
			goo_component_get_port (self->camera, "output0");
		g_assert (port != NULL);

		GST_INFO_OBJECT (self, "disabling viewfinding port");
		goo_component_disable_port (self->camera, port);

		g_object_unref (port);
	}


	/* thumbnail port configuration */
#if 0	/* we can disable thumbnail port at the moment */
	{
		GooPort* port = goo_component_get_port (self->camera,
							"output2");
		g_assert (port != NULL);

		OMX_PARAM_PORTDEFINITIONTYPE *param = NULL;
		param = GOO_PORT_GET_DEFINITION (port);

		gint tn_width = MIN (maxres.width, width);
		gint tn_height = MIN (maxres.height, height);

		param->format.video.nFrameWidth  = tn_width;
		param->format.video.nFrameHeight = tn_height;
		param->format.video.eColorFormat = color;
		param->eDomain = OMX_PortDomainVideo;

		g_object_unref (port);
	}
#else
	{
		GooPort* port =
			goo_component_get_port (self->camera, "output2");
		g_assert (port != NULL);

		GST_INFO_OBJECT (self, "disabling thumbnail port");
		goo_component_disable_port (self->camera, port);

		g_object_unref (port);

	}
#endif
		/* capture port configuration */
	{
		OMX_PARAM_PORTDEFINITIONTYPE* param;
		param = GOO_PORT_GET_DEFINITION (self->captureport);

		/**TRUE if some images will be captured**/
		if (priv->capmode != VIDEO_MODE)
		{
			param->format.image.eColorFormat = color;
			param->eDomain = OMX_PortDomainImage;
			param->format.image.nFrameWidth = width;
			param->format.image.nFrameHeight = height;
		}
		else
		{
			param->format.video.eColorFormat = color;
			param->eDomain = OMX_PortDomainVideo;
			param->format.video.nFrameWidth = width;
			param->format.video.nFrameHeight = height;
		}
		param->nBufferCountActual = priv->output_buffers;
	}

	/* looking for a goo component in the capture port */
	{
		GstPad *peer, *src_peer;
		GstElement *next_element=NULL;

		if (GST_BASE_SRC_PAD (self))
		{
			peer = gst_pad_get_peer (GST_BASE_SRC_PAD (self));
			if (peer != NULL)
			{
				next_element = GST_ELEMENT (gst_pad_get_parent (peer));
				if (G_UNLIKELY (next_element != NULL))
				{
					if (!(g_object_get_data (G_OBJECT (next_element), "goo")) && GST_IS_BASE_TRANSFORM (next_element))
					{
						GST_DEBUG_OBJECT(self, "next element name: %s", gst_element_get_name (next_element));
						src_peer = gst_element_get_pad (next_element,"src");
						if ( src_peer != NULL )
						{
							gst_object_unref (peer);
							peer = gst_pad_get_peer (src_peer);
							gst_object_unref (next_element);
							next_element = GST_ELEMENT(gst_pad_get_parent (peer)) ;
							GST_DEBUG_OBJECT (self, "one after element name: %s", gst_element_get_name(next_element));
							gst_object_unref (src_peer);
							component = GOO_COMPONENT (g_object_get_data (G_OBJECT (next_element), "goo"));
						}
					}
					else
					{
						component = GOO_COMPONENT (g_object_get_data (G_OBJECT (next_element), "goo"));
					}

					if (GOO_IS_TI_ANY_VIDEO_ENCODER (component))
					{
						/* video encoder configuration and set tunnel */
						gst_goo_videoenc_config (component, self, width, height,
							     color, fps_n, fps_d);
					}
					else if ( GOO_IS_TI_JPEGENC (component))
					{
						/* jpeg encoder configuration and set tunnel */
						gst_goo_jpegenc_config (component, self, width, height,
							     color, fps_n, fps_d);
					}
					gst_object_unref (next_element);
				}
				gst_object_unref (peer);
			}
		} /* end of capture port configuration */
	}

no_enc:
	if (priv->ipp == TRUE)
	{
		g_object_set (self->camera, "ipp", priv->ipp, NULL);
	}

	GST_INFO_OBJECT (self, "setting up tunnel with post processor");
	goo_component_set_tunnel_by_name (self->camera, "output0",
				  self->postproc, "input0",
				  OMX_BufferSupplyInput);
	GooPort* port_pp =	goo_component_get_port (self->postproc,
				"input0");
	g_assert (port_pp != NULL);
	goo_component_set_supplier_port (self->postproc, port_pp, OMX_BufferSupplyInput);
	gst_object_unref (port_pp);

	if (component != NULL)
	{
		g_object_unref (component);
	}
	return;
}

static void
gst_goo_camera_omx_start (GstGooCamera* self, gint width, gint height,
		     guint32 color, guint fps_n, guint fps_d)
{
	g_assert (self != NULL);
	GstGooCameraPrivate* priv = GST_GOO_CAMERA_GET_PRIVATE (self);

	GST_OBJECT_LOCK (self);

	if (goo_component_get_state (self->camera) == OMX_StateLoaded)
	{
		/* Camera ports configuration and tunneled components */
		gst_goo_camera_sync (self, width, height, color, fps_n, fps_d);

		if (priv->capmode == VIDEO_MODE )
		{	/* In video mode, Camera should use the omx clock for correct timestamping */
			GST_INFO ("Create the OMX Clock");
			self->clock = goo_component_factory_get_component (self->factory, GOO_TI_CLOCK);
			GST_INFO ("Camera and OMX linkage");
			goo_ti_camera_set_clock (self->camera, self->clock);
		}

		GST_INFO_OBJECT (self, "going to idle");
		goo_component_set_state_idle (self->camera);
	}

	if (goo_component_get_state (self->camera) == OMX_StateIdle)
	{
		if (self->clock)
		{
			GST_INFO_OBJECT (self, "moving clock to to idle");
			goo_component_set_state_idle (self->clock);
		}

		if (priv->vstab == TRUE)
		{
			GST_INFO_OBJECT (self, "enabling vstab");
			g_object_set (self->camera, "vstab", TRUE, NULL);
		}

		GST_INFO_OBJECT (self, "camera: going to executing");
		goo_component_set_state_executing (self->camera);

		if (self->clock)
		{
			GST_INFO_OBJECT (self, "moving clock to to executing");
			goo_component_set_state_executing (self->clock);
		}

		GST_INFO_OBJECT (self, "seeting zoom = %d",priv->zoom);
		g_object_set (self->camera,"zoom", priv->zoom, NULL);

		GST_INFO_OBJECT (self, "seeting color effects = %d",priv->effects);
		g_object_set (self->camera,"effects", priv->effects, NULL);

		GST_INFO_OBJECT (self, "seeting exposure = %d",priv->exposure);
		g_object_set (self->camera,"exposure", priv->exposure, NULL);
	}

	GST_OBJECT_UNLOCK (self);
	return;
}

static gboolean
gst_goo_camera_setcaps (GstBaseSrc* self, GstCaps* caps)
{
	GstGooCamera* me = GST_GOO_CAMERA (self);
	GstGooCameraPrivate* priv = GST_GOO_CAMERA_GET_PRIVATE (me);
	GstStructure* structure;

	GST_DEBUG_OBJECT (me, "");
	structure = gst_caps_get_structure (caps, 0);

	gint width = 0, height = 0;
	gst_structure_get_int (structure, "width", &width);
	gst_structure_get_int (structure, "height", &height);

	guint32 color = gst_goo_camera_color_from_structure (structure);
	if (color == OMX_COLOR_FormatUnused)
	{
		return FALSE;
	}

	const GValue *framerate;
	guint fps_n, fps_d;
	framerate = gst_structure_get_value (structure, "framerate");

	if (G_LIKELY (framerate != NULL))
	{
		fps_n = gst_value_get_fraction_numerator (framerate);
		fps_d = gst_value_get_fraction_denominator (framerate);
	}
	else
	{
		fps_n = 0;
		fps_d = 1;
	}

	priv->fps_n = fps_n;
	priv->fps_d = fps_d;

	gst_goo_camera_omx_start (me, width, height, color, fps_n, fps_d);

	return TRUE;
}

static gboolean
gst_goo_camera_query (GstBaseSrc* self, GstQuery* query)
{
	GstGooCamera* me = GST_GOO_CAMERA (self);
	GstGooCameraPrivate* priv = GST_GOO_CAMERA_GET_PRIVATE (me);
	gboolean res = FALSE;

	switch (GST_QUERY_TYPE (query))
	{
	case GST_QUERY_LATENCY:
	{
		GST_DEBUG_OBJECT (me, "latency query");

		GstClockTime min_latency, max_latency;

		/* we must have a framerate */
		if (priv->fps_n <= 0 || priv->fps_d <= 0)
			goto done;

		/* min latency is the time to capture one frame */
		min_latency =
			gst_util_uint64_scale_int (GST_SECOND,
						   priv->fps_d, priv->fps_n);

		/* max latency is total duration of the frame buffer */
		/* FIXME: what to use here? */
		max_latency = 1 * min_latency;

		GST_DEBUG_OBJECT (me,
				  "report latency min %" GST_TIME_FORMAT
				  " max %" GST_TIME_FORMAT,
				  GST_TIME_ARGS (min_latency),
				  GST_TIME_ARGS (max_latency));

		/* we are always live, the min latency is 1 frame and the max
		 * latency is the complete buffer of frames. */
		gst_query_set_latency (query, TRUE, min_latency, max_latency);

		res = TRUE;
		break;
	}
	default:
		res = GST_BASE_SRC_CLASS (parent_class)->query (self, query);
		break;
	}

done:
	return res;
}

static GstFlowReturn
gst_goo_camera_create (GstPushSrc* self, GstBuffer **buffer)
{
	GstGooCamera* me = GST_GOO_CAMERA (self);
	GstGooCameraPrivate* priv = GST_GOO_CAMERA_GET_PRIVATE (me);
	GstBuffer* gst_buffer = NULL;
	OMX_BUFFERHEADERTYPE* omx_buffer = NULL;
	GST_DEBUG_OBJECT (me, " ");
	if (me->camera->cur_state != OMX_StateExecuting)
	{
		return GST_FLOW_UNEXPECTED;
	}

	if (goo_port_is_tunneled (me->captureport))
	{
		GST_INFO_OBJECT (me, "port is tunneled, send ghost_buffer");
		gst_buffer = GST_BUFFER (gst_ghost_buffer_new ());
		GST_DEBUG_OBJECT (me, "setting caps on ghost buffer");
		gst_buffer_set_caps (gst_buffer,
				     GST_PAD_CAPS (GST_BASE_SRC_PAD (self)));
		goto beach;

	}

	GST_DEBUG_OBJECT (me, "goo stuff");
	{
		omx_buffer = goo_port_grab_buffer (me->captureport);

#ifdef PADALLOC
		if (gst_pad_alloc_buffer (GST_BASE_SRC_PAD (self),
					  priv->outcount,
					  omx_buffer->nFilledLen,
					  GST_PAD_CAPS (
						  GST_BASE_SRC_PAD (self)
						  ),
					  &gst_buffer) == GST_FLOW_OK)
		{
			if (GST_IS_GOO_BUFFER (gst_buffer))
			{
				GST_DEBUG_OBJECT (me, "It is an OMX buffer!It is an OMX buffer!");
				OMX_BUFFERHEADERTYPE* buf;

				buf = GST_GOO_BUFFER (gst_buffer)->omx_buffer;
				memcpy (buf->pBuffer, omx_buffer->pBuffer,
					omx_buffer->nFilledLen);

				buf->nFilledLen = omx_buffer->nFilledLen;
				buf->nFlags = omx_buffer->nFlags;
				buf->nOffset = omx_buffer->nOffset;
				buf->nTickCount = omx_buffer->nTickCount;
				buf->nTimeStamp = omx_buffer->nTimeStamp;

				goo_component_release_buffer (me->camera,
							      omx_buffer);
			}
			else
			{
				GST_DEBUG_OBJECT (me, "It ain't an OMX buffer :( memcopy");
				/* gst_buffer_unref (gst_buffer); */
				/* GST_DEBUG ("Creating an new GooBuffer"); */
				/* gst_buffer = gst_goo_buffer_new (); */
				/* gst_goo_buffer_set_data (gst_buffer, me->camera, omx_buffer); */
				memcpy (GST_BUFFER_DATA (gst_buffer),
					omx_buffer->pBuffer,
					omx_buffer->nFilledLen);
				goo_component_release_buffer (me->camera,
							      omx_buffer);
			}
		}
		else
		{
			goto fail;
		}
#else
		GST_DEBUG_OBJECT (me, "Creating an new GooBuffer");
/* 		gst_buffer = gst_goo_buffer_new (); */
/* 		gst_goo_buffer_set_data (gst_buffer, me->camera, omx_buffer); */
		gst_buffer = gst_buffer_new_and_alloc (omx_buffer->nFilledLen);
		memmove (GST_BUFFER_DATA (gst_buffer),
			 omx_buffer->pBuffer, omx_buffer->nFilledLen);

		goo_component_release_buffer (me->camera, omx_buffer);
#endif

	}

	GST_DEBUG_OBJECT (me, "Timestamp and duration");
	{

		GST_DEBUG_OBJECT (self, "Timestamp Calculation");
		{
			GstClockTime timestamp;
			GST_DEBUG_OBJECT (me, "Try to Convert OMX to GST timestamps");
			timestamp = gst_goo_timestamp_omx2gst (omx_buffer);

			if (!GST_CLOCK_TIME_IS_VALID (timestamp))
			{
				GST_DEBUG_OBJECT (me, "Invalid omx to gst conversion, thus set the expected one");
				if (priv->fps_d && priv->fps_n)
				{
					timestamp = gst_util_uint64_scale_int
						(GST_SECOND,
						 priv->outcount * priv->fps_d,
						 priv->fps_n);
				}
				else
				{
					timestamp = GST_CLOCK_TIME_NONE;
				}

			}
			GST_BUFFER_TIMESTAMP (gst_buffer) = timestamp;
		}

		GST_DEBUG_OBJECT (me, "Durarion Calculating");
		{
			GstClockTime duration;
			if (priv->fps_n > 0)
			{
				duration = gst_util_uint64_scale_int
					(GST_SECOND, priv->fps_d, priv->fps_n);
			}
			else
			{
				duration = GST_CLOCK_TIME_NONE;
			}
			GST_BUFFER_DURATION (gst_buffer) = duration;
		}

		GST_DEBUG_OBJECT (self,
			"timestamp= %"GST_TIME_FORMAT" duration= "GST_TIME_FORMAT,
			GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (gst_buffer)),
			GST_TIME_ARGS (GST_BUFFER_DURATION (gst_buffer)));


		GST_DEBUG_OBJECT (me, "setting caps");
		gst_buffer_set_caps (gst_buffer,
				     GST_PAD_CAPS (GST_BASE_SRC_PAD (self)));
	}

beach:
	GST_DEBUG_OBJECT (me, "beach");
	GST_BUFFER_OFFSET (gst_buffer) = priv->outcount++;
	GST_BUFFER_OFFSET_END (gst_buffer) = priv->outcount;
	*buffer = gst_buffer;
	return GST_FLOW_OK;

fail:
	if (G_LIKELY (*buffer))
	{
		gst_buffer_unref (*buffer);
	}

	return GST_FLOW_ERROR;
}

static void
_gst_goo_camera_set_video_balance (GstGooCamera* self, guint prop_id,
				   const gint value)
{
	const GList* channels_list = NULL;
	GstColorBalanceChannel* found_channel = NULL;

	channels_list = gst_color_balance_list_channels
		(GST_COLOR_BALANCE (self));

	while (channels_list)
	{
		GstColorBalanceChannel* channel = channels_list->data;
		if (prop_id == PROP_CONTRAST && channel &&
		    g_strrstr (channel->label, CONTRAST_LABEL))
		{
			g_object_ref (channel);
			found_channel = channel;
		}
		else if (prop_id == PROP_BRIGHTNESS && channel &&
			 g_strrstr (channel->label, BRIGHTHNESS_LABEL))
		{
			g_object_ref (channel);
			found_channel = channel;
		}
		channels_list = g_list_next (channels_list);
	}

	if (GST_IS_COLOR_BALANCE_CHANNEL (found_channel))
	{
		/* nice interpolation for future use */
		/* value = value * ((double) found_channel->max_value - found_channel->min_value) / 65535 + found_channel->min_value; */
		gst_color_balance_set_value (GST_COLOR_BALANCE (self),
					     found_channel, value);
		g_object_unref (found_channel);
	}

	return;
}

static void
gst_goo_camera_set_property (GObject* object, guint prop_id,
			     const GValue* value, GParamSpec* pspec)
{
	GstGooCamera* self = GST_GOO_CAMERA (object);
	GstGooCameraPrivate* priv = GST_GOO_CAMERA_GET_PRIVATE (object);
	GooPort* viewfinding_port = NULL;

	switch (prop_id)
	{
	case PROP_NUM_OUTPUT_BUFFERS:
		priv->output_buffers = g_value_get_uint (value);
		break;
	case PROP_PREVIEW:
		priv->preview = g_value_get_boolean (value);
		break;
	case PROP_DISPLAY_WIDTH:
		priv->display_width = g_value_get_uint (value);
		break;
	case PROP_DISPLAY_HEIGHT:
		priv->display_height = g_value_get_uint (value);
		break;
	case PROP_DISPLAY_POS_X:
		g_object_set_property (G_OBJECT (self->postproc),
				       "x-pos", value);
		break;
	case PROP_DISPLAY_POS_Y:
		g_object_set_property (G_OBJECT (self->postproc),
				       "y-pos", value);
		break;
	case PROP_DISPLAY_ROTATION:
		g_object_set_property (G_OBJECT (self->postproc),
				       "rotation", value);
		break;
	case PROP_DISPLAY_VIDEOPIPELINE:
		priv->video_pipeline = g_value_get_uint (value);
		break;
	case PROP_OUTPUT_DEVICE:
		g_object_set_property (G_OBJECT (self->postproc),
				       "out-device", value);
		break;
	case PROP_CONTRAST:
	{
		gint contrast = g_value_get_int (value);
		_gst_goo_camera_set_video_balance (self, prop_id, contrast);
	}
	break;
	case PROP_BRIGHTNESS:
	{
		gint brightness = g_value_get_int (value);
		_gst_goo_camera_set_video_balance (self, prop_id, brightness);
	}
	break;
	case PROP_ZOOM:
		priv->zoom = g_value_get_enum (value);
		break;
	case PROP_BALANCE:
		g_object_set_property (G_OBJECT (self->camera),
				       "balance", value);
		break;
	case PROP_EXPOSURE:
		priv->exposure = g_value_get_enum (value);
		break;
	case PROP_FOCUS:
		priv->focus = g_value_get_enum (value);
		break;
	case PROP_VSTAB:
		priv->vstab = g_value_get_boolean (value);
		break;
	case PROP_EFFECTS:
		priv->effects = g_value_get_enum (value);
		break;
	case PROP_IPP:
		priv->ipp = g_value_get_boolean (value);
		break;
	case PROP_CAPMODE:
		priv->capmode = g_value_get_enum (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_camera_get_property (GObject* object, guint prop_id,
			     GValue* value, GParamSpec* pspec)
{
	GstGooCamera* self = GST_GOO_CAMERA (object);
	GstGooCameraPrivate* priv = GST_GOO_CAMERA_GET_PRIVATE (object);

	switch (prop_id)
	{
	case PROP_NUM_OUTPUT_BUFFERS:
		g_value_set_uint (value, priv->output_buffers);
		break;
	case PROP_PREVIEW:
		g_value_set_boolean (value, priv->preview);
		break;
	case PROP_DISPLAY_WIDTH:
		g_value_set_uint (value, priv->display_width);
		break;
	case PROP_DISPLAY_HEIGHT:
		g_value_set_uint (value, priv->display_height);
		break;
	case PROP_DISPLAY_POS_X:
		g_object_get_property (G_OBJECT (self->postproc),
				       "x-pos", value);
		break;
	case PROP_DISPLAY_POS_Y:
		g_object_get_property (G_OBJECT (self->postproc),
				       "y-pos", value);
		break;
	case PROP_DISPLAY_ROTATION:
		g_object_get_property (G_OBJECT (self->postproc),
				       "rotation", value);
		break;
	case PROP_DISPLAY_VIDEOPIPELINE:
		g_value_set_uint (value, priv->video_pipeline);
		break;
	case PROP_CONTRAST:
		g_object_get_property (G_OBJECT (self->camera),
				       "contrast", value);
		break;
	case PROP_OUTPUT_DEVICE:
		g_object_get_property (G_OBJECT (self->postproc),
				       "out-device", value);
		break;
	case PROP_BRIGHTNESS:
		g_object_get_property (G_OBJECT (self->camera),
				       "brightness", value);
		break;
	case PROP_ZOOM:
		g_object_get_property (G_OBJECT (self->camera),"zoom", value);
		break;
	case PROP_BALANCE:
		g_object_get_property (G_OBJECT (self->camera),"balance", value);
		break;
	case PROP_EXPOSURE:
		g_object_get_property (G_OBJECT (self->camera),"exposure", value);
		break;
	case PROP_FOCUS:
		g_object_get_property (G_OBJECT (self->camera),"focus", value);
		break;
	case PROP_VSTAB:
		g_value_set_boolean (value, priv->vstab);
		break;
	case PROP_EFFECTS:
		g_object_get_property (G_OBJECT (self->camera),"effects", value);
		break;
	case PROP_IPP:
		g_value_set_boolean (value, priv->ipp);
		break;
	case PROP_CAPMODE:
                g_value_set_enum (value, priv->capmode);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_camera_base_init (gpointer g_klass)
{
	GST_DEBUG_CATEGORY_INIT (gst_goo_camera_debug, "goocamera", 0,
				 "TI camera source element");

	GstElementClass* e_klass = GST_ELEMENT_CLASS (g_klass);

	gst_element_class_add_pad_template (e_klass,
					    gst_static_pad_template_get
					    (&src_factory));

	gst_element_class_set_details (e_klass, &details);

	return;
}

static void
gst_goo_camera_dispose (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->dispose (object);
	GstGooCamera* me = GST_GOO_CAMERA (object);

	GST_DEBUG ("");

	if (G_LIKELY (me->captureport != NULL))
	{
		GST_DEBUG ("unrefing captureport");
		g_object_unref (me->captureport);
	}

	if (G_LIKELY (me->camera != NULL))
	{
		GST_DEBUG ("unrefing camera");
		g_object_unref (me->camera);
	}

	if (G_LIKELY (me->postproc != NULL))
	{
		GST_DEBUG ("unrefing postproc");
		g_object_unref (me->postproc);
	}

	if (G_LIKELY (me->clock != NULL))
	{
			GST_DEBUG ("unrefing clock");
			g_object_unref (me->clock);
	}

	if (G_LIKELY (me->factory != NULL))
	{
		GST_DEBUG ("unrefing factory");
		g_object_unref (me->factory);
	}

	return;
}

static void
gst_goo_camera_finalize (GObject *object)
{
	GstGooCamera* me = GST_GOO_CAMERA (object);

	if (G_LIKELY (me->priv != NULL))
	{
		GST_DEBUG ("freeing private data");
		g_free (me->priv);
	}

	G_OBJECT_CLASS (parent_class)->dispose (object);
}
/*
* Utility function which only shows a log of the component's state
* transition
*
* @transition the transition to be done
*/

static void
gst_goo_camera_print_verbose_state_change(GstStateChange transition)
{
	switch(transition)
	{
		case GST_STATE_CHANGE_READY_TO_PAUSED:
		{
			GST_LOG("changing state GST_STATE_CHANGE_READY_TO_PAUSED");
			break;
		}
		case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
		{
			GST_LOG("Changing state GST_STATE_CHANGE_PAUSED_TO_PLAYING");
			break;
		}
		case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
		{
			GST_LOG("Changing state GST_STATE_CHANGE_PLAYING_TO_PAUSED");
			break;
		}
		case GST_STATE_CHANGE_PAUSED_TO_READY:
		{
			GST_LOG("Changing state GST_STATE_CHANGE_PAUSED_TO_READY");
			break;
		}
		case GST_STATE_CHANGE_READY_TO_NULL:
		{
			GST_LOG("Changing state GST_STATE_CHANGE_READY_TO_NULL");
			break;
		}
		case GST_STATE_CHANGE_NULL_TO_READY:
		{
			GST_LOG("Changing state GST_STATE_CHANGE_NULL_TO_READY");
			break;
		}
		default:
			break;
	}
	return;
}

static GstStateChangeReturn
gst_goo_camera_change_state (GstElement* element, GstStateChange transition)
{
	GST_LOG ("transition=%d", transition);
	gst_goo_camera_print_verbose_state_change(transition);

	GstGooCamera* self = GST_GOO_CAMERA (element);
	GstStateChangeReturn result;
	GstGooCameraPrivate* priv = GST_GOO_CAMERA_GET_PRIVATE (self);

	result = GST_ELEMENT_CLASS (parent_class)->change_state (element,
								 transition);

	/* Send a message with the timestamp of the component's state change */
	gchar *structure_name;
	structure_name = g_strdup_printf("%s_%d", "camera_transition",transition);
	gst_goo_util_post_message ( GST_ELEMENT (self), structure_name, NULL);
	g_free(structure_name);

	switch (transition)
	{
	case GST_STATE_CHANGE_READY_TO_PAUSED:
		{
			priv->last_pause_time = DEFAULT_START_TIME;
			break;
		}
	case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
		{
			GST_INFO_OBJECT (self, "setting focus = %d", priv->focus);
			g_object_set (self->camera, "focus", priv->focus, NULL);
			if (priv->capture == CAPMODE_DEFAULT)
			{
				GST_INFO_OBJECT (self, "Resume");
				if ((priv->capmode == VIDEO_MODE) || (priv->capmode == HIGH_PERFORMANCE_MODE))
					priv->capture = HIGH_PERFORMANCE_MODE;
				else
					priv->capture = HIGH_QUALITY_MODE;

				g_object_set (self->camera, "capture", priv->capture, NULL);
			}

			if (self->clock)
			{	/* Take pause time, so when resuming this would be the base time */
				GST_INFO_OBJECT (self, "Take pause time");
				goo_ti_clock_set_starttime (self->clock, priv->last_pause_time);
			}

		}
		break;
	case GST_STATE_CHANGE_PLAYING_TO_PAUSED:

		if (priv->capture != CAPMODE_DEFAULT)
		{
			GST_INFO_OBJECT (self, "Pause");
			g_object_set (self->camera, "capture", CAPMODE_DEFAULT, NULL);
			priv->capture = CAPMODE_DEFAULT;
		}

		if (self->clock)
		{
			/* Set the base time (the time where the clock starts) */
			GST_INFO_OBJECT (self, "Set base time");
			priv->last_pause_time = goo_ti_clock_get_timestamp (self->clock);
		}

		break;
	case GST_STATE_CHANGE_PAUSED_TO_READY:
		{
			GST_INFO_OBJECT (self, "Ready");
			gst_goo_camera_stop (GST_BASE_SRC(self));
		}
		break;
	default:
		break;
	}
	return result;
}

static void
gst_goo_camera_class_init (GstGooCameraClass* klass)
{
	GObjectClass* g_klass;
	GstPushSrcClass* p_klass;
	GstBaseSrcClass* b_klass;
	GstElementClass* gst_klass;
	GParamSpec* spec;

	{
		/* global constant */
		maxres = goo_get_resolution ("720p");

	}

	g_klass = G_OBJECT_CLASS (klass);
	g_type_class_add_private (klass, sizeof (GstGooCameraPrivate));
	g_klass->set_property =	GST_DEBUG_FUNCPTR (gst_goo_camera_set_property);
	g_klass->get_property =	GST_DEBUG_FUNCPTR (gst_goo_camera_get_property);
	g_klass->dispose = GST_DEBUG_FUNCPTR (gst_goo_camera_dispose);
	g_klass->finalize = GST_DEBUG_FUNCPTR (gst_goo_camera_finalize);

	spec = g_param_spec_uint ("output-buffers", "Output buffers",
				  "The number of output buffers in OMX",
				  1, 6, NUM_OUTPUT_BUFFERS_DEFAULT,
				  G_PARAM_READWRITE);
	g_object_class_install_property (g_klass,
					 PROP_NUM_OUTPUT_BUFFERS, spec);

#if 0
	spec = g_param_spec_boolean ("preview", "Enable preview",
				     "Enable the display preview",
				     PREVIEW_DEFAULT, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_PREVIEW, spec);
#endif

	spec = g_param_spec_uint ("display-width", "Display width",
				  "Set the preview display width",
				  176, maxres.width, DISPLAY_WIDTH_DEFAULT,
				  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_DISPLAY_WIDTH, spec);

	spec = g_param_spec_uint ("display-height", "Display height",
				  "Set the preview display height",
				  144, maxres.height, DISPLAY_HEIGHT_DEFAULT,
				  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_DISPLAY_HEIGHT, spec);

	spec = g_param_spec_uint ("display-pos-x", "Display X position",
				  "Set the preview display X position",
				  0, maxres.width, DISPLAY_POSX_DEFAULT,
				  G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_DISPLAY_POS_X, spec);

	spec = g_param_spec_uint ("display-pos-y", "Display Y position",
				  "Set the preview display Y position",
				  0, maxres.height, DISPLAY_POSY_DEFAULT,
				  G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_DISPLAY_POS_Y, spec);

	spec = g_param_spec_enum ("display-rotation", "Display rotation",
				  "Set the preview display rotation",
				  GST_GOO_CAMERA_ROTATION,
				  DISPLAY_ROTATION_DEFAULT,
				  G_PARAM_READWRITE);
	g_object_class_install_property (g_klass,
					 PROP_DISPLAY_ROTATION, spec);

	spec = g_param_spec_enum ("outdev", "Output device",
				   "The output device",
				   GST_GOO_CAMERA_OUTPUT,
				   OUTPUT_DEFAULT, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_OUTPUT_DEVICE, spec);

	spec = g_param_spec_int ("contrast", "Contrast",
				 "Set the contrast value",
				 -100, 100, CONTRAST_DEFAULT,
				 G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_CONTRAST, spec);

	spec = g_param_spec_int ("brightness", "Brightness",
				 "Set the brightness value",
				 -100, 100, BRIGHTNESS_DEFAULT,
				 G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_BRIGHTNESS, spec);

	spec = g_param_spec_uint ("videolayer", "VideoPipeline",
				   "Video pipeline/layer",
				   1, 2, VIDEOPIPELINE_DEFAULT,
				   G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass,
					 PROP_DISPLAY_VIDEOPIPELINE, spec);

	spec = g_param_spec_enum ("zoom", "Zoom value",
				  "Set/Get the zoom value",
				  GOO_TI_CAMERA_ZOOM, ZOOM_DEFAULT,
				  G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_ZOOM, spec);

	spec = g_param_spec_enum ("balance", "White balance control",
				  "Set/Get the white balance mode",
				  GOO_TI_CAMERA_WHITE_BALANCE,
				  BALANCE_DEFAULT, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_BALANCE, spec);

	spec = g_param_spec_enum ("exposure",
				  "Exposure control",
				  "Set the exposure mode",
				  GOO_TI_CAMERA_EXPOSURE,
				  EXPOSURE_DEFAULT,  G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_EXPOSURE, spec);

	spec = g_param_spec_enum ("focus",
				  "Focus control",
				  "Set the autofocus mode ",
				  GOO_TI_CAMERA_MODE_FOCUS,
				  FOCUS_DEFAULT, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_FOCUS, spec);

	spec = g_param_spec_boolean ("vstab", "Video stabilization",
				     "Enable the video stabilization",
				     VSTAB_DEFAULT,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_VSTAB, spec);

	spec = g_param_spec_enum ("effects",
				  "Color effects",
				  "Set/Get the color effects",
				  GOO_TI_CAMERA_EFFECTS,
				  EFFECTS_DEFAULT, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_EFFECTS, spec);

	spec = g_param_spec_boolean ("ipp", "Image Processing Pipeline",
				     "Enabled IPP",
				     IPP_DEFAULT, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_IPP, spec);

	spec = g_param_spec_enum ("capture-mode", "Capture mode",
				  "Select capture mode",
				  GST_GOO_CAMERA_CAPTURE_MODE,
				  CAPMODE_DEFAULT, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_CAPMODE, spec);

	/* GST stuff */
	p_klass = GST_PUSH_SRC_CLASS (klass);
	p_klass->create = GST_DEBUG_FUNCPTR (gst_goo_camera_create);
	b_klass = GST_BASE_SRC_CLASS (klass);
	b_klass->start    = GST_DEBUG_FUNCPTR (gst_goo_camera_start);
	b_klass->set_caps = GST_DEBUG_FUNCPTR (gst_goo_camera_setcaps);
	b_klass->query    = GST_DEBUG_FUNCPTR (gst_goo_camera_query);
	b_klass->fixate   = GST_DEBUG_FUNCPTR (gst_goo_camera_fixate);

	/* GST */
	gst_klass = GST_ELEMENT_CLASS (klass);
	gst_klass->change_state = GST_DEBUG_FUNCPTR (gst_goo_camera_change_state);

	return;
}

static void
gst_goo_camera_init (GstGooCamera* self, GstGooCameraClass* klass)
{
	GST_DEBUG ("");

	{
		self->priv = g_new0 (GstGooCameraPrivate, 1);
		GstGooCameraPrivate* priv = GST_GOO_CAMERA_GET_PRIVATE (self);

		priv->outcount = 0;
		priv->fps_n = priv->fps_d = 0;
		priv->preview = PREVIEW_DEFAULT;
		priv->video_pipeline = VIDEOPIPELINE_DEFAULT;
		priv->display_width = DISPLAY_WIDTH_DEFAULT;
		priv->display_height = DISPLAY_HEIGHT_DEFAULT;
		priv->capture = CAPMODE_DEFAULT;
		priv->vstab = VSTAB_DEFAULT;
		priv->focus = FOCUS_DEFAULT;
		priv->zoom = ZOOM_DEFAULT;
		priv->effects = EFFECTS_DEFAULT;
		priv->exposure = EXPOSURE_DEFAULT;
		priv->ipp = IPP_DEFAULT;
		priv->capmode = CAPMODE_DEFAULT;
		priv->output_buffers = NUM_OUTPUT_BUFFERS_DEFAULT;
		priv->last_pause_time = DEFAULT_START_TIME;
	}

	/* color balance */
	{
		self->channels = NULL;

		GstColorBalanceChannel* channel;

		/* contrast */
		channel = g_object_new (GST_TYPE_COLOR_BALANCE_CHANNEL, NULL);
		channel->label = g_strdup (CONTRAST_LABEL);
		channel->min_value = -100;
		channel->max_value = 100;
		self->channels = g_list_append (self->channels, channel);

		/* brightness */
		channel = g_object_new (GST_TYPE_COLOR_BALANCE_CHANNEL, NULL);
		channel->label = g_strdup (BRIGHTHNESS_LABEL);
		channel->min_value = -100;
		channel->max_value = 100;
		self->channels = g_list_append (self->channels, channel);
	}

	{
		self->clock = NULL;
	}

	/* goo component */
	self->factory = goo_ti_component_factory_get_instance ();

	self->camera = goo_component_factory_get_component
		(self->factory, GOO_TI_CAMERA);

	self->postproc = goo_component_factory_get_component
		(self->factory, GOO_TI_POST_PROCESSOR);

	self->captureport = goo_component_get_port (self->camera, "output1");
	g_assert (self->captureport != NULL);

	g_object_set_data (G_OBJECT (self->camera), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", self->camera);

	/* gst stuff */
	gst_base_src_set_format (GST_BASE_SRC (self), GST_FORMAT_TIME);
	gst_base_src_set_live (GST_BASE_SRC (self), TRUE);

	gst_pad_set_event_function (GST_BASE_SRC (self)->srcpad,
		GST_DEBUG_FUNCPTR (gst_goo_camera_src_event));

	g_signal_connect(self->camera, "PPM_focus_start", (GCallback) gst_goo_camera_cb_PPM_focus_start, self);
	g_signal_connect(self->camera, "PPM_focus_end", (GCallback) gst_goo_camera_cb_PPM_focus_end, self);
	g_signal_connect(self->camera, "PPM_omx_event", (GCallback) gst_goo_camera_cb_PPM_omx_events, self);

	return;
}
