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

#ifndef __GST_GOO_FILTER_H__
#define __GST_GOO_FILTER_H__

#include <goo-ti-component-factory.h>
#include "gstgooadapter.h"
#include "gstgoosem.h"

G_BEGIN_DECLS

#define GST_TYPE_GOO_FILTER \
     (gst_goo_filter_get_type ())
#define GST_GOO_FILTER(obj) \
     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_GOO_FILTER, GstGooFilter))
#define GST_GOO_FILTER_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_GOO_FILTER, GstGooFilterClass))
#define GST_GOO_FILTER_GET_CLASS(obj) \
     (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_GOO_FILTER, GstGooFilterClass))
#define GST_IS_GOO_FILTER(obj) \
     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_GOO_FILTER))
#define GST_IS_GOO_FILTER_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_GOO_FILTER))

typedef struct _GstGooFilter GstGooFilter;
typedef struct _GstGooFilterClass GstGooFilterClass;
typedef struct _GstGooFilterPrivate GstGooFilterPrivate;

struct _GstGooFilter
{
	GstElement element;

	/*< protected >*/
	GooComponentFactory* factory;
	GooComponent* component;
	GooPort* inport;
	GooPort* outport;
	GstGooAdapter* adapter;

	/*< protected >*/
	GstPad* sinkpad;
	GstPad* srcpad;

#if 0
	GstGooSem* dasfsrc_sem;
#endif
	GstClockTime running_time;
	GstClockTime duration;
};

struct _GstGooFilterClass
{
	GstElementClass parent_class;

	gboolean (*check_fixed_src_caps_func) (GstGooFilter* self);
	gboolean (*timestamp_buffer_func) (GstGooFilter* self, GstBuffer *gst_buffer, OMX_BUFFERHEADERTYPE* buffer);
	GstBuffer* (*codec_data_processing_func) (GstGooFilter* self, GstBuffer *buffer);

	/** FIXME GStreamer should not insert the header.  OMX component should take
 	 * care of it.  Remove this function upon resolution of DR OMAPS00140835 and
	 * OMAPS00140836 **/
	GstBuffer* (*insert_header_func) (GstGooFilter* self, GstBuffer *buffer, guint counter);

	GstBuffer* (*extra_buffer_processing_func) (GstGooFilter* self, GstBuffer *buffer);
	gboolean (*last_dasf_buffer_func) (GstGooFilter* self, guint output_count);
	gboolean (*sink_setcaps_func) (GstPad *pad, GstCaps *caps);
	gboolean (*src_setcaps_func) (GstPad *pad, GstCaps *caps);
	gboolean (*set_process_mode_func) (GstGooFilter* self, guint value);

};

GType gst_goo_filter_get_type (void);
void gst_goo_filter_outport_buffer (GooPort* port, OMX_BUFFERHEADERTYPE* buffer, gpointer data);

G_END_DECLS

#endif /* __GST_GOO_FILTER_H__ */
