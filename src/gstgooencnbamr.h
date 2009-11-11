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

#ifndef __GST_GOO_ENCNBAMR_H__
#define __GST_GOO_ENCNBAMR_H__

#include <gst/gst.h>
#include <goo-ti-component-factory.h>
#include "gstgooadapter.h"

G_BEGIN_DECLS

#define GST_TYPE_GOO_ENCNBAMR			\
	(gst_goo_encnbamr_get_type ())
#define GST_GOO_ENCNBAMR(obj)						\
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_GOO_ENCNBAMR, GstGooEncNbAmr))
#define GST_GOO_ENCNBAMR_CLASS(klass)					\
	(G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_GOO_ENCNBAMR, GstGooEncNbAmrClass))
#define GST_GOO_ENCNBAMR_GET_CLASS(obj)					\
	(G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_GOO_ENCNBAMR, GstGooEncNbAmrClass))
#define GST_IS_GOO_ENCNBAMR(obj)					\
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_GOO_ENCNBAMR))
#define GST_IS_GOO_ENCNBAMR_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_GOO_ENCNBAMR))

typedef struct _GstGooEncNbAmr GstGooEncNbAmr;
typedef struct _GstGooEncNbAmrClass GstGooEncNbAmrClass;

struct _GstGooEncNbAmr
{
	GstElement element;

	GstPad* sinkpad;
	GstPad* srcpad;
	guint64 ts;

	GstGooAdapter* adapter;

	GooComponentFactory* factory;
	GooComponent* component;
	GooPort* inport;
	GooPort* outport;

	OMX_AUDIO_AMRBANDMODETYPE bandmode;
	OMX_AUDIO_AMRDTXMODETYPE dtxmode;
	gboolean mime;
	gboolean mux;

	gint channels;
	gint rate;
	guint outcount;
	gint duration;

	/**
	 * Number of bytes sent to OMX since receiving a encoded buffer back.
	 * Since we know the bitrate, and other parameters, we can convert
	 * between bytes and time for raw samples.
	 *
	 * To generate back the timestamp on the encoded buffer, we assume
	 * that it contains data from all the raw buffers sent down to OMX.
	 * However this won't always be true.. so there might be some jitter
	 * in this algorithm.
	 */
	guint bytes_pending;
	guint ns_per_byte;


};

struct _GstGooEncNbAmrClass
{
        GstElementClass parent_class;
};

GType gst_goo_encnbamr_get_type (void);

G_END_DECLS

#endif /* __GST_GOO_ENCNBAMR_H__ */
