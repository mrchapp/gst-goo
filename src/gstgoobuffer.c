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

#include "gstgoobuffer.h"

GST_EXPORT GstDebugCategory *GST_CAT_BUFFER;

static void
gst_goo_buffer_finalize (GstGooBuffer* buffer)
{
	g_return_if_fail (buffer != NULL);

	GST_CAT_LOG (GST_CAT_BUFFER, "GooBuffer released %p", buffer);

	OMX_BUFFERHEADERTYPE* omx_buffer;
	GooComponent* component;

	omx_buffer = buffer->omx_buffer;
	component = buffer->component;

	goo_component_release_buffer (component, omx_buffer);

	g_object_unref (component);

	gst_caps_replace (&GST_BUFFER_CAPS (buffer), NULL);
}

static void
gst_goo_buffer_class_init (gpointer g_class, gpointer class_data)
{
	GstMiniObjectClass* mini_object_class =
		GST_MINI_OBJECT_CLASS (g_class);

	mini_object_class->finalize =
		(GstMiniObjectFinalizeFunction) gst_goo_buffer_finalize;

	return;
}

static void
gst_goo_buffer_init (GTypeInstance* instance, gpointer g_class)
{
	GstBuffer* buf = (GstBuffer*) instance;

	GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_READONLY);

	return;
}

GType
gst_goo_buffer_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
	{
		static const GTypeInfo info = {
			sizeof (GstGooBufferClass),
			NULL,
			NULL,
			gst_goo_buffer_class_init,
			NULL,
			NULL,
			sizeof (GstGooBuffer),
			0,
			gst_goo_buffer_init,
			NULL
		};

		type = g_type_register_static (GST_TYPE_BUFFER, "GstGooBuffer",
					       &info, 0);
	}

	return type;
}

GstBuffer*
gst_goo_buffer_new ()
{
	GstBuffer* buffer = GST_BUFFER (
		gst_mini_object_new (GST_TYPE_GOO_BUFFER)
		);

	return buffer;
}

void
gst_goo_buffer_set_data (GstBuffer* buffer,
			 GooComponent* component,
			 OMX_BUFFERHEADERTYPE* omx_buffer)
{
	g_assert (GST_IS_GOO_BUFFER (buffer));
	g_assert (GOO_IS_COMPONENT (component));
	g_assert (omx_buffer != NULL);

	g_object_ref (component);

	GST_GOO_BUFFER (buffer)->omx_buffer = omx_buffer;
	GST_GOO_BUFFER (buffer)->component = component;
	GST_BUFFER_DATA (buffer) = omx_buffer->pBuffer + omx_buffer->nOffset;
	GST_BUFFER_SIZE (buffer) = omx_buffer->nFilledLen;

	if (GST_CLOCK_TIME_IS_VALID ((guint64) omx_buffer->nTimeStamp))
	{
		GST_BUFFER_TIMESTAMP (buffer) =
			(guint64) omx_buffer->nTimeStamp;
	}
	else
	{
		GST_BUFFER_TIMESTAMP (buffer) = GST_CLOCK_TIME_NONE;
	}

	GST_BUFFER_DURATION (buffer) = GST_CLOCK_TIME_NONE;

	return;
}
