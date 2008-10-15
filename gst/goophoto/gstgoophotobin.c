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

#include <gst/gst.h>
#include <goo-ti-camera.h>
#include <goo-ti-jpegenc.h>

#define GST_TYPE_GOO_PHOTOBIN \
     (gst_goo_photobin_get_type ())
#define GST_GOO_PHOTOBIN(obj) \
     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_GOO_PHOTOBIN, GstGooPhotoBin))
#define GST_GOO_PHOTOBIN_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_GOO_PHOTOBIN, GstGooPhotoBinClass))
#define GST_GOO_PHOTOBIN_GET_CLASS(obj) \
     (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_GOO_PHOTOBIN, GstGooPhotoBinClass))
#define GST_IS_GOO_PHOTOBIN(obj) \
     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_GOO_PHOTOBIN))
#define GST_IS_GOO_PHOTOBIN_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_GOO_PHOTOBIN))

typedef struct _GstGooPhotoBin GstGooPhotoBin;
typedef struct _GstGooPhotoBinClass GstGooPhotoBinClass;

struct _GstGooPhotoBin
{
	GstBin parent;

	GstElement* camera;
	GstElement* jpegenc;
	GstPad* ghostpad;
	GstCaps* caps;
};

struct _GstGooPhotoBinClass
{
	GstBinClass parent_class;
};

GST_DEBUG_CATEGORY_STATIC (gst_goo_photobin_debug);
#define GST_CAT_DEFAULT gst_goo_photobin_debug


enum
{
	PROP_0,
	PROP_CAPS
};

static GstStaticPadTemplate src_factory =
	GST_STATIC_PAD_TEMPLATE (
		"src",
		GST_PAD_SRC,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS_ANY
		);

static const GstElementDetails details =
	GST_ELEMENT_DETAILS (
		"OpenMAX Image Capture",
		"Generic/Bin/Image",
		"Capture images with the camera in JPEG format with OpenMAX",
		"Texas Instrument"
		);

GST_BOILERPLATE (GstGooPhotoBin, gst_goo_photobin, GstBin, GST_TYPE_BIN);

static gboolean
gst_goo_photobin_build (GstGooPhotoBin* self)
{
	gboolean ret = TRUE;

	gint width = 1600;
	gint height = 1200;
	gint quality = 100;

	GST_INFO ("Linking elements");
	{
		GstCaps *caps;

		caps = gst_caps_new_simple ("video/x-raw-yuv",
					    "width", G_TYPE_INT, width,
					    "height", G_TYPE_INT, height,
					    "framerate", GST_TYPE_FRACTION,
					    0, 1,
					    "format", GST_TYPE_FOURCC,
					    GST_MAKE_FOURCC ('U', 'Y', 'V', 'Y'),
					    NULL);

		ret = gst_element_link_pads_filtered (self->camera, "src",
						      self->jpegenc, "sink",
						      caps);

		gst_caps_unref (caps);

		if (!ret)
		{
			goto done;
		}
	}

	GST_INFO ("Tunneling components");
	{
		GooComponent* camera =
			g_object_get_data (G_OBJECT (self->camera), "goo");

		g_assert (camera != NULL);

		GooComponent* jpegenc =
			g_object_get_data (G_OBJECT (self->jpegenc), "goo");

		g_assert (camera != NULL);

		/* CAMERA */
		{
			{
				OMX_PARAM_SENSORMODETYPE* param;
				param = GOO_TI_CAMERA_GET_PARAM (camera);
				param->bOneShot = OMX_TRUE;
				param->nFrameRate = 0;
				param->sFrameSize.nWidth = width;
				param->sFrameSize.nHeight = height;
			}

			/* output port */
			{
				GooPort* port = goo_component_get_port
					(camera, "output0");
				g_assert (port != NULL);

				g_object_set (port, "buffercount", 1, NULL);

				OMX_PARAM_PORTDEFINITIONTYPE* param;
				param = GOO_PORT_GET_DEFINITION (port);
				param->format.image.eColorFormat =
					OMX_COLOR_FormatCbYCrY;

				g_object_unref (port);
			}

		}

		/* JPEG ECODER */
		{
			/* param */
			{
				OMX_IMAGE_PARAM_QFACTORTYPE* param;

				param = GOO_TI_JPEGENC_GET_PARAM (jpegenc);
				param->nQFactor = quality;
			}

			/* inport */
			{
				GooPort* port = goo_component_get_port
					(jpegenc, "input0");
				g_assert (port != NULL);

				OMX_PARAM_PORTDEFINITIONTYPE* param;
				param = GOO_PORT_GET_DEFINITION (port);
				param->format.image.nFrameWidth = width;
				param->format.image.nFrameHeight = height;
				param->format.image.eColorFormat =
					OMX_COLOR_FormatCbYCrY;

				/* mandatory for JPEG encoder */
				g_object_set (port, "buffercount", 1, NULL);

				g_object_unref (port);
			}

			/* outport */
			{
				GooPort* port = goo_component_get_port
					(jpegenc, "output0");
				g_assert (port != NULL);

				OMX_PARAM_PORTDEFINITIONTYPE* param;
				param = GOO_PORT_GET_DEFINITION (port);
				param->format.image.nFrameWidth = width;
				param->format.image.nFrameHeight = height;
				param->format.image.eColorFormat =
					OMX_COLOR_FormatCbYCrY;

				/* Mandatory for JPEG encoder */
				g_object_set (port, "buffercount", 1, NULL);

				g_object_unref (port);
			}
		}

		goo_component_set_tunnel_by_name (camera, "output0",
						  jpegenc, "input0", OMX_BufferSupplyOutput);

		GST_INFO ("camera to idle");
		goo_component_set_state_idle (camera);
		GST_INFO ("jpegenc to idle");
		goo_component_set_state_idle (jpegenc);
	}

done:
	return ret;
}

