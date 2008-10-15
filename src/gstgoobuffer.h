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

#ifndef __GST_GOO_BUFFER_H__
#define __GST_GOO_BUFFER_H__

#include <goo-component.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_GOO_BUFFER \
     (gst_goo_buffer_get_type ())
#define GST_IS_GOO_BUFFER(obj) \
     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_GOO_BUFFER))
#define GST_IS_GOO_BUFFER_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_GOO_BUFFER))
#define GST_GOO_BUFFER(obj) \
     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_GOO_BUFFER, GstGooBuffer))
#define GST_GOO_BUFFER_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_GOO_BUFFER, GstGooBufferClass))

typedef struct _GstGooBuffer GstGooBuffer;
typedef struct _GstGooBufferClass GstGooBufferClass;

struct _GstGooBuffer
{
     GstBuffer buffer;
     GooComponent* component;
     OMX_BUFFERHEADERTYPE* omx_buffer;
};

struct _GstGooBufferClass
{
     GstBufferClass buffer_class;
};

GType gst_goo_buffer_get_type (void);
GstBuffer* gst_goo_buffer_new ();
void gst_goo_buffer_set_data (GstBuffer* buffer,
                              GooComponent* component,
                              OMX_BUFFERHEADERTYPE* omx_buffer);


G_END_DECLS

#endif /* __GST_GOO_BUFFER_H__ */
