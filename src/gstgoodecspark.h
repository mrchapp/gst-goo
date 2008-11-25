/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* =====================================================================
 *                Texas Instruments OMAP(TM) Platform Software
 *             Copyright (c) 2005 Texas Instruments, Incorporated
 *
 * Use of this software is controlled by the terms and conditions found
 * in the license agreement under which this software has been supplied.
 * ===================================================================== */

#ifndef __GST_GOO_DECSPARK_H__
#define __GST_GOO_DECSPARK_H__

#include "gstgoovideodec.h"

G_BEGIN_DECLS

#define GST_TYPE_GOO_DECSPARK \
     (gst_goo_decspark_get_type ())
#define GST_GOO_DECSPARK(obj) \
     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_GOO_DECSPARK, GstGooDecSpark))
#define GST_GOO_DECSPARK_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_GOO_DECSPARK, GstGooDecSparkClass))
#define GST_GOO_DECSPARK_GET_CLASS(obj) \
     (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_GOO_DECSPARK, GstGooDecSparkClass))
#define GST_IS_GOO_DECSPARK(obj) \
     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_GOO_DECSPARK))
#define GST_IS_GOO_DECSPARK_CLASS(klass) \
     (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_GOO_DECSPARK))

typedef struct _GstGooDecSpark GstGooDecSpark;
typedef struct _GstGooDecSparkClass GstGooDecSparkClass;
typedef struct _GstGooDecSparkPrivate GstGooDecSparkPrivate;


struct _GstGooDecSpark
{
        GstGooVideoDec parent;

};

struct _GstGooDecSparkClass
{
        GstGooVideoDecClass parent_class;
};

GType gst_goo_decspark_get_type (void);

G_END_DECLS

#endif /* __GST_GOO_DECSPARK_H__ */
