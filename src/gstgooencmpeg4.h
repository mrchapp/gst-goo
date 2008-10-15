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

#ifndef __GST_GOO_ENCMPEG4_H__
#define __GST_GOO_ENCMPEG4_H__

#include <goo-ti-component-factory.h>
#include <goo-ti-mpeg4enc.h>
#include "gstgoovideoenc.h"


G_BEGIN_DECLS

#define GST_TYPE_GOO_ENCMPEG4 \
     (gst_goo_encmpeg4_get_type ())
#define GST_GOO_ENCMPEG4(obj) \
     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_GOO_ENCMPEG4, GstGooEncMpeg4))
#define GST_GOO_ENCMPEG4_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_GOO_ENCMPEG4, GstGooEncMpeg4Class))
#define GST_GOO_ENCMPEG4_GET_CLASS(obj) \
     (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_GOO_ENCMPEG4, GstGooEncMpeg4Class))
#define GST_IS_GOO_ENCMPEG4(obj) \
     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_GOO_ENCMPEG4))
#define GST_IS_GOO_ENCMPEG4_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_GOO_ENCMPEG4))

typedef struct _GstGooEncMpeg4 GstGooEncMpeg4;
typedef struct _GstGooEncMpeg4Class GstGooEncMpeg4Class;
typedef struct _GstGooEncMpeg4Private GstGooEncMpeg4Private;


struct _GstGooEncMpeg4
{
	GstGooVideoEnc parent;
};

struct _GstGooEncMpeg4Class
{
        GstGooVideoEncClass parent_class;
};

GType gst_goo_encmpeg4_get_type (void);

G_END_DECLS

#endif /* __GST_GOO_ENCMPEG4_H__ */








