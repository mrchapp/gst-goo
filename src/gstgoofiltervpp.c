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

#include <goo-ti-vpp.h>

#include "gstgoofiltervpp.h"
#include "gstgoobuffer.h"
#include "gstghostbuffer.h"

GST_BOILERPLATE (GstGooFilterVPP, gst_goo_filtervpp, GstElement,
		 GST_TYPE_ELEMENT);

GST_DEBUG_CATEGORY_STATIC (gst_goo_filtervpp_debug);
#define GST_CAT_DEFAULT gst_goo_filtervpp_debug

/* signals */
enum
{
	LAST_SIGNAL
};

/* args */
enum
{
	PROP_0,
	PROP_ROTATION,
	PROP_LEFTCROP,
	PROP_TOPCROP,
	PROP_WIDTHCROP,
	PROP_HEIGHTCROP,
	PROP_ZOOMFACTOR,
	PROP_ZOOMLIMIT,
	PROP_ZOOMSPEED,
	PROP_ZOOMXOFFSET,
	PROP_ZOOMYOFFSET,
	PROP_GLASSEFFECT,
	PROP_CONTRAST
};

struct _GstGooFilterVPPPrivate
{
	guint incount;
	guint outcount;

	guint in_width;
	guint in_height;
	guint out_width;
	guint out_height;
	guint in_color;
	guint out_color;

	guint num_input_buffers;
	guint num_output_buffers;

	GstSegment segment;

	gboolean have_same_caps;
	gboolean fixed_rotated_size;
};

/* default values */
#define DEFAULT_INPUT_WIDTH   176
#define DEFAULT_INPUT_HEIGHT  144
#define DEFAULT_OUTPUT_WIDTH  176
#define DEFAULT_OUTPUT_HEIGHT 144
#define DEFAULT_FRAMERATE     15
#define DEFAULT_INPUT_COLOR_FORMAT  OMX_COLOR_FormatYUV420PackedPlanar
#define DEFAULT_OUTPUT_COLOR_FORMAT OMX_COLOR_FormatCbYCrY
#define DEFAULT_CROP_LEFT   0	/* No cropping in X axis */
#define DEFAULT_CROP_TOP    0	/* No cropping in Y axis */
#define DEFAULT_CROP_WIDTH  0	/* No cropping width */
#define DEFAULT_CROP_HEIGHT 0	/* No cropping height */
#define DEFAULT_ZOOMLIMIT   0	/* Default zoom limit */
#define DEFAULT_ZOOMSPEED   0	/* Default zoom limit */
#define DEFAULT_ZOOMXOFFSET 0	/* Default zoom offset in X axis */
#define DEFAULT_ZOOMYOFFSET 0	/* Default zoom offset in Y axis */
#define DEFAULT_GLASSEFFECT 0	/* No glass effect */
#define DEFAULT_ZOOM_FACTOR 1	/* No Zoom*/
#define DEFAULT_ZOOM_LIMIT  64	/* Maximum zoom */
#define DEFAULT_ZOOM_SPEED  0	/* No dynamic zoom */
#define DEFAULT_CONTRAST    0	/* No constrast modification */
#define DEFAULT_ROTATION    GOO_TI_VPP_ROTATION_NONE

#define DEFAULT_NUM_INPUT_BUFFERS  1
#define DEFAULT_NUM_OUTPUT_BUFFERS 1

#define GOO_TI_VPP_ROTATION \
	(goo_ti_vpp_rotation_get_type())

#define GST2OMX_TIMESTAMP(ts) (OMX_S64) (ts) / 1000;
#define OMX2GST_TIMESTAMP(ts) (guint64) ts * 1000;

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

static const GstElementDetails details =
	GST_ELEMENT_DETAILS (
		"OpenMAX VPP filter",
		"Codedc/Filter/Video",
		"Video pre-post-processes with OpenMAX",
		"Texas Instrument");

static GstStaticPadTemplate src_factory =
	GST_STATIC_PAD_TEMPLATE (
		"src",
		GST_PAD_SRC,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS (GST_VIDEO_CAPS_YUV ("{ YUY2, UYVY, I420 }") ";"
				 GST_VIDEO_CAPS_RGB_16 ";"
				 GST_VIDEO_CAPS_RGB ";"
				 GST_VIDEO_CAPS_ABGR)
		);
/*
static GstStaticPadTemplate src_factory =
	GST_STATIC_PAD_TEMPLATE (
		"src",
		GST_PAD_SRC,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS (GST_VIDEO_CAPS_YUV ("{ YUY2, UYVY, I420 }"))
		);
*/
static GstStaticPadTemplate sink_factory =
	GST_STATIC_PAD_TEMPLATE (
		"sink",
		GST_PAD_SINK,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS (GST_VIDEO_CAPS_YUV ("{ YUY2, UYVY, I420 }") ";"
				 GST_VIDEO_CAPS_RGB_16 ";"
				 GST_VIDEO_CAPS_RGB ";"
				 GST_VIDEO_CAPS_ABGR)
		);

