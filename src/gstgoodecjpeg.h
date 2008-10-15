/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

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

#ifndef __GST_GOO_DECJPEG_H__
#define __GST_GOO_DECJPEG_H__

#include <goo-ti-component-factory.h>

G_BEGIN_DECLS

#define GST_TYPE_GOO_DECJPEG			\
	(gst_goo_decjpeg_get_type ())
#define GST_GOO_DECJPEG(obj)						\
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_GOO_DECJPEG, GstGooDecJpeg))
#define GST_GOO_DECJPEG_CLASS(klass)					\
	(G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_GOO_DECJPEG, GstGooDecJpegClass))
#define GST_GOO_DECJPEG_GET_CLASS(obj)					\
	(G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_GOO_DECJPEG, GstGooDecJpegClass))
#define GST_IS_GOO_DECJPEG(obj)					\
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_GOO_DECJPEG))
#define GST_IS_GOO_DECJPEG_CLASS(klass)				\
	(G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_GOO_DECJPEG))

typedef struct _GstGooDecJpeg GstGooDecJpeg;
typedef struct _GstGooDecJpegClass GstGooDecJpegClass;
typedef struct _GstGooDecJpegPrivate GstGooDecJpegPrivate;


struct _GstGooDecJpeg
{
	GstElement element;


	/*< protected >*/
	GooComponentFactory* factory;
	GooComponent* component;
	GooPort* inport;
	GooPort* outport;

	/*< protected >*/
	GstPad* sinkpad;
	GstPad* srcpad;
};

struct _GstGooDecJpegClass
{
	GstElementClass parent_class;
};

GType gst_goo_decjpeg_get_type (void);

G_END_DECLS

#endif /* __GST_GOO_DECJPEG_H__ */
