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
#include <goo-ti-video-encoder.h>
#include <goo-ti-post-processor.h>
#include <goo-ti-jpegenc.h>
#include <goo-utils.h>

#include "gstgoocamera.h"
#include "gstghostbuffer.h"
#include "gstgoobuffer.h"
#include "string.h"

GST_DEBUG_CATEGORY_STATIC (gst_goo_camera_debug);
#define GST_CAT_DEFAULT gst_goo_camera_debug

/* signals */
enum
{
	LAST_SIGNAL
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
	PROP_CONTRAST,
	PROP_BRIGHTNESS,
	PROP_ZOOM,
	PROP_BALANCE,
	PROP_VSTAB
};

#define GST_GOO_CAMERA_GET_PRIVATE(obj) (GST_GOO_CAMERA (obj)->priv)

struct _GstGooCameraPrivate
{
	gboolean preview;
	guint video_pipeline;
	guint outcount;
	guint display_height;
	guint display_width;
	gboolean capture;
	gint fps_n, fps_d;
	gboolean vstab;
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

#define NUM_OUTPUT_BUFFERS_DEFAULT 4
#define PREVIEW_DEFAULT		   TRUE
#define VIDEOPIPELINE_DEFAULT      2
#define DISPLAY_WIDTH_DEFAULT      176
#define DISPLAY_HEIGHT_DEFAULT     144
#define DISPLAY_POSX_DEFAULT       0
#define DISPLAY_POSY_DEFAULT       0
#define COLOR_DEFAULT              OMX_COLOR_FormatYCbYCr
#define BALANCE_DEFAULT            OMX_WhiteBalControlAuto
#define ZOOM_DEFAULT               GOO_TI_CAMERA_ZOOM_1X
#define CONTRAST_DEFAULT	   -70
#define BRIGHTNESS_DEFAULT	   10
#define DISPLAY_ROTATION_DEFAULT   GOO_TI_POST_PROCESSOR_ROTATION_NONE
#define VSTAB_DEFAULT              FALSE
#define CONTRAST_LABEL		   "Contrast"
#define BRIGHTHNESS_LABEL	   "Brightness"

/* use the base clock to timestamp the frame */
#define BASECLOCK
/* #undef BASECLOCK */

/* use gst_pad_alloc_buffer */
/* #define PADALLOC */
#undef PADALLOC

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

	GST_DEBUG_OBJECT (self, "Stop enter");

	if (priv->capture == TRUE)
	{
		GST_INFO_OBJECT (self, "Capture off");
		g_object_set (me->camera, "capture", FALSE, NULL);
	}
	
	GST_INFO_OBJECT (self, "going to idle");
	goo_component_set_state_idle (me->camera);
	GST_INFO_OBJECT (self, "exit to idle");
	
	GST_INFO_OBJECT (self, "going to loaded");
	goo_component_set_state_loaded (me->camera);
	