static gboolean
omx_sync (GstGooFilterVPP* self, GstCaps* caps)
{
	//guint32 fourcc;
	GstStructure *structure;
	structure = gst_caps_get_structure (caps, 0);

	g_assert (self != NULL);

	GstGooFilterVPPPrivate* priv = self->priv;
	g_assert (self->priv != NULL);

	g_assert (self->component != NULL);
	g_assert (self->inport    != NULL);
	g_assert (self->outport   != NULL);

	/* inport */
	{
		g_object_set (self->inport,
			      "buffercount", priv->num_input_buffers, NULL);

		OMX_PARAM_PORTDEFINITIONTYPE* param =
			GOO_PORT_GET_DEFINITION (self->inport);

		param->format.video.nFrameWidth = priv->in_width;
		param->format.video.nFrameHeight = priv->in_height;
		param->format.video.eColorFormat = priv->in_color;
	}

	/* overlay port */
	{
		GooPort* port = goo_component_get_port (self->component,
							"input1");
		g_assert (port != NULL);

		goo_component_disable_port (self->component, port);

		g_object_unref (G_OBJECT (port));
	}

	/* rbg output port */
	{
		if (g_strrstr (gst_structure_get_name (structure), "video/x-raw-yuv"))
		{
			GooPort* port = goo_component_get_port (self->component,
								"output0");
			self->outport = goo_component_get_port (self->component,
							"output1");
			g_assert (port != NULL);

			goo_component_disable_port (self->component, port);

			g_object_unref (G_OBJECT (port));
		}

		else if(g_strrstr (gst_structure_get_name (structure), "video/x-raw-rgb"))
		{
			GooPort* port = goo_component_get_port (self->component,
								"output1");
			self->outport = goo_component_get_port (self->component,
							"output0");
			g_assert (port != NULL);

			goo_component_disable_port (self->component, port);

			g_object_unref (G_OBJECT (port));
		}
	}

	/* outport */
	{
		g_object_set (self->outport,
			      "buffercount", priv->num_output_buffers, NULL);

		OMX_PARAM_PORTDEFINITIONTYPE* param =
			GOO_PORT_GET_DEFINITION (self->outport);

		param->format.video.nFrameWidth = priv->out_width;
		param->format.video.nFrameHeight = priv->out_height;
		param->format.video.eColorFormat = priv->out_color;
	}

	GST_DEBUG ("");

	return TRUE;
}

static gboolean
omx_start (GstGooFilterVPP* self, GstCaps* out)
{
	g_assert (self != NULL);
	g_assert (self->component != NULL);

	GST_OBJECT_LOCK (self);
	if (goo_component_get_state (self->component) == OMX_StateLoaded)
	{
		omx_sync (self, out);

		GST_INFO_OBJECT (self, "going to idle");
		goo_component_set_state_idle (self->component);
	}

	if (goo_component_get_state (self->component) == OMX_StateIdle)
	{
		GST_INFO_OBJECT (self, "going to executing");
		goo_component_set_state_executing (self->component);
	}
	GST_OBJECT_UNLOCK (self);

	return TRUE;
}

static gboolean
omx_stop (GstGooFilterVPP* self)
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
parse_caps (GstCaps* caps, gint* format, gint* width, gint* height)
{
	gboolean ret;
	GstStructure *structure;
	guint32 fourcc;

	structure = gst_caps_get_structure (caps, 0);
	ret = gst_structure_get_int (structure, "width", width);
	ret &= gst_structure_get_int (structure, "height", height);

	*format = -1;
	if (g_strrstr (gst_structure_get_name (structure), "video/x-raw-yuv"))
	{
		if (gst_structure_get_fourcc (structure, "format", &fourcc))
		{
			switch (fourcc)
			{
			case GST_MAKE_FOURCC ('Y', 'U', 'Y', '2'):
				*format = OMX_COLOR_FormatYCbYCr;
				break;
			case GST_MAKE_FOURCC ('I', '4', '2', '0'):
				*format = OMX_COLOR_FormatYUV420PackedPlanar;
				break;
			case GST_MAKE_FOURCC ('U', 'Y', 'V', 'Y'):
				*format = OMX_COLOR_FormatCbYCrY;
				break;
			default:
				break;
			}
		}
	}
	else if (g_strrstr (gst_structure_get_name (structure),
			    "video/x-raw-rgb"))
	{
		gint bpp;
		if (gst_structure_get_int (structure, "bpp", &bpp))
		{
			switch (bpp)
			{
			case 16:
				*format = OMX_COLOR_Format16bitRGB565;
				break;
			case 24:
				*format = OMX_COLOR_Format24bitRGB888;
				break;
			case 32:
				*format = OMX_COLOR_Format32bitARGB8888;
				break;
			default:
				return FALSE;
			}
		}
	}


	if (*format == -1)
	{
		ret = FALSE;
	}

	return ret;
}

