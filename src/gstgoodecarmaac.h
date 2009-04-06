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

#ifndef __GST_GOO_DECARMAAC_H__
#define __GST_GOO_DECARMAAC_H__

#include <gst/gst.h>
#include <goo-ti-component-factory.h>
#include <gst/audio/audio.h>
#include "gstgooadapter.h"

G_BEGIN_DECLS

#define GST_TYPE_GOO_DECARMAAC \
     (gst_goo_decarmaac_get_type ())
#define GST_GOO_DECARMAAC(obj) \
     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_GOO_DECARMAAC, GstGooDecArmAac))
#define GST_GOO_DECARMAAC_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_GOO_DECARMAAC, GstGooDecArmAacClass))
#define GST_GOO_DECARMAAC_GET_CLASS(obj) \
     (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_GOO_DECARMAAC, GstGooDecArmAacClass))
#define GST_IS_GOO_DECARMAAC(obj) \
     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_GOO_DECARMAAC))
#define GST_IS_GOO_DECARMAAC_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_GOO_DECARMAAC))

typedef struct _GstGooDecArmAac GstGooDecArmAac;
typedef struct _GstGooDecArmAacClass GstGooDecArmAacClass;

struct _GstGooDecArmAac
{
	GstElement element;

	GstPad* sinkpad;
	GstPad* srcpad;
	guint64 ts;
	guint64 prev_ts;
	gboolean val_ts;
	GstSegment segment;
	
	GstGooAdapter* adapter;

	GooComponentFactory* factory;
	GooComponent* component;
	GooPort* inport;
	GooPort* outport;

    gboolean sbr;
    gboolean bit_output;
    gboolean parametric_stereo;
	guint profile;
	guint format;
	gint channels;
	gint rate;
	gint duration;
	guint outcount;
};

struct _GstGooDecArmAacClass
{
        GstElementClass parent_class;
};

GType gst_goo_decarmaac_get_type (void);

G_END_DECLS

#endif /* __GST_GOO_DECARMAAC_H__ */
