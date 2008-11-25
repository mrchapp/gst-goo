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

#ifndef __GST_GOO_AUDIO_FILTER_H__
#define __GST_GOO_AUDIO_FILTER_H__

#include <goo-ti-component-factory.h>
#include "gstgooadapter.h"
#include "gstgoosem.h"

G_BEGIN_DECLS

#define GST_TYPE_GOO_AUDIO_FILTER \
     (gst_goo_audio_filter_get_type ())
#define GST_GOO_AUDIO_FILTER(obj) \
     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_GOO_AUDIO_FILTER, GstGooAudioFilter))
#define GST_GOO_AUDIO_FILTER_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_GOO_AUDIO_FILTER, GstGooAudioFilterClass))
#define GST_GOO_AUDIO_FILTER_GET_CLASS(obj) \
     (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_GOO_AUDIO_FILTER, GstGooAudioFilterClass))
#define GST_IS_GOO_AUDIO_FILTER(obj) \
     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_GOO_AUDIO_FILTER))
#define GST_IS_GOO_AUDIO_FILTER_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_GOO_AUDIO_FILTER))

typedef struct _GstGooAudioFilter GstGooAudioFilter;
typedef struct _GstGooAudioFilterClass GstGooAudioFilterClass;
typedef struct _GstGooAudioFilterPrivate GstGooAudioFilterPrivate;

struct _GstGooAudioFilter
{
	GstElement element;

	/*< protected >*/
	GooComponentFactory* factory;
	GooComponent* component;
	GooPort* inport;
	GooPort* outport;
	GstGooAdapter* adapter;
	gboolean nbamr_mime;
	gboolean wbamr_mime;
	gboolean seek_active;
	gboolean dasf_mode;
	gint64 seek_time;

	/*< protected >*/
	GstPad* sinkpad;
	GstPad* srcpad;

#if 0
	GstGooSem* dasfsrc_sem;
#endif
	GstClockTime running_time;
	GstClockTime duration;
	GstClockTime audio_timestamp;
};

struct _GstGooAudioFilterClass
{
	GstElementClass parent_class;

	gboolean (*check_fixed_src_caps_func) (GstGooAudioFilter* self);
	gboolean (*timestamp_buffer_func) (GstGooAudioFilter* self, GstBuffer *gst_buffer, OMX_BUFFERHEADERTYPE* buffer);
	GstBuffer* (*codec_data_processing_func) (GstGooAudioFilter* self, GstBuffer *buffer);

	/** FIXME GStreamer should not insert the header.  OMX component should take
 	 * care of it.  Remove this function upon resolution of DR OMAPS00140835 and
	 * OMAPS00140836 **/
	GstBuffer* (*insert_header_func) (GstGooAudioFilter* self, GstBuffer *buffer, guint counter);

	GstBuffer* (*extra_buffer_processing_func) (GstGooAudioFilter* self, GstBuffer *buffer);
	gboolean (*last_dasf_buffer_func) (GstGooAudioFilter* self, guint output_count);
	gboolean (*sink_setcaps_func) (GstPad *pad, GstCaps *caps);
	gboolean (*src_setcaps_func) (GstPad *pad, GstCaps *caps);
	gboolean (*set_process_mode_func) (GstGooAudioFilter* self, guint value);

};

GType gst_goo_audio_filter_get_type (void);
void gst_goo_audio_filter_outport_buffer (GooPort* port, OMX_BUFFERHEADERTYPE* buffer, gpointer data);

G_END_DECLS

#endif /* __GST_GOO_AUDIO_FILTER_H__ */