static GstCaps*
gst_goo_filtervpp_transformcaps (GstGooFilterVPP* self,
				 GstPadDirection direction, GstCaps* caps)
{
	const GstCaps* template = NULL;
	GstCaps* result = NULL;
	GstStructure *ins, *outs;

	GST_DEBUG_OBJECT (self, "");

	/* this function is always called with a simple caps */
	g_return_val_if_fail (GST_CAPS_IS_SIMPLE (caps), NULL);

	if (direction == GST_PAD_SINK)
	{
		template = gst_pad_get_pad_template_caps (self->srcpad);
		result = gst_caps_copy (template);
	}
	else if (direction == GST_PAD_SRC)
	{
		template = gst_pad_get_pad_template_caps (self->sinkpad);
		result = gst_caps_copy (template);
	}

	/* we only force the same framerate */
	ins = gst_caps_get_structure (caps, 0);
	outs = gst_caps_get_structure (result, 0);

	gint num, den;
	gst_structure_get_fraction (ins, "framerate", &num, &den);

	gst_structure_set (outs, "framerate",
			   GST_TYPE_FRACTION, num, den, NULL);

	/* validate rotation: only accepts I420 output */
	gint rotation;
	g_object_get (self->component, "rotation", &rotation, NULL);

	if (rotation != GOO_TI_VPP_ROTATION_NONE)
	{
		gst_structure_set (outs, "format",
				   GST_TYPE_FOURCC,
				   GST_MAKE_FOURCC ('I', '4', '2', '0'), NULL);
	}

	DEBUG_CAPS ("transformed caps %s", result);

	return result;

}

static void
gst_goo_filtervpp_fixatecaps (GstGooFilterVPP* self, GstPadDirection direction,
			      GstCaps* caps, GstCaps* othercaps)
{
	GstStructure *ins, *outs;

	g_return_if_fail (gst_caps_is_fixed (caps));

	ins = gst_caps_get_structure (caps, 0);
	outs = gst_caps_get_structure (othercaps, 0);

	gint from_w, from_h, from_n, from_d;
	gint count = 0, w = 0, h = 0;

	if (gst_structure_get_int (outs, "width", &w))
	{
		++count;
	}

	if (gst_structure_get_int (outs, "height", &h))
	{
		++count;
	}

	if (count == 2)
	{
		GST_DEBUG ("dimensions already set to %dx%d, not fixating",
			   w, h);
		return;
	}

	gst_structure_get_int (ins, "width", &from_w);
	gst_structure_get_int (ins, "height", &from_h);

	if (h)
	{
		GST_DEBUG ("height is fixed, scaling width");
		w = from_w;
	}
	else if (w)
	{
		GST_DEBUG ("width is fixed, scaling height");
		h = from_h;
	}
	else
	{
		/* none of width or height is fixed */
		w = from_w;
		h = from_h;
	}

	/* validate rotation: change the image size if no rescaling */
	gint rotation;
	g_object_get (self->component, "rotation", &rotation, NULL);

	if (rotation == GOO_TI_VPP_ROTATION_90 ||
	    rotation == GOO_TI_VPP_ROTATION_270)
	{
		gint wt = w;
		w = h;
		h = wt;
		self->priv->fixed_rotated_size = TRUE;
	}

	GST_DEBUG ("scaling to %dx%d", w, h);

	/* now fixate */
	gst_structure_fixate_field_nearest_int (outs, "width", w);
	gst_structure_fixate_field_nearest_int (outs, "height", h);

	DEBUG_CAPS ("fixated othercaps to %s", othercaps);

	return;
}

