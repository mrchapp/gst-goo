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


#ifndef __GST_GOO_UTILS_H__
#define __GST_GOO_UTILS_H__

#include <goo-ti-component-factory.h>
#include <gst/gstghostpad.h>
#include <gst/gst.h>

G_BEGIN_DECLS

void * gst_goo_util_register_pipeline_change_cb (GstElement *elem, void (*cb) (GstElement *elem));
GooComponent* gst_goo_util_find_goo_component (GstElement *elem, GType type);


#define GST2OMX_TIMESTAMP(ts) ((OMX_S64) ts / 1000)
#define OMX2GST_TIMESTAMP(ts) ((guint64) ts * 1000)


/* for some reason, if user seeks back to beginning, we don't get DISCONT flag */
#define GST_GOO_UTIL_IS_DISCONT(buffer)  (GST_BUFFER_FLAG_IS_SET ((buffer), GST_BUFFER_FLAG_DISCONT) || (GST_BUFFER_TIMESTAMP ((buffer)) == 0))

gboolean gst_goo_timestamp_gst2omx (OMX_BUFFERHEADERTYPE* omx_buffer, GstBuffer* buffer, gboolean normalize);
GstClockTime gst_goo_timestamp_omx2gst (OMX_BUFFERHEADERTYPE *omx_buffer);

GstEvent * gst_goo_event_new_reverse_eos (void);
gboolean   gst_goo_event_is_reverse_eos (GstEvent *evt);


void gst_goo_util_ensure_executing (GooComponent *component);

void gst_goo_util_post_message (GstElement* self, gchar* structure_name, GTimeVal* gotten_time);


#define PRINT_BUFFER(buffer)  \
	GST_DEBUG (#buffer "=0x%08x (time=%"GST_TIME_FORMAT", duration=%"GST_TIME_FORMAT", flags=%08x)", (buffer), GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buffer)), GST_TIME_ARGS (GST_BUFFER_DURATION(buffer)), GST_BUFFER_FLAGS (buffer));



G_END_DECLS

#endif /* __GST_GOO_UTILS_H__ */
