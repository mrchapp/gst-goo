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

#include "gstghostbuffer.h"

static void
gst_ghost_buffer_class_init (gpointer g_class, gpointer class_data)
{
        return;
}

static void
gst_ghost_buffer_init (GTypeInstance* instance, gpointer g_class)
{
        GstBuffer* buf = (GstBuffer*) instance;

        GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_READONLY);
        GST_BUFFER_DATA (buf) = NULL;
        GST_BUFFER_SIZE (buf) = 0;

        return;
}

GType
gst_ghost_buffer_get_type (void)
{
        static GType type = 0;

        if (G_UNLIKELY (type == 0))
        {
                static const GTypeInfo info = {
                        sizeof (GstGhostBufferClass),
                        NULL,
                        NULL,
                        gst_ghost_buffer_class_init,
                        NULL,
                        NULL,
                        sizeof (GstGhostBuffer),
                        0,
                        gst_ghost_buffer_init,
                        NULL
                };

                type = g_type_register_static (GST_TYPE_BUFFER, "GstGhostBuffer",
                                               &info, 0);
        }

        return type;
}

GstBuffer*
gst_ghost_buffer_new ()
{
        GstBuffer* buffer = GST_BUFFER (
		gst_mini_object_new (GST_TYPE_GHOST_BUFFER)
		);

        return buffer;
}