static gboolean
gst_goo_filtervpp_configure_caps (GstGooFilterVPP* self,
				  GstCaps* in, GstCaps* out)
{
	GstGooFilterVPPPrivate* priv = self->priv;
	gboolean ret;

	GST_DEBUG ("");

	g_return_val_if_fail (GST_CAPS_IS_SIMPLE (in), FALSE);
	g_return_val_if_fail (GST_CAPS_IS_SIMPLE (out), FALSE);

	 /* input capabilities */
	ret = parse_caps (in, &priv->in_color,
			  &priv->in_width, &priv->in_height);

	/* outpor capabilities */
	ret &= parse_caps (out, &priv->out_color,
			   &priv->out_width, &priv->out_height);

	/* OMX glinch: in rotation, change the image size */
	gint rotation;
	g_object_get (self->component, "rotation", &rotation, NULL);

	if (priv->fixed_rotated_size == TRUE ||
	    (rotation != GOO_TI_VPP_ROTATION_NONE &&
	     rotation != GOO_TI_VPP_ROTATION_180))
	{
		gint wt = priv->out_width;
		priv->out_width = priv->out_height;
		priv->out_height = wt;
	}

	GST_DEBUG ("In %d-%d,%d -> Out %d-%d,%d",
		   priv->in_color, priv->in_width, priv->in_height,
		   priv->out_color, priv->out_width, priv->out_height);
	if (!ret)
	{
		goto done;
	}

	ret &= omx_start (self, out); /* let's begin */

done:
	return ret;
}

