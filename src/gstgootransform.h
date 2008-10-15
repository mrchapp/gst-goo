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

#ifndef __GST_GOO_TRANSFORM_H__
#define __GST_GOO_TRANSFORM_H__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>

#include <goo-component-factory.h>

G_BEGIN_DECLS

#define GST_TYPE_GOO_TRANSFORM \
     (gst_goo_transform_get_type ())
#define GST_GOO_TRANSFORM(obj) \
     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_GOO_TRANSFORM, GstGooTransform))
#define GST_GOO_TRANSFORM_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_GOO_TRANSFORM, GstGooTransformClass))
#define GST_GOO_TRANSFORM_GET_CLASS(obj) \
     (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_GOO_TRANSFORM, GstGooTransformClass))
#define GST_IS_GOO_TRANSFORM(obj) \
     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_GOO_TRANSFORM))
#define GST_IS_GOO_TRANSFORM_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_GOO_TRANSFORM))

/**
 * GST_GOO_TRANSFORM_SINK_NAME:
 *
 * the name of the templates for the sink pad
 */
#define GST_GOO_TRANSFORM_SINK_NAME	"sink"
/**
 * GST_GOO_TRANSFORM_SRC_NAME:
 *
 * the name of the templates for the source pad
 */
#define GST_GOO_TRANSFORM_SRC_NAME	"src"

/**
 * GST_GOO_TRANSFORM_SRC_PAD:
 * @obj: base transform instance
 *
 * Gives the pointer to the source #GstPad object of the element.
 */
#define GST_GOO_TRANSFORM_SRC_PAD(obj)		(GST_GOO_TRANSFORM (obj)->srcpad)

/**
 * GST_GOO_TRANSFORM_SINK_PAD:
 * @obj: base transform instance
 *
 * Gives the pointer to the sink #GstPad object of the element.
 */
#define GST_GOO_TRANSFORM_SINK_PAD(obj)	(GST_GOO_TRANSFORM (obj)->sinkpad)


typedef struct _GstGooTransform GstGooTransform;
typedef struct _GstGooTransformClass GstGooTransformClass;
typedef struct _GstGooTransformPrivate GstGooTransformPrivate;

struct _GstGooTransform
{
	GstElement element;

	/*< protected >*/
	GooComponentFactory* factory;
	GooComponent* component;
	GooPort* inport;
	GooPort* outport;
	GstAdapter* adapter;

	/*< protected >*/
	GstPad* sinkpad;
	GstPad* srcpad;
};

struct _GstGooTransformClass
{
	GstElementClass parent_class;

	gboolean (*setcaps_func) (GstPad* pad, GstCaps* caps);
};

GType gst_goo_transform_get_type (void);
void gst_goo_transform_outport_buffer (GooPort* port,
				       OMX_BUFFERHEADERTYPE* buffer,
				       gpointer data);

G_END_DECLS

#endif /* __GST_GOO_TRANSFORM_H__ */
