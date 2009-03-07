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

void gst_goo_util_register_pipeline_change_cb (GstElement *elem, void (*cb) (GstElement *elem));
GooComponent* gst_goo_utils_find_goo_component (GstElement *elem, GType type);


#define GST2OMX_TIMESTAMP(ts) (OMX_S64) ts / 1000;
#define OMX2GST_TIMESTAMP(ts) (guint64) ts * 1000;


G_END_DECLS

#endif /* __GST_GOO_UTILS_H__ */