	return TRUE;
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
gst_goo_camera_sync (GstGooCamera* self, gint width, gint height,
		     guint32 color, guint fps_n, guint fps_d)
{
	GstGooCameraPrivate* priv = GST_GOO_CAMERA_GET_PRIVATE (self);
	gboolean one_shot = FALSE;
	GooComponent *component=NULL;
	GST_DEBUG_OBJECT (self, "");

	/* sensor configuration */
	{
		OMX_PARAM_SENSORMODETYPE* param =
			GOO_TI_CAMERA_GET_PARAM (self->camera);

		one_shot = param->bOneShot = (fps_n == 0 && fps_d == 1);
		GST_INFO_OBJECT (self, "Oneshot mode = %d", param->bOneShot);

		if (param->bOneShot == TRUE)
		{
			param->nFrameRate = 15;
		}
		else if (fps_d != 0)
		{
			param->nFrameRate = fps_n / fps_d;
		}
		else
		{
			param->nFrameRate = 15;
		}

		param->sFrameSize.nWidth = width;
		param->sFrameSize.nHeight = height;
		GST_INFO_OBJECT (self, "sensor dwidth = %d | sensor dheight = %d",
					 param->sFrameSize.nWidth,
					 param->sFrameSize.nHeight);	
	}
	
	if (priv->preview == TRUE)
	{
		/* display configuration */
		{
			if (one_shot == FALSE)
			{	
				priv->display_width  = width;
				priv->display_height = height;
				
			}
			else if (priv->display_width == 0 &&
				 priv->display_height == 0)
			{
				priv->display_width =
					MIN (maxres.width, width);
				priv->display_height =
					MIN (maxres.height, height);
			}
			else
			{
				priv->display_width = 320;
				priv->display_height = 240;
					
			}
			GST_INFO_OBJECT (self, " preview dwidth = %d | preview dheight = %d",
					 priv->display_width,
					 priv->display_height);
		}

		/* postroc input port */
		{
			GooTiPostProcessor* pp;
			pp = GOO_TI_POST_PROCESSOR (self->postproc);
			pp->video_pipeline = priv->video_pipeline;

			GooPort* port =
				goo_component_get_port (self->postproc,
							"input0");
			g_assert (port != NULL);

			OMX_PARAM_PORTDEFINITIONTYPE* param;
			param = GOO_PORT_GET_DEFINITION (port);

			param->format.video.nFrameWidth = priv->display_width;
			param->format.video.nFrameHeight =priv->display_height;
			param->format.video.eColorFormat = color;

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
#if 0	/* we can disable thumbnail port by the moment */
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

		if (GOO_TI_CAMERA_GET_PARAM (self->camera)->bOneShot == TRUE)
		{
			param->format.image.eColorFormat = color;
			
		}
		else
		{
			param->format.video.eColorFormat = color;
			
		}
	}
		
	/*Video encoder o jpeg encoder configuration*/	
	{
		GstPad *peer, *src_peer;
		GstElement *next_element=NULL;
		
		if (!(GST_BASE_SRC_PAD (self)))
		{
			GST_INFO ("it is not a src pad");
			goto no_enc;	
		}
		
		else
		{
			peer = gst_pad_get_peer (GST_BASE_SRC_PAD (self));
		
			if ( peer == NULL)
			{
				GST_INFO ("No next pad");
				goto no_enc;	
			}
								
			else 
			{
				next_element = GST_ELEMENT (gst_pad_get_parent (peer));
				
				if  G_UNLIKELY (next_element == NULL)
				{	
					goto no_enc;
				}
				
				else
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
					
					if (GOO_IS_TI_VIDEO_ENCODER (component))
					{
							GST_INFO_OBJECT (self, "There is a video encoder" );
						/* input port */
						{
							GooPort *peer_port = goo_component_get_port (component, "input0");
							g_assert (peer_port != NULL);
							OMX_PARAM_PORTDEFINITIONTYPE* param =GOO_PORT_GET_DEFINITION (peer_port);					
									
							param->format.video.xFramerate = (fps_n / fps_d)<<16;
							param->format.video.nFrameWidth =   width;
							param->format.video.nFrameHeight =	height;
							param->format.video.eColorFormat = color;
							g_object_unref (peer_port);
						}		
						
						/* output port */
						{	
							GooPort* port = goo_component_get_port (component, "output0");
							g_assert (port != NULL);
							OMX_PARAM_PORTDEFINITIONTYPE* param_out =GOO_PORT_GET_DEFINITION (port);
				
							param_out->format.video.nFrameWidth =   width;
							param_out->format.video.nFrameHeight =	height;
						
							GST_INFO_OBJECT (self, "setting up tunnel with video encoder ");
							goo_component_set_tunnel_by_name (self->camera, "output1",
					  				component, "input0", 
					  				OMX_BufferSupplyInput);
							g_object_unref (port);
					  	}
					}
					
					else if ( GOO_IS_TI_JPEGENC (component))
					{
						/* input port */
						{
							GooPort *peer_port = goo_component_get_port (component, "input0");
							g_assert (peer_port != NULL);
							OMX_PARAM_PORTDEFINITIONTYPE* param =GOO_PORT_GET_DEFINITION (peer_port);					
									
							param->format.image.nFrameWidth = width;
							param->format.image.nFrameHeight = height;
							param->format.image.eColorFormat = color;
							
							g_object_unref (peer_port);
						}		
						
						/* output port */
						{	
							GooPort* port = goo_component_get_port (component, "output0");
							g_assert (port != NULL);
							OMX_PARAM_PORTDEFINITIONTYPE* param_out =GOO_PORT_GET_DEFINITION (port);
				
							param_out->format.image.nFrameWidth = width;
							param_out->format.image.nFrameHeight = height;
							param_out->format.image.eColorFormat = color;
								
					  		GST_INFO_OBJECT (self, "setting up tunnel with jpeg encoder ");
							goo_component_set_tunnel_by_name (self->camera, "output1",
					  				component, "input0", 
					  				OMX_BufferSupplyInput);
					  		g_object_unref (port);
					  	}
					}
					gst_object_unref (next_element);
				}	
				gst_object_unref (peer);
			}
		} /* end of capture port configuration */
	}
	
no_enc:

	GST_INFO_OBJECT (self, "setting up tunnel with post processor");
	goo_component_set_tunnel_by_name (self->camera, "output0",
				  self->postproc, "input0",
				  OMX_BufferSupplyInput);
	GooPort* port_pp =	goo_component_get_port (self->postproc,
				"input0");
	g_assert (port_pp != NULL);
	goo_component_set_supplier_port (self->postproc, port_pp, OMX_BufferSupplyInput);
	gst_object_unref (port_pp);
	
	GST_INFO_OBJECT (self, "going to idle");
	goo_component_set_state_idle (self->camera);

	if (priv->vstab == TRUE)
	{
		GST_INFO_OBJECT (self, "enabling vstab");
		g_object_set (self->camera, "vstab", TRUE, NULL);
	}

	GST_INFO_OBJECT (self, "camera: going to executing");
	goo_component_set_state_executing (self->camera);

	if (component != NULL)
	{
		g_object_unref (component);
	}
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

	gst_goo_camera_sync (me, width, height, color, fps_n, fps_d);
	
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

	if (priv->capture == FALSE)
	{
		GST_INFO_OBJECT (me, "Capture on");
		g_object_set (me->camera, "capture", TRUE, NULL);
		priv->capture = !priv->capture;
	}

	if (goo_port_is_tunneled (me->captureport))
	{
		GST_INFO_OBJECT (me, "port is tunneled, send ghost_buffer");
		gst_buffer = gst_ghost_buffer_new ();
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


	GST_DEBUG_OBJECT (me, "gst stuff");
	{
		GstClock* clock = NULL;
		GstClockTime timestamp, duration;

#ifdef BASECLOCK /** @fixme: muxing sync hack */
		GST_DEBUG_OBJECT (me, "Using element clock");
		GST_OBJECT_LOCK (self);
		if ((clock = GST_ELEMENT_CLOCK (self)))
		{
			/* we have a clock, get base time and ref clock */
			timestamp = GST_ELEMENT (self)->base_time;
			gst_object_ref (clock);
		}
		else
		{
			/* no clock, can't set timestamps */
			timestamp = GST_CLOCK_TIME_NONE;
		}
		GST_OBJECT_UNLOCK (self);

		if (clock)
		{
			/* the time now is the time of the clock minus the
			 * base time */
			timestamp = gst_clock_get_time (clock) - timestamp;
			gst_object_unref (clock);
		}
#else
		if (priv->framerate > 0)
		{
			timestamp = gst_util_uint64_scale_int
				(GST_SECOND,
				 priv->outcount * priv->rate_denominator,
				 priv->rate_numerator);
		}
		else
		{
			timestamp = GST_CLOCK_TIME_NONE;
		}
#endif

		if (priv->fps_n > 0)
		{
			duration = gst_util_uint64_scale_int
				(GST_SECOND, priv->fps_d, priv->fps_n);
		}
		else
		{
			duration = GST_CLOCK_TIME_NONE;
		}

		GST_BUFFER_TIMESTAMP (gst_buffer) = timestamp;
		GST_BUFFER_DURATION (gst_buffer) = duration;

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

	switch (prop_id)
	{
	case PROP_NUM_OUTPUT_BUFFERS:
		g_object_set_property (G_OBJECT (self->captureport),
				       "buffercount", value);
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
		g_object_set_property (G_OBJECT (self->camera),
				       "zoom", value);
		break;
	case PROP_BALANCE:
		g_object_set_property (G_OBJECT (self->camera),
				       "balance", value);
		break;
	case PROP_VSTAB:
		priv->vstab = g_value_get_boolean (value);
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
		g_object_get_property (G_OBJECT (self->captureport),
				       "buffercount", value);
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
	case PROP_BRIGHTNESS:
		g_object_get_property (G_OBJECT (self->camera),
				       "brightness", value);
		break;
	case PROP_ZOOM:
		g_object_get_property (G_OBJECT (self->camera),
				       "zoom", value);
		break;
	case PROP_BALANCE:
		g_object_get_property (G_OBJECT (self->camera),
				       "balance", value);
		break;
	case PROP_VSTAB:
		g_value_set_boolean (value, priv->vstab);
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

static void
gst_goo_camera_class_init (GstGooCameraClass* klass)
{
	GObjectClass* g_klass;
	GstPushSrcClass* p_klass;
	GstBaseSrcClass* b_klass;
	GParamSpec* spec;

	{
		/* global constant */
		maxres = goo_get_resolution ("cif");
		
	}

	g_klass = G_OBJECT_CLASS (klass);
	g_type_class_add_private (klass, sizeof (GstGooCameraPrivate));

	g_klass->set_property =
		GST_DEBUG_FUNCPTR (gst_goo_camera_set_property);
	g_klass->get_property =
		GST_DEBUG_FUNCPTR (gst_goo_camera_get_property);
	g_klass->dispose = GST_DEBUG_FUNCPTR (gst_goo_camera_dispose);
	g_klass->finalize = GST_DEBUG_FUNCPTR (gst_goo_camera_finalize);

	spec = g_param_spec_uint ("output-buffers", "Output buffers",
				  "The number of output buffers in OMX",
				  1, 4, NUM_OUTPUT_BUFFERS_DEFAULT,
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
				  1, maxres.width, DISPLAY_WIDTH_DEFAULT,
				  G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_DISPLAY_WIDTH, spec);

	spec = g_param_spec_uint ("display-height", "Display height",
				  "Set the preview display height",
				  1, maxres.height, DISPLAY_HEIGHT_DEFAULT,
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

	spec = g_param_spec_int ("contrast", "Contrast",
				 "Set the contrast value",
				 -100, 100, CONTRAST_DEFAULT,
				 G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_CONTRAST, spec);

	spec = g_param_spec_int ("brightness", "Brightness",
				 "Set the brightness value",
				 0, 100, BRIGHTNESS_DEFAULT,
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

	spec = g_param_spec_boolean ("vstab", "Video stabilization",
				     "Enable the video stabilization",
				     VSTAB_DEFAULT,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_VSTAB, spec);

	/* GST stuff */
	p_klass = GST_PUSH_SRC_CLASS (klass);
	p_klass->create = GST_DEBUG_FUNCPTR (gst_goo_camera_create);
	b_klass = GST_BASE_SRC_CLASS (klass);
	b_klass->start    = GST_DEBUG_FUNCPTR (gst_goo_camera_start);
	b_klass->stop     = GST_DEBUG_FUNCPTR (gst_goo_camera_stop);
	b_klass->set_caps = GST_DEBUG_FUNCPTR (gst_goo_camera_setcaps);
	b_klass->query    = GST_DEBUG_FUNCPTR (gst_goo_camera_query);
	b_klass->fixate   = GST_DEBUG_FUNCPTR (gst_goo_camera_fixate);

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
		priv->display_width =
			priv->display_height = 0;
		priv->capture = FALSE;
		priv->fps_n = priv->fps_d = 0;
		priv->vstab = VSTAB_DEFAULT;
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
		channel->min_value = 0;
		channel->max_value = 100;
		self->channels = g_list_append (self->channels, channel);
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

	return;
}
