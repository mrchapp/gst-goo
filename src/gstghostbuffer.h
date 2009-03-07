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

#ifndef __GST_GHOST_BUFFER_H__
#define __GST_GHOST_BUFFER_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_GHOST_BUFFER \
     (gst_ghost_buffer_get_type ())
#define GST_IS_GHOST_BUFFER(obj) \
     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_GHOST_BUFFER))
#define GST_IS_GHOST_BUFFER_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_GHOST_BUFFER))
#define GST_GHOST_BUFFER(obj) \
     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_GHOST_BUFFER, GstGhostBuffer))
#define GST_GHOST_BUFFER_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_GHOST_BUFFER, GstGhostBufferClass))

typedef struct _GstGhostBuffer GstGhostBuffer;
typedef struct _GstGhostBufferClass GstGhostBufferClass;

struct _GstGhostBuffer
{
     GstBuffer super;
     GstPadChainFunction chain;   /* the deferred chain function of the decoder */
     GstPad    *pad;
     GstBuffer *buffer;
};

struct _GstGhostBufferClass
{
     GstBufferClass buffer_class;
};

GType gst_ghost_buffer_get_type (void);
GstGhostBuffer* gst_ghost_buffer_new ();

G_END_DECLS

#endif /* __GST_GHOST_BUFFER_H__ */
