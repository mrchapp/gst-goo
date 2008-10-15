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

#ifndef __GST_GOO_FILTERVPP_H__
#define __GST_GOO_FILTERVPP_H__

#include <gst/gst.h>

#include <goo-ti-component-factory.h>

G_BEGIN_DECLS

#define GST_TYPE_GOO_FILTERVPP			\
	(gst_goo_filtervpp_get_type ())
#define GST_GOO_FILTERVPP(obj)			\
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_GOO_FILTERVPP, GstGooFilterVPP))
#define GST_GOO_FILTERVPP_CLASS(klass)		\
	(G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_GOO_FILTERVPP, GstGooFilterVPPClass))
#define GST_GOO_FILTERVPP_GET_CLASS(obj)	\
	(G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_GOO_FILTERVPP, GstGooFilterVPPClass))
#define GST_IS_GOO_FILTERVPP(obj)		\
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_GOO_FILTERVPP))
#define GST_IS_GOO_FILTERVPP_CLASS(klass)	\
	(G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_GOO_FILTERVPP))

typedef struct _GstGooFilterVPP GstGooFilterVPP;
typedef struct _GstGooFilterVPPClass GstGooFilterVPPClass;
typedef struct _GstGooFilterVPPPrivate GstGooFilterVPPPrivate;

struct _GstGooFilterVPP
{
	GstElement parent;
	GstGooFilterVPPPrivate* priv;

	GooComponentFactory* factory;
	GooComponent* component;
	GooPort* inport;
	GooPort* outport;

	GstPad* sinkpad;
	GstPad* srcpad;
};

struct _GstGooFilterVPPClass
{
	GstElementClass parent_class;
};

GType gst_goo_filtervpp_get_type (void);

G_END_DECLS

#endif /* __GST_GOO_FILTERVPP_H__ */