static gboolean
gst_goo_photobin_stop (GstGooPhotoBin* self)
{
	g_return_val_if_fail (GST_IS_GOO_PHOTOBIN (self), FALSE);

	GST_DEBUG ("");

	gboolean ret = TRUE;

	GooComponent* camera =
		g_object_get_data (G_OBJECT (self->camera), "goo");

	g_assert (camera != NULL);

	GooComponent* jpegenc =
		g_object_get_data (G_OBJECT (self->jpegenc), "goo");

	g_assert (camera != NULL);

	GST_INFO ("camera to idle");
	goo_component_set_state_idle (camera);
	GST_INFO ("jpegenc to idle");
	goo_component_set_state_idle (jpegenc);

	return ret;
}

static GstStateChangeReturn
gst_goo_photobin_change_state (GstElement* element, GstStateChange transition)
{
	GstGooPhotoBin *self = GST_GOO_PHOTOBIN (element);

	GST_DEBUG ("Change state 0x%x", transition);

	switch (transition)
	{
	case GST_STATE_CHANGE_NULL_TO_READY:
		if (!gst_goo_photobin_build (self))
		{
			return GST_STATE_CHANGE_FAILURE;
		}
		break;
	case GST_STATE_CHANGE_PAUSED_TO_READY:
		if (!gst_goo_photobin_stop (self))
		{
			return GST_STATE_CHANGE_FAILURE;
		}
		break;
	default:
		break;
	}


	return GST_ELEMENT_CLASS (parent_class)->change_state (element,
							       transition);
}

static void
gst_goo_photobin_set_property (GObject* object, guint prop_id,
			       const GValue* value, GParamSpec* pspec)
{
	GstGooPhotoBin* self = GST_GOO_PHOTOBIN (object);

	switch (prop_id)
	{
	case PROP_CAPS:
	{
		gpointer data;
		data = g_value_get_pointer (value);
		g_return_if_fail (GST_IS_CAPS(data));
		self->caps = GST_CAPS (data);
		break;
	}
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_photobin_dispose (GObject* object)
{
	G_OBJECT_CLASS (parent_class)->dispose (object);

	GstGooPhotoBin* self = GST_GOO_PHOTOBIN (object);

	if (G_LIKELY (self->caps))
	{
		GST_INFO ("unrefing caps");
		gst_object_unref (self->caps);
	}

	return;
}

static void
gst_goo_photobin_base_init (gpointer g_klass)
{
	GST_DEBUG_CATEGORY_INIT (gst_goo_photobin_debug, "goophotobin", 0,
				 "TI photo bin element");

	GstElementClass* e_klass = GST_ELEMENT_CLASS (g_klass);

	gst_element_class_add_pad_template (e_klass,
					    gst_static_pad_template_get
					    (&src_factory));

	gst_element_class_set_details (e_klass, &details);

	return;
}

static void
gst_goo_photobin_class_init (GstGooPhotoBinClass* klass)
{
	GObjectClass* g_klass;
	GstElementClass* gst_klass;

	g_klass = G_OBJECT_CLASS (klass);
	g_klass->dispose = GST_DEBUG_FUNCPTR (gst_goo_photobin_dispose);
	g_klass->set_property =
		GST_DEBUG_FUNCPTR (gst_goo_photobin_set_property);

	GParamSpec* pspec;
/* 	pspec = g_param_spec_pointer ("caps", "Capabilities", */
/* 				      "The capabilities for the camera", */
/* 				      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY); */
/* 	g_object_class_install_property (g_klass, PROP_CAPS, pspec); */

	gst_klass = GST_ELEMENT_CLASS (klass);
	gst_klass->change_state = gst_goo_photobin_change_state;

	return;
}

static void
gst_goo_photobin_init (GstGooPhotoBin* self, GstGooPhotoBinClass* klass)
{
	GST_DEBUG ("");

	self->caps = NULL;

	GST_INFO ("creating camera");
	self->camera = gst_element_factory_make ("goocamera", "camera");
	g_assert (self->camera != NULL);

	gst_bin_add (GST_BIN (self), self->camera);
	g_object_set (self->camera, "num-buffers", 5, NULL);
	g_object_set (self->camera, "display-rotation", 90, NULL);
	g_object_set (self->camera, "display-width", 320, NULL);
	g_object_set (self->camera, "display-height", 240, NULL);

	GST_INFO ("creating jpegenc");
	self->jpegenc = gst_element_factory_make ("gooenc_jpeg", "jpegenc");
	g_assert (self->jpegenc != NULL);

	gst_bin_add (GST_BIN (self), self->jpegenc);

	GstPad* pad;
	pad = gst_element_get_pad (self->jpegenc, "src");
	self->ghostpad = gst_ghost_pad_new ("src", pad);
	gst_element_add_pad (GST_ELEMENT (self), self->ghostpad);
	/* gst_object_ref (self->ghostpad); */
	gst_object_unref (pad);

	return;
}

static gboolean
plugin_init (GstPlugin* plugin)
{
	gboolean ret;

	ret = gst_element_register (plugin, "goophotobin",
				    GST_RANK_PRIMARY, GST_TYPE_GOO_PHOTOBIN);
	return ret;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
		   GST_VERSION_MINOR,
		   "goophotobin",
		   "Capture images from the camera in JPEG format",
		   plugin_init,
		   VERSION,
		   "LGPL",
		   GST_PACKAGE_NAME,
		   GST_PACKAGE_ORIGIN);