/* this is a blatant copy of GstBaseTransform setcasp function */
static gboolean
gst_goo_filtervpp_setcaps (GstPad* pad, GstCaps* caps)
{
	GstGooFilterVPP* self;
	GstGooFilterVPPPrivate* priv;
	GstPad* otherpad;
	GstPad* otherpeer;
	GstCaps* othercaps = NULL;
	gboolean ret = TRUE;
	gboolean peer_checked = FALSE;

	GST_DEBUG ("");

	self = GST_GOO_FILTERVPP (gst_pad_get_parent (pad));
	priv = self->priv;

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

	othercaps = gst_goo_filtervpp_transformcaps
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
		gst_goo_filtervpp_fixatecaps (self, GST_PAD_DIRECTION (pad),
					      caps, othercaps);
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

	priv->have_same_caps = gst_caps_is_equal (caps, othercaps);
	GST_DEBUG ("have_same_caps: %d", priv->have_same_caps);

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

	if (!(ret = gst_goo_filtervpp_configure_caps (self, incaps, outcaps)))
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

static GstFlowReturn
gst_goo_filtervpp_chain (GstPad* pad, GstBuffer* buffer)
{
	GstGooFilterVPP* self = GST_GOO_FILTERVPP (gst_pad_get_parent (pad));
	GstGooFilterVPPPrivate* priv = self->priv;

	GstFlowReturn ret = GST_FLOW_OK;
	OMX_BUFFERHEADERTYPE* omx_buffer = NULL;
	GstBuffer* outbuf = NULL;
	gint size;

	GstClockTime timestamp, duration;
	guint64 offset, offsetend;

	if (self->component->cur_state != OMX_StateExecuting)
	{
		goto not_negotiated;
	}

	if (goo_port_is_tunneled (self->inport))
	{
		GST_INFO ("input port is tunneled");
		gst_buffer_unref (buffer);
		goto process_output;
	}

	if (goo_port_is_eos (self->inport))
	{
		GST_INFO ("port is eos");
		ret = GST_FLOW_UNEXPECTED;
		gst_buffer_unref (buffer);
		goto done;
	}

	/**** STATE PROCESSING ***********************************************/
	if (GST_BUFFER_SIZE (buffer) == 0)
	{
		GST_INFO ("zero sized frame");
		gst_buffer_unref (buffer);
		ret = GST_FLOW_OK;
		goto done;
	}

	/* input buffer sanity test */
	gint omxbufsiz;
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

	/* let's copy the timestamp meta data */
	timestamp = GST_BUFFER_TIMESTAMP (buffer);
	duration  = GST_BUFFER_DURATION (buffer);
	offset	  = GST_BUFFER_OFFSET (buffer);
	offsetend = GST_BUFFER_OFFSET_END (buffer);

	/**** INPUT PORT *****************************************************/
	GST_DEBUG ("Pushing data to OMX");

	/* if the incoming buffer is not allocated by us, we must copy it */
	if (!(GST_IS_GOO_BUFFER (buffer) &&
	      goo_port_is_my_buffer (self->inport,
				     GST_GOO_BUFFER (buffer)->omx_buffer)))
	{
		omx_buffer = goo_port_grab_buffer (self->inport);
		size = MIN (omxbufsiz, GST_BUFFER_SIZE (buffer));
		memmove (omx_buffer->pBuffer, GST_BUFFER_DATA (buffer), size);
		omx_buffer->nFilledLen = size;
		omx_buffer->nTimeStamp = OMX2GST_TIMESTAMP (timestamp);
		GST_DEBUG ("memcpy data, nFilledLen %ld",
			   omx_buffer->nFilledLen);
		goo_component_release_buffer (self->component, omx_buffer);
	}

	/* if it is a goobuffer we must get ride of it as quick as possible */
	gst_buffer_unref (buffer);
	priv->incount++;

	/**** YUV OUTPUT PORT ************************************************/
process_output:
	if (goo_port_is_tunneled (self->outport))
	{
		outbuf = gst_ghost_buffer_new ();
		GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_READONLY);
		GST_BUFFER_DATA (outbuf)       = NULL;
		GST_BUFFER_SIZE (outbuf)       = 0;
		GST_BUFFER_TIMESTAMP (outbuf)  = timestamp;
		GST_BUFFER_DURATION (outbuf)   = duration;
		GST_BUFFER_OFFSET (outbuf)     = offset;
		GST_BUFFER_OFFSET_END (outbuf) = offsetend;
		gst_buffer_set_caps (buffer, GST_PAD_CAPS (self->srcpad));
		gst_pad_push (self->srcpad, buffer);
		ret = GST_FLOW_OK;
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

			goo_component_release_buffer (self->component,
						      omx_buffer);
		}
		else
		{
			gst_buffer_unref (outbuf);
			outbuf = GST_BUFFER (gst_goo_buffer_new ());
			gst_goo_buffer_set_data (outbuf,
						 self->component,
						 omx_buffer);
		}

		GST_GOO_BUFFER (outbuf)->omx_buffer->nTimeStamp =
			GST2OMX_TIMESTAMP (timestamp);
		GST_BUFFER_TIMESTAMP (outbuf)  = timestamp;
		GST_BUFFER_DURATION (outbuf)   = duration;
		GST_BUFFER_OFFSET (outbuf)     = offset;
		GST_BUFFER_OFFSET_END (outbuf) = offsetend;

		gst_buffer_set_caps (outbuf, GST_PAD_CAPS (self->srcpad));
		ret = gst_pad_push (self->srcpad, outbuf);

		if (omx_buffer->nFlags == OMX_BUFFERFLAG_EOS ||
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

done:
	GST_DEBUG ("");
	gst_object_unref (self);
	return ret;

	/* ERRORS */
not_negotiated:
	{
		GST_ELEMENT_ERROR (self, CORE, NEGOTIATION, (NULL),
				   ("failed to negotiate caps"));
		gst_buffer_unref (buffer);
		ret = GST_FLOW_ERROR;
		goto done;
	}

}

static GstFlowReturn
gst_goo_filtervpp_buffer_alloc (GstPad *pad, guint64 offset, guint size,
				GstCaps *caps, GstBuffer **buf)
{
	GstGooFilterVPP* self = GST_GOO_FILTERVPP (gst_pad_get_parent (pad));
	GstGooFilterVPPPrivate* priv = self->priv;

	GstFlowReturn ret = GST_FLOW_OK;

	GST_DEBUG ("a buffer of %d bytes was requested with caps %"
		   GST_PTR_FORMAT " and offset %llu", size, caps, offset);

	if (self->component->cur_state != OMX_StateExecuting)
	{
		gst_goo_filtervpp_setcaps (pad, caps);
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

static gboolean
gst_goo_filtervpp_sink_event (GstPad* pad, GstEvent* event)
{
	GstGooFilterVPP* self = GST_GOO_FILTERVPP (gst_pad_get_parent (pad));
	GstGooFilterVPPPrivate* priv = self->priv;
	gboolean ret = TRUE;

	switch (GST_EVENT_TYPE (event))
	{
	case GST_EVENT_FLUSH_START:
		break;
	case GST_EVENT_FLUSH_STOP:
		/* reset QoS parameters */
		/* we need new segment info after the flush. */
		gst_segment_init (&self->priv->segment, GST_FORMAT_UNDEFINED);
		break;
	case GST_EVENT_EOS:
		break;
	case GST_EVENT_TAG:
		break;
	case GST_EVENT_NEWSEGMENT:
	{
		GstFormat format;
		gdouble rate, arate;
		gint64 start, stop, time;
		gboolean update;

		gst_event_parse_new_segment_full (event,
						  &update, &rate, &arate,
						  &format, &start, &stop,
						  &time);

		gst_segment_set_newsegment_full (&self->priv->segment,
						 update, rate, arate,
						 format, start, stop, time);

		if (format == GST_FORMAT_TIME)
		{
			GST_DEBUG_OBJECT
				(self,
				 "received TIME NEW_SEGMENT %" GST_TIME_FORMAT
				 " -- %" GST_TIME_FORMAT
				 ", time %" GST_TIME_FORMAT
				 ", accum %" GST_TIME_FORMAT,
				 GST_TIME_ARGS (self->priv->segment.start),
				 GST_TIME_ARGS (self->priv->segment.stop),
				 GST_TIME_ARGS (self->priv->segment.time),
				 GST_TIME_ARGS (self->priv->segment.accum));
		}
		else
		{
			GST_DEBUG_OBJECT
				(self,
				 "received NEW_SEGMENT %" G_GINT64_FORMAT
				 " -- %" G_GINT64_FORMAT
				 ", time %" G_GINT64_FORMAT
				 ", accum %" G_GINT64_FORMAT,
				 self->priv->segment.start,
				 self->priv->segment.stop,
				 self->priv->segment.time,
				 self->priv->segment.accum);
		}
		break;
	}
	default:
		break;
	}

	ret = gst_pad_push_event (self->srcpad, event);

	gst_object_unref (self);
	return ret;
}

static GstStateChangeReturn
gst_goo_filtervpp_change_state (GstElement* element, GstStateChange transition)
{
	GST_LOG ("");

	GstGooFilterVPP* self = GST_GOO_FILTERVPP (element);
	GstStateChangeReturn result;
	GstGooFilterVPPPrivate* priv = self->priv;

	g_assert (self->component != NULL);
	g_assert (self->inport != NULL);
	g_assert (self->outport != NULL);

	switch (transition)
	{
	case GST_STATE_CHANGE_NULL_TO_READY:
		break;
	case GST_STATE_CHANGE_READY_TO_PAUSED:
		GST_OBJECT_LOCK (self);
		priv->incount = 0;
		priv->outcount = 0;
		priv->in_width = DEFAULT_INPUT_WIDTH;
		priv->in_height = DEFAULT_INPUT_HEIGHT;
		priv->in_color = DEFAULT_INPUT_COLOR_FORMAT;
		priv->out_width = DEFAULT_OUTPUT_WIDTH;
		priv->out_height = DEFAULT_OUTPUT_HEIGHT;
		priv->out_color = DEFAULT_OUTPUT_COLOR_FORMAT;
		priv->fixed_rotated_size = FALSE;
		priv->have_same_caps = FALSE;
		GST_OBJECT_UNLOCK (self);
		gst_segment_init (&priv->segment, GST_FORMAT_UNDEFINED);
		break;
	case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
		GST_OBJECT_LOCK (self);
		goo_component_set_state_executing (self->component);
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
		break;
	case GST_STATE_CHANGE_READY_TO_NULL:
		omx_stop (self);
		break;
	default:
		break;
	}

	return result;
}

static void
gst_goo_filtervpp_set_property (GObject* object, guint prop_id,
				const GValue* value, GParamSpec* pspec)
{
	GstGooFilterVPP* self = GST_GOO_FILTERVPP (object);
	GooComponent* component = self->component;

	switch (prop_id)
	{
	case PROP_CONTRAST:
		g_object_set_property (G_OBJECT (component),
				       "contrast", value);
		break;
	case PROP_ROTATION:
		g_object_set_property (G_OBJECT (component),
				       "rotation", value);
		break;
	case PROP_LEFTCROP:
		g_object_set_property (G_OBJECT (component),
				       "crop-left", value);
		break;
	case PROP_TOPCROP:
		g_object_set_property (G_OBJECT (component),
				       "crop-top", value);
		break;
	case PROP_WIDTHCROP:
		g_object_set_property (G_OBJECT (component),
				       "crop-width", value);
		break;
	case PROP_HEIGHTCROP:
		g_object_set_property (G_OBJECT (component),
				       "crop-height", value);
		break;
	case PROP_ZOOMFACTOR:
		g_object_set_property (G_OBJECT (component),
				       "zoom-factor", value);
		break;
	case PROP_ZOOMLIMIT:
		g_object_set_property (G_OBJECT (component),
				       "zoom-limit", value);
		break;
	case PROP_ZOOMSPEED:
		g_object_set_property (G_OBJECT (component),
				       "zoom-speed", value);
		break;
	case PROP_ZOOMXOFFSET:
		g_object_set_property (G_OBJECT (component),
				       "zoom-xoffset", value);
		break;
	case PROP_ZOOMYOFFSET:
		g_object_set_property (G_OBJECT (component),
				       "zoom-yoffset", value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_filtervpp_get_property (GObject* object, guint prop_id,
				GValue* value, GParamSpec* pspec)
{
	GstGooFilterVPP* self = GST_GOO_FILTERVPP (object);
	GooComponent* component = self->component;

	switch (prop_id)
	{
	case PROP_CONTRAST:
		g_object_get_property (G_OBJECT (component),
				       "contrast", value);
		break;
	case PROP_ROTATION:
		g_object_get_property (G_OBJECT (component),
				       "rotation", value);
		break;
	case PROP_LEFTCROP:
		g_object_get_property (G_OBJECT (component),
				       "crop-left", value);
		break;
	case PROP_TOPCROP:
		g_object_get_property (G_OBJECT (component),
				       "crop-top", value);
		break;
	case PROP_WIDTHCROP:
		g_object_get_property (G_OBJECT (component),
				       "crop-width", value);
		break;
	case PROP_HEIGHTCROP:
		g_object_get_property (G_OBJECT (component),
				       "crop-height", value);
		break;
	case PROP_ZOOMFACTOR:
		g_object_get_property (G_OBJECT (component),
				       "zoom-factor", value);
		break;
	case PROP_ZOOMLIMIT:
		g_object_get_property (G_OBJECT (component),
				       "zoom-limit", value);
		break;
	case PROP_ZOOMSPEED:
		g_object_get_property (G_OBJECT (component),
				       "zoom-speed", value);
		break;
	case PROP_ZOOMXOFFSET:
		g_object_get_property (G_OBJECT (component),
				       "zoom-xoffset", value);
		break;
	case PROP_ZOOMYOFFSET:
		g_object_get_property (G_OBJECT (component),
				       "zoom-yoffset", value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_goo_filtervpp_dispose (GObject* object)
{
	GstGooFilterVPP* self = GST_GOO_FILTERVPP (object);

	G_OBJECT_CLASS (parent_class)->dispose (object);

	if (G_LIKELY (self->inport != NULL))
	{
		GST_DEBUG ("unrefing inport");
		g_object_unref (self->inport);
	}

	if (G_LIKELY (self->outport != NULL))
	{
		GST_DEBUG ("unrefing outport");
		g_object_unref (self->outport);
	}

	if (G_LIKELY (self->component != NULL))
	{
		GST_DEBUG ("unrefing component");
		G_OBJECT(self->component)->ref_count = 1;
		g_object_unref (self->component);
	}

	if (G_LIKELY (self->factory != NULL))
	{
		GST_DEBUG ("unrefing factory");
		g_object_unref (self->factory);
	}

	return;
}

static void
gst_goo_filtervpp_finalize (GObject* object)
{
	GstGooFilterVPP* self = GST_GOO_FILTERVPP (object);

	if (G_LIKELY (self->priv != NULL))
	{
		g_free (self->priv);
		self->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);

	return;
}

static void
gst_goo_filtervpp_base_init (gpointer g_klass)
{
	GST_DEBUG_CATEGORY_INIT (gst_goo_filtervpp_debug, "goofiltervpp", 0,
				 "OpenMAX Video Pre-PostProcessor element");

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
gst_goo_filtervpp_class_init (GstGooFilterVPPClass* klass)
{
	/* GOBJECT */
	GObjectClass* g_klass = G_OBJECT_CLASS (klass);
	g_klass->set_property =
		GST_DEBUG_FUNCPTR (gst_goo_filtervpp_set_property);
	g_klass->get_property =
		GST_DEBUG_FUNCPTR (gst_goo_filtervpp_get_property);
	g_klass->dispose = GST_DEBUG_FUNCPTR (gst_goo_filtervpp_dispose);
	g_klass->finalize = GST_DEBUG_FUNCPTR (gst_goo_filtervpp_finalize);

	GParamSpec* spec = NULL;
	spec = g_param_spec_enum ("rotation", "Rotation", "Frame rotation",
				  GOO_TI_VPP_ROTATION,
				  DEFAULT_ROTATION, G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_ROTATION, spec);

	spec = g_param_spec_uint ("crop-left", "Crop Left",
				  "Sets the left cropping",
				  0, G_MAXUINT, DEFAULT_CROP_LEFT,
				  G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_LEFTCROP, spec);

	spec = g_param_spec_uint ("crop-width", "Crop Width",
				  "Sets the width to crop",
				  0, G_MAXUINT, DEFAULT_CROP_WIDTH,
				  G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_WIDTHCROP, spec);

	spec = g_param_spec_uint ("crop-top", "Crop top",
				  "Sets the top cropping",
				  0, G_MAXUINT, DEFAULT_CROP_TOP,
				  G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_TOPCROP, spec);

	spec = g_param_spec_uint ("crop-height", "Crop Height",
				  "Sets the height to crop",
				  0, G_MAXUINT, DEFAULT_CROP_HEIGHT,
				  G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_HEIGHTCROP, spec);

	spec = g_param_spec_uint ("zoom-factor", "Zoom Factor",
				  "Sets the zoom factor",
				  0, G_MAXUINT, DEFAULT_ZOOM_FACTOR,
				  G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_ZOOMFACTOR, spec);

	spec = g_param_spec_uint ("zoom-limit", "Zoom Limit",
				  "Sets the magnification limit",
				  0, G_MAXUINT, DEFAULT_ZOOM_LIMIT,
				  G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_ZOOMLIMIT, spec);

	spec = g_param_spec_uint ("zoom-speed", "Zoom Speed",
				  "Sets the dynamic magnification speed",
				  0, G_MAXUINT, DEFAULT_ZOOM_SPEED,
				  G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_ZOOMSPEED, spec);

	spec = g_param_spec_uint ("zoom-xoffset", "Zoom x offset",
				  "Sets the magnification offset on the X axis",
				  0, G_MAXUINT, DEFAULT_ZOOMXOFFSET,
				  G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_ZOOMXOFFSET, spec);

	spec = g_param_spec_uint ("zoom-yoffset", "Zoom y offset",
				  "Sets the magnification offset on the Y axis",
				  0, G_MAXUINT, DEFAULT_ZOOMYOFFSET,
				  G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_ZOOMYOFFSET, spec);

	spec = g_param_spec_int ("contrast", "Contrast",
				 "Sets the value of the contrast",
				 -100, 100, DEFAULT_CONTRAST,
				 G_PARAM_READWRITE);
	g_object_class_install_property (g_klass, PROP_CONTRAST, spec);

	/* GST VIDEO FILTER */
	GstElementClass *gst_klass = GST_ELEMENT_CLASS (klass);
	gst_klass->change_state =
		GST_DEBUG_FUNCPTR (gst_goo_filtervpp_change_state);

	return;
}

static void
gst_goo_filtervpp_init (GstGooFilterVPP* self, GstGooFilterVPPClass* klass)
{
	{
		self->priv = g_new0 (GstGooFilterVPPPrivate, 1);
		g_assert (self->priv != NULL);

		self->priv->incount = 0;
		self->priv->outcount = 0;
		self->priv->in_width = DEFAULT_INPUT_WIDTH;
		self->priv->in_height = DEFAULT_INPUT_HEIGHT;
		self->priv->in_color = DEFAULT_INPUT_COLOR_FORMAT;
		self->priv->out_width = DEFAULT_OUTPUT_WIDTH;
		self->priv->out_height = DEFAULT_OUTPUT_HEIGHT;
		self->priv->out_color = DEFAULT_OUTPUT_COLOR_FORMAT;
		self->priv->num_input_buffers = DEFAULT_NUM_INPUT_BUFFERS;
		self->priv->num_output_buffers = DEFAULT_NUM_OUTPUT_BUFFERS;
		self->priv->fixed_rotated_size = FALSE;
		self->priv->have_same_caps = FALSE;
	}

	/* component creation */
	{
		self->factory = goo_ti_component_factory_get_instance ();
		g_assert (self->factory != NULL);

		self->component = goo_component_factory_get_component
			(self->factory, GOO_TI_VPP);
		g_assert (self->component != NULL);
	}

	/* inport */
	{
		self->inport = goo_component_get_port (self->component,
						       "input0");
		g_assert (self->inport != NULL);
	}

	/* outport */
	{
		self->outport = goo_component_get_port (self->component,
							"output0");
		g_assert (self->outport != NULL);
	}


	g_object_set_data (G_OBJECT (self->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", self->component);

	/* GST */
	GstPadTemplate* pad_template;

	pad_template = gst_element_class_get_pad_template
		(GST_ELEMENT_CLASS (klass), "sink");
	g_return_if_fail (pad_template != NULL);
	self->sinkpad = gst_pad_new_from_template (pad_template, "sink");
	gst_pad_set_chain_function (self->sinkpad,
				    GST_DEBUG_FUNCPTR (gst_goo_filtervpp_chain));
	gst_pad_set_setcaps_function (self->sinkpad,
				      GST_DEBUG_FUNCPTR (gst_goo_filtervpp_setcaps));
	gst_pad_set_event_function (self->sinkpad,
				    GST_DEBUG_FUNCPTR (gst_goo_filtervpp_sink_event));
/* 	gst_pad_set_bufferalloc_function (self->sinkpad, */
/* 					  GST_DEBUG_FUNCPTR (gst_goo_filtervpp_buffer_alloc)); */
	gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);

	pad_template = gst_element_class_get_pad_template (GST_ELEMENT_CLASS (klass), "src");
	g_return_if_fail (pad_template != NULL);
	self->srcpad = gst_pad_new_from_template (pad_template, "src");
	/* gst_pad_set_setcaps_function
	   (self->esrcpad, GST_DEBUG_FUNCPTR (gst_goo_filtervpp_setcaps)); */
	gst_element_add_pad (GST_ELEMENT (self), self->srcpad);

	return;
}
