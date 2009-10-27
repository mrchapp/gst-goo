/* GStreamer
 * Copyright (C) 2004 Benjamin Otte <otte@gnome.org>
 *               2005 Wim Taymans <wim@fluendo.com>
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


#include "gstgooadapter.h"
#include <string.h>

/* default size for the assembled data buffer */
#define DEFAULT_SIZE 16

GST_DEBUG_CATEGORY_STATIC (gst_goo_adapter_debug);
#define GST_CAT_DEFAULT gst_goo_adapter_debug

#define _do_init(thing) \
  GST_DEBUG_CATEGORY_INIT (gst_goo_adapter_debug, "adapter", 0, "object to splice and merge buffers to desired size")
GST_BOILERPLATE_FULL (GstGooAdapter, gst_goo_adapter, GObject, G_TYPE_OBJECT,
    _do_init);

static void gst_goo_adapter_dispose (GObject * object);
static void gst_goo_adapter_finalize (GObject * object);

static void
gst_goo_adapter_base_init (gpointer g_class)
{
  /* nop */
}

static void
gst_goo_adapter_class_init (GstGooAdapterClass * klass)
{
  GObjectClass *object = G_OBJECT_CLASS (klass);

  object->dispose = gst_goo_adapter_dispose;
  object->finalize = gst_goo_adapter_finalize;
}

static void
gst_goo_adapter_init (GstGooAdapter * adapter, GstGooAdapterClass * g_class)
{
  adapter->assembled_data = g_malloc (DEFAULT_SIZE);
  adapter->assembled_size = DEFAULT_SIZE;
}

static void
gst_goo_adapter_dispose (GObject * object)
{
  GstGooAdapter *adapter = GST_GOO_ADAPTER (object);

  gst_goo_adapter_clear (adapter);

  GST_CALL_PARENT (G_OBJECT_CLASS, dispose, (object));
}

static void
gst_goo_adapter_finalize (GObject * object)
{
  GstGooAdapter *adapter = GST_GOO_ADAPTER (object);

  g_free (adapter->assembled_data);

  GST_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

GstGooAdapter *
gst_goo_adapter_new (void)
{
  return g_object_new (GST_TYPE_GOO_ADAPTER, NULL);
}

void
gst_goo_adapter_clear (GstGooAdapter * adapter)
{
  g_return_if_fail (GST_IS_GOO_ADAPTER (adapter));
GST_DEBUG ("clearing %d bytes", adapter->size);

  g_slist_foreach (adapter->buflist, (GFunc) gst_mini_object_unref, NULL);
  g_slist_free (adapter->buflist);
  adapter->buflist = NULL;
  adapter->buflist_end = NULL;
  adapter->size = 0;
  adapter->skip = 0;
  adapter->assembled_len = 0;
}

void
gst_goo_adapter_push (GstGooAdapter * adapter, GstBuffer * buf)
{
  g_return_if_fail (GST_IS_GOO_ADAPTER (adapter));
  g_return_if_fail (GST_IS_BUFFER (buf));

GST_DEBUG ("0x%08x (size=%d)", buf, GST_BUFFER_SIZE (buf));

  adapter->size += GST_BUFFER_SIZE (buf);

  buf = gst_buffer_ref(buf);

  /* Note: merging buffers at this point is premature. */
  if (G_UNLIKELY (adapter->buflist == NULL)) {
    adapter->buflist = adapter->buflist_end = g_slist_append (NULL, buf);
  } else {
    /* Otherwise append to the end, and advance our end pointer */
    adapter->buflist_end = g_slist_append (adapter->buflist_end, buf);
    adapter->buflist_end = g_slist_next (adapter->buflist_end);
  }
}

/* Internal function that copies data into the given buffer, size must be
 * bigger than the first buffer */
static void
gst_goo_adapter_peek_into (GstGooAdapter * adapter, guint8 * data, guint size)
{
  GstBuffer *cur;
  GSList *cur_list;
  guint copied, to_copy;

  /* The first buffer might be partly consumed, so need to handle
   * 'skipped' bytes. */
  cur = adapter->buflist->data;
  copied = to_copy = MIN (GST_BUFFER_SIZE (cur) - adapter->skip, size);
  memcpy (data, GST_BUFFER_DATA (cur) + adapter->skip, copied);
  data += copied;

  cur_list = g_slist_next (adapter->buflist);
  while (copied < size) {
    g_assert (cur_list);
    cur = cur_list->data;
    cur_list = g_slist_next (cur_list);
    to_copy = MIN (GST_BUFFER_SIZE (cur), size - copied);
    memcpy (data, GST_BUFFER_DATA (cur), to_copy);
    data += to_copy;
    copied += to_copy;
  }
}

const guint8 *
gst_goo_adapter_peek (GstGooAdapter * adapter, guint size, OMX_BUFFERHEADERTYPE *omx_buffer)
{
  GstBuffer *cur;

  g_return_val_if_fail (GST_IS_GOO_ADAPTER (adapter), NULL);
  g_return_val_if_fail (size > 0, NULL);

  /* we don't have enough data, return NULL. This is unlikely
   * as one usually does an _available() first instead of peeking a
   * random size. */
  if (G_UNLIKELY (size > adapter->size))
    return NULL;

    gst_goo_adapter_peek_into (adapter, omx_buffer->pBuffer, size);

  return adapter->assembled_data;
}

void
gst_goo_adapter_flush (GstGooAdapter * adapter, guint flush)
{
  GstBuffer *cur;

  g_return_if_fail (GST_IS_GOO_ADAPTER (adapter));
  g_return_if_fail (flush <= adapter->size);

  GST_LOG_OBJECT (adapter, "flushing %u bytes", flush);
  adapter->size -= flush;
  adapter->assembled_len = 0;
  while (flush > 0) {
    g_assert ( adapter->buflist != NULL );
    cur = adapter->buflist->data;
    if (GST_BUFFER_SIZE (cur) <= flush + adapter->skip) {
      /* can skip whole buffer */
      flush -= GST_BUFFER_SIZE (cur) - adapter->skip;
      adapter->skip = 0;
      adapter->buflist =
          g_slist_delete_link (adapter->buflist, adapter->buflist);
      if (G_UNLIKELY (adapter->buflist == NULL))
        adapter->buflist_end = NULL;
GST_DEBUG ("0x%08x", cur);
      gst_buffer_unref (cur);
    } else {
      adapter->skip += flush;
      break;
    }
  }
}

guint
gst_goo_adapter_available (GstGooAdapter * adapter)
{
  g_return_val_if_fail (GST_IS_GOO_ADAPTER (adapter), 0);

  return adapter->size;
}

guint
gst_goo_adapter_available_fast (GstGooAdapter * adapter)
{
  g_return_val_if_fail (GST_IS_GOO_ADAPTER (adapter), 0);

  /* no buffers, we have no data */
  if (!adapter->buflist)
    return 0;

  /* some stuff we already assembled */
  if (adapter->assembled_len)
    return adapter->assembled_len;

  /* we cannot have skipped more than the first buffer */
  g_assert (GST_BUFFER_SIZE (adapter->buflist->data) > adapter->skip);

  /* we can quickly get the data of the first buffer */
  return GST_BUFFER_SIZE (adapter->buflist->data) - adapter->skip;
}
