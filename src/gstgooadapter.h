/* GStreamer
 * Copyright (C) 2004 Benjamin Otte <otte@gnome.org>
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

#include <gst/gst.h>
#include <OMX_Core.h>

#ifndef __GST_GOO_ADAPTER_H__
#define __GST_GOO_ADAPTER_H__

G_BEGIN_DECLS


#define GST_TYPE_GOO_ADAPTER \
  (gst_goo_adapter_get_type())
#define GST_GOO_ADAPTER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_GOO_ADAPTER, GstGooAdapter))
#define GST_GOO_ADAPTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_GOO_ADAPTER, GstGooAdapterClass))
#define GST_GOO_ADAPTER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_GOO_ADAPTER, GstGooAdapterClass))
#define GST_IS_GOO_ADAPTER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_GOO_ADAPTER))
#define GST_IS_GOO_ADAPTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_GOO_ADAPTER))

typedef struct _GstGooAdapter GstGooAdapter;
typedef struct _GstGooAdapterClass GstGooAdapterClass;

/**
 * GstGooAdapter:
 * @object: the parent object.
 *
 * The opaque #GstGooAdapter data structure.
 */
struct _GstGooAdapter {
  GObject	object;

  /*< private >*/
  GSList *	buflist;
  guint		size;
  guint		skip;

  /* we keep state of assembled pieces */
  guint8 *	assembled_data;
  guint		assembled_size;
  guint		assembled_len;

  /* Remember where the end of our buffer list is to
   * speed up the push */
  GSList *buflist_end;
  gpointer _gst_reserved[GST_PADDING - 1];


};

struct _GstGooAdapterClass {
  GObjectClass	parent_class;

  /*< private >*/
  gpointer _gst_reserved[GST_PADDING];
};

GstGooAdapter *		gst_goo_adapter_new			(void);

void			gst_goo_adapter_clear		(GstGooAdapter *adapter);
void			gst_goo_adapter_push		(GstGooAdapter *adapter, GstBuffer* buf);
const guint8 *		gst_goo_adapter_peek      		(GstGooAdapter *adapter, guint size, OMX_BUFFERHEADERTYPE *omx_buffer);
void			gst_goo_adapter_flush		(GstGooAdapter *adapter, guint flush);
guint			gst_goo_adapter_available		(GstGooAdapter *adapter);
guint			gst_goo_adapter_available_fast    	(GstGooAdapter *adapter);
GType			gst_goo_adapter_get_type		(void);

G_END_DECLS

#endif /* __GST_GOO_ADAPTER_H__ */
