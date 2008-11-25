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

#ifndef __GST_GOO_VIDEO_FILTER_H__
#define __GST_GOO_VIDEO_FILTER_H__

#include <goo-ti-component-factory.h>
#include "gstgooadapter.h"
#include "gstgoosem.h"
#include <gst/gst.h>
#include <goo-ti-video-decoder.h>
#include <goo-ti-post-processor.h>
#include "gstgooutils.h"


G_BEGIN_DECLS

#define GST_TYPE_GOO_VIDEO_FILTER \
     (gst_goo_video_filter_get_type ())
#define GST_GOO_VIDEO_FILTER(obj) \
     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_GOO_VIDEO_FILTER, GstGooVideoFilter))
#define GST_GOO_VIDEO_FILTER_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_GOO_VIDEO_FILTER, GstGooVideoFilterClass))
#define GST_GOO_VIDEO_FILTER_GET_CLASS(obj) \
     (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_GOO_VIDEO_FILTER, GstGooVideoFilterClass))
#define GST_IS_GOO_VIDEO_FILTER(obj) \
     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_GOO_VIDEO_FILTER))
#define GST_IS_GOO_VIDEO_FILTER_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_GOO_VIDEO_FILTER))

typedef struct _GstGooVideoFilter GstGooVideoFilter;
typedef struct _GstGooVideoFilterClass GstGooVideoFilterClass;
typedef struct _GstGooVideoFilterPrivate GstGooVideoFilterPrivate;

struct _GstGooVideoFilter
{
	GstElement element;

	/*< protected >*/
	GooComponentFactory* factory;
	GooComponent* component;
	GooPort* inport;
	GooPort* outport;
	GstGooAdapter* adapter;
	gboolean seek_active;
	gint64 seek_time;

	/*< protected >*/
	GstPad* sinkpad;
	GstPad* srcpad;

	/* This field is used to normalize timestamps to 0 */
	OMX_S64 omx_normalize_timestamp;

	/* These fields are used to estimate
	     timestamps when not present */
	guint rate_numerator;
	guint rate_denominator;

	GstClockTime running_time;
};

/**
 * GstVideoFilterClass:
 * @transform_caps: Optional.  given the pad in this direction and the given
 *                  caps, what caps are allowed on the other pad in this
 *                  element ?
 * @fixate_caps:  Optional. Given the pad in this direction and the given
 *                  caps, fixate the caps on the other pad.
 * @set_caps: Allows the subclass to be notified of the actual caps set.
 */

struct _GstGooVideoFilterClass
{
	GstElementClass parent_class;

	GstCaps* (*transform_caps) (GstGooVideoFilter *self, GstPadDirection direction, GstCaps *caps);
	void (*fixate_caps)(GstGooVideoFilter *self, GstPadDirection direction, GstCaps *caps,
		GstCaps *othercaps);
	gboolean (*set_caps) (GstGooVideoFilter *self, GstCaps *incaps,
		GstCaps *outcaps);
	gboolean (*timestamp_buffer_func) (GstGooVideoFilter* self,
		GstBuffer *gst_buffer, OMX_BUFFERHEADERTYPE* buffer);
	GstBuffer* (*codec_data_processing_func) (GstGooVideoFilter* self, GstBuffer *buffer);
	GstBuffer* (*extra_buffer_processing_func) (GstGooVideoFilter* self, GstBuffer *buffer);
	gboolean (*set_process_mode_func) (GstGooVideoFilter* self, guint value);

};

GType gst_goo_video_filter_get_type (void);
void gst_goo_video_filter_outport_buffer (GooPort* port, OMX_BUFFERHEADERTYPE* buffer, gpointer data);

G_END_DECLS

#endif /* __GST_GOO_VIDEO_FILTER_H__ */
