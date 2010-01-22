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

#ifndef __GST_GOO_VIDEODEC720P_H__
#define __GST_GOO_VIDEODEC720P_H__

#include <goo-ti-component-factory.h>
#include "gstgoovideofilter.h"

G_BEGIN_DECLS

#define GST_TYPE_GOO_VIDEODEC720P \
     (gst_goo_videodec720p_get_type ())
#define GST_GOO_VIDEODEC720P(obj) \
     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_GOO_VIDEODEC720P, GstGooVideoDec720p))
#define GST_GOO_VIDEODEC720P_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_GOO_VIDEODEC720P, GstGooVideoDec720pClass))
#define GST_GOO_VIDEODEC720P_GET_CLASS(obj) \
     (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_GOO_VIDEODEC720P, GstGooVideoDec720pClass))
#define GST_IS_GOO_VIDEODEC720P(obj) \
     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_GOO_VIDEODEC720P))
#define GST_IS_GOO_VIDEODEC720P_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_GOO_VIDEODEC720P))

typedef struct _GstGooVideoDec720p GstGooVideoDec720p;
typedef struct _GstGooVideoDec720pClass GstGooVideoDec720pClass;
typedef struct _GstGooVideoDec720pPrivate GstGooVideoDec720pPrivate;


struct _GstGooVideoDec720p
{
	GstGooVideoFilter parent;

	GstBuffer *video_header;


};

struct _GstGooVideoDec720pClass
{
        GstGooVideoFilterClass parent_class;
};

GType gst_goo_videodec720p_get_type (void);

G_END_DECLS

#endif /* __GST_GOO_VIDEODEC720P_H__ */







