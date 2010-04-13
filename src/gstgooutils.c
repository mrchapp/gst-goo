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


#include "gstgooutils.h"


GST_DEBUG_CATEGORY_STATIC (gstgooutils);
#define GST_CAT_DEFAULT gstgooutils


/**
 * If @key is not already in @visited_nodes, add it and return FALSE, otherwise
 * return TRUE.  The result is that the first time this is called with a given
 * key, it will return FALSE, but subsequent calls with the same key will return
 * TRUE
 */
static gboolean
already_visited ( GHashTable *visited_nodes, gpointer key )
{
	if( g_hash_table_lookup (visited_nodes, key) != NULL )
	{
		return TRUE;
	}
	else
	{
		/* we just map elem->elem... it only matters that the value is not
		 * NULL.  (Really, we just need a HashSet)
		 *
		 * Note: this doesn't increase the ref count of key.. so it is
		 * possible that it could be freed while it is still in the
		 * table..  not really sure if that is likely to cause any
		 * issue.
		 */
		g_hash_table_insert (visited_nodes, key, key);
		return FALSE;
	}
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

typedef struct {
	gboolean needs_cb;
	void (*cb) (GstElement *elem);
	GstElement *elem;
	GHashTable *visited_nodes;  /* table of nodes we've already visited, to prevent loops */
} PipelineChangeListenerContext;

static void pad_added (GstElement *elem, GstPad *pad, PipelineChangeListenerContext *ctx);


static void hook_element (GstElement *elem, PipelineChangeListenerContext *ctx)
{
	GstIterator *itr;
	gpointer     item;

	/* check if we've already examined this element, to prevent loops: */
	if (!already_visited (ctx->visited_nodes, elem))
	{
		GST_INFO_OBJECT (elem, "");

		g_signal_connect (elem, "pad-added", (GCallback)pad_added, ctx);

		/* Since it is likely that some pads are already hooked, let's iterate them
		 * (since there will be no pad-added signal for them) and simulate the
		 * the missed signal
		 *
		 * XXX here we probably should do something sane if the underlying data
		 * structure changes as we iterate.. hmm
		 */
		for( itr = gst_element_iterate_pads (elem);
		     gst_iterator_next (itr, &item) == GST_ITERATOR_OK;
		     gst_object_unref (item) )
		{
			pad_added (elem, GST_PAD(item), ctx);
		}
		gst_iterator_free (itr);

		/* in case this elem is a bin, we need to hook all the elements within
		 * the bin too:
		 */
		if( GST_IS_BIN (elem) )
		{
			for( itr = gst_bin_iterate_elements (GST_BIN (elem));
			     gst_iterator_next (itr, &item) == GST_ITERATOR_OK;
			     gst_object_unref (item) )
			{
				hook_element (GST_ELEMENT (item), ctx);
			}
			gst_iterator_free (itr);
		}

		ctx->needs_cb = TRUE;
	}
}

static void pad_linked (GstPad *pad, GstPad *peer, PipelineChangeListenerContext *ctx)
{
	GST_INFO_OBJECT (pad, "");
	GstElement *elem = NULL;
	GstObject *obj = NULL;

	/* in the case of GstGhostPad (and probably other proxy pads)
	 * the parent is actually the pad we are a proxy for, so
	 * keep looping until we find the GstElement
	 */
	while (peer != NULL)
	{
		if(obj)
		{
			gst_object_unref (obj);
		}

		obj = gst_pad_get_parent (peer);

		if( GST_IS_PAD(obj) )
		{
			peer = GST_PAD (obj);
		}
		else
		{
			elem = GST_ELEMENT (obj);
			break;
		}
	}

	if (G_LIKELY (elem))
	{
		hook_element (elem, ctx);

		if (ctx->needs_cb && ctx->cb)
		{
			GST_INFO ("going to call callback");
			ctx->cb (ctx->elem);
			ctx->needs_cb = FALSE;
		}
	}

	if(obj)
	{
		gst_object_unref (obj);
	}
}

static void pad_added (GstElement *elem, GstPad *pad, PipelineChangeListenerContext *ctx)
{
	GstPad *peer = gst_pad_get_peer (pad);

	GST_INFO ("found pad: %s (%s)", gst_pad_get_name (pad), G_OBJECT_TYPE_NAME (pad));

	/* if this pad isn't already linked, setup a signal handler.. otherwise
	 * simulate the signal:
	 */
	if (peer != NULL)
	{
		pad_linked (pad, peer, ctx);
		gst_object_unref (peer);
	}
	else
	{
		g_signal_connect ((gpointer)pad, "linked", (GCallback)pad_linked, (gpointer)ctx);
	}
}

/**
 * A utility which will setup the necessary signal handlers handle the "linked"
 * signal from @elem's pads, and from any pad of any element connected directly
 * or indirectly to this element.  And as new elements/pads are connected in to
 * the pipeline, the necessary signal handlers are set up for the new elements.
 * So basically use this function if you want to get notified of <i>any</i>
 * change to the shape of the pipeline.
 *
 * Note: currently we don't listen for pads being removed, or unlinked.. we
 * assume that no-one cares about this.. (although this could change.. I'm
 * just too lazy to handle that right now)
 *
 * @return a handle which can be later passed to gst_goo_util_unregister_pipeline_change_cb
 */
void *
gst_goo_util_register_pipeline_change_cb (GstElement *elem, void (*cb) (GstElement *elem))
{
	static gboolean first_time = TRUE;

	if (first_time)   /* ugg! any way to not need this hack to have debug logging? */
	{
		GST_DEBUG_CATEGORY_INIT (gstgooutils, "gstgooutils", 0, "GST-GOO Utils");
		first_time = FALSE;
	}

	GST_INFO ("");

	PipelineChangeListenerContext *ctx = (PipelineChangeListenerContext *)
		malloc (sizeof (PipelineChangeListenerContext));

	if (ctx == NULL)
		return NULL;

	ctx->visited_nodes = g_hash_table_new (g_direct_hash, g_direct_equal);

	ctx->elem = elem;

	/* until we have iterated and setup listeners for everything already in the
	 * pipeline... we don't want to generate a whole bunch of callbacks.. so
	 * leave cb as NULL for now.
	 */
	ctx->cb = NULL;

	hook_element (elem, ctx);

	/* ok, now that we have iterated and setup signal handlers, now we want any
	 * further changes to generate a callback:
	 */
	ctx->needs_cb = FALSE;
	ctx->cb = cb;

	/* just in case everything is already wired up, call the cb once
	 * to simulate the signal:
	 */
	cb (elem);

	return ctx;
}

/**
 * TODO Remove a previously setup pipeline change callback..
 *
 * @handle is what is returned by gst_goo_util_register_pipeline_change_cb()
 */
void
gst_goo_util_unregister_pipeline_change_cb (void *handle)
{
	PipelineChangeListenerContext *ctx = (PipelineChangeListenerContext *)handle;

	/* Note:  we want to do something like this:
	 *
	 *   g_hash_table_destroy (ctx->visited_nodes);
	 *   free(ctx);
	 *
	 * but we probably need to ensure that all signal handlers are unwired
	 * first, otherwise we could, in theory, get callbacks trying to deref
	 * freed memory..
	 */
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

typedef struct {
	GType       type;           /* type we are searching for */
	GHashTable *visited_nodes;  /* table of nodes we've already visited, to prevent loops */
} SearchContext;

static GooComponent * find_goo_component (GstElement *elem, SearchContext *ctx);


/**
 * helper for gst_goo_util_find_goo_component() to check if the current element
 * is the one we are searching for
 */
static GooComponent *
check_for_goo_component (GstElement *elem, SearchContext *ctx)
{
	GooComponent *component = GOO_COMPONENT (g_object_get_data (G_OBJECT (elem), "goo"));

	if (component != NULL)
	{
		if (!G_TYPE_CHECK_INSTANCE_TYPE (component, ctx->type))
		{
			GST_INFO("found GOO component: %s", G_OBJECT_TYPE_NAME (component));
			component = NULL;
		}
	}

	return component;
}

/**
 * helper for gst_goo_util_find_goo_component() to iterate and search
 * the members of a bin
 */
static GooComponent *
find_goo_component_in_bin (GstBin *bin, SearchContext *ctx)
{
	GstIterator  *itr;
	gpointer      item;
	GooComponent *component = NULL;

	GST_INFO ("bin=%s (%s)", gst_element_get_name (bin), G_OBJECT_TYPE_NAME (bin));

	/* note: we don't handle the case of the underlying data structure changing
	 * while iterating.. we just bail out and the user needs to restart.
	 */
	for( itr = gst_bin_iterate_elements (bin);
	     gst_iterator_next (itr, &item) == GST_ITERATOR_OK && !component;
	     gst_object_unref (item) )
	{
		GstElement   *elem = GST_ELEMENT (item);

		component = find_goo_component (elem, ctx);
	}
	gst_iterator_free (itr);

	return component;
}


/**
 * recursively iterate all our pads and search adjacent elements
 */
static GooComponent *
find_goo_component_in_elem (GstElement *elem, SearchContext *ctx)
{
	GstIterator  *itr;
	gpointer      item;
	GooComponent *component = NULL;


	/* check if we've already examined this element, to prevent loops: */
	if (already_visited (ctx->visited_nodes, elem))
	{
		GST_INFO ("already visited elem=%s (%s)", gst_element_get_name (elem), G_OBJECT_TYPE_NAME (elem));
		return NULL;
	}

	GST_INFO ("elem=%s (%s)", gst_element_get_name (elem), G_OBJECT_TYPE_NAME (elem));

	/* note: we don't handle the case of the underlying data structure changing
	 * while iterating.. we just bail out and the user needs to restart.
	 */
	for( itr = gst_element_iterate_pads (elem);
	     gst_iterator_next (itr, &item) == GST_ITERATOR_OK && !component;
	     gst_object_unref (item) )
	{
		GstElement   *adjacent_elem = NULL;
		GstPad *pad = GST_PAD (item);
		GstPad *peer = gst_pad_get_peer (pad);

		GST_INFO ("found pad: %s (%s)", gst_pad_get_name (pad), G_OBJECT_TYPE_NAME (pad));

		if (G_UNLIKELY (peer == NULL))
		{
			GST_INFO ("NULL peer.. not connected yet?");
			continue;
		}

		/* in the case of GstGhostPad (and probably other proxy pads)
		 * the parent is actually the pad we are a proxy for, so
		 * keep looping until we find the GstElement
		 */
		while(TRUE)
		{
			GstObject *obj = gst_pad_get_parent (peer);
			if( GST_IS_PAD(obj) )
			{
				gst_object_unref (peer);
				peer = GST_PAD (obj);
			}
			else
			{
				adjacent_elem = GST_ELEMENT (obj);
				break;
			}
		}

		if (G_UNLIKELY (adjacent_elem == NULL))
		{
			gst_object_unref (peer);
			GST_INFO ("Cannot find a adjacent element");
			continue;
		}

		GST_INFO ("found adjacent_elem: %s", gst_element_get_name (adjacent_elem));

		component = find_goo_component (adjacent_elem, ctx);

		/* cleanup: */
		gst_object_unref (adjacent_elem);
		gst_object_unref (peer);
	}
	gst_iterator_free (itr);

	return component;
}


static GooComponent *
find_goo_component (GstElement *elem, SearchContext *ctx)
{
	GooComponent *component = check_for_goo_component (elem, ctx);

	if (component == NULL)
	{
		if (GST_IS_BIN (elem))
		{
			component = find_goo_component_in_bin (GST_BIN (elem), ctx);
		}
	}

	if (component == NULL)
	{
		component = find_goo_component_in_elem (elem, ctx);
	}

	return component;
}


/**
 * Utility to search the pipeline that @elem is a member of find the element
 * with the specified @type
 *
 * Note: if underlying pipeline structure changes as we iterate thru it,
 * it could cause us to bail out early before iterating thru the entire
 * pipeline.. basically the caller should listen for changes to the pipeline
 * structure, and call this again if new elements are added and the caller
 * hasn't yet found the peer element it is looking for.
 */
GooComponent *
gst_goo_util_find_goo_component (GstElement *elem, GType type)
{
	SearchContext ctx;  /* only needed until this fxn returns, so alloc on stack */

	GST_INFO("type=%s", g_type_name(type));

	ctx.type = type;
	ctx.visited_nodes = g_hash_table_new (g_direct_hash, g_direct_equal);

	GooComponent *component = find_goo_component (elem, &ctx);

	g_hash_table_destroy (ctx.visited_nodes);

	return component;
}


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

static OMX_S64 omx_normalize_timestamp = 0;
static gboolean needs_normalization = TRUE;

/**
 * Utility function to handle transferring Gstreamer timestamp to OMX
 * timestamp.  This function handles discontinuities and timestamp
 * renormalization.
 *
 * @omx_buffer the destination OMX buffer for the timestamp
 * @buffer     the source Gstreamer buffer for the timestamp
 * @normalize  should this buffer be the one that we renormalize on
 *   (iff normalization is required)?  (ie. with TI OMX, you should
 *   only re-normalize on a video buffer)
 */
gboolean
gst_goo_timestamp_gst2omx (
		OMX_BUFFERHEADERTYPE* omx_buffer,
		GstBuffer* buffer,
		gboolean normalize)
{
	GstClockTime timestamp = GST_BUFFER_TIMESTAMP (buffer);

	if (GST_GOO_UTIL_IS_DISCONT (buffer))
	{
		needs_normalization = TRUE;
		GST_DEBUG ("needs_normalization");
	}

	if (needs_normalization && normalize)
	{
		GST_INFO ("Setting OMX_BUFFER_STARTTIME..");
		omx_buffer->nFlags |= OMX_BUFFERFLAG_STARTTIME;
		omx_normalize_timestamp = GST2OMX_TIMESTAMP ((gint64)timestamp);
		needs_normalization = FALSE;
		GST_DEBUG ("omx_normalize_timestamp=%lld", omx_normalize_timestamp);
	}

	/* transfer timestamp to openmax */
	if (GST_CLOCK_TIME_IS_VALID (timestamp))
	{
		omx_buffer->nTimeStamp = GST2OMX_TIMESTAMP ((gint64)timestamp) - omx_normalize_timestamp;
		GST_INFO ("OMX timestamp = %lld (%lld - %lld)", omx_buffer->nTimeStamp, GST2OMX_TIMESTAMP ((gint64)timestamp), omx_normalize_timestamp);
		return TRUE;
	}
	else
	{
		GST_WARNING ("Invalid timestamp!");
		return FALSE;
	}
}

/**
 * Utility function to handle transferring an OMX timestamp to a Gstreamer
 * timestamp
 */
gboolean
gst_goo_timestamp_omx2gst (GstBuffer *gst_buffer, OMX_BUFFERHEADERTYPE *buffer)
{
	gint64 buffer_ts = (gint64)buffer->nTimeStamp;
	guint64 timestamp = OMX2GST_TIMESTAMP (buffer_ts);

	/* We need to remove the OMX timestamp normalization */
	timestamp += OMX2GST_TIMESTAMP(omx_normalize_timestamp);

	if (GST_CLOCK_TIME_IS_VALID (timestamp))
	{
		GST_INFO ("Already had a timestamp: %" GST_TIME_FORMAT, GST_TIME_ARGS (timestamp));
		GST_BUFFER_TIMESTAMP (gst_buffer) = timestamp;
		return TRUE;
	}
	else
	{
		GST_WARNING ("Invalid timestamp!");
		return FALSE;
	}
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/**
 * This helper function is used to construct a custom event to send back
 * upstream when the sink receives the EOS event.  In tunnelled mode,
 * in some cases the upstream component (such as the video/audio decoder)
 * needs to perform some cleanup (such as sending EOS to OMX), which should
 * not happen until the GST layer sink receives the EOS.  The video/audio
 * decoder should not assume that when it receives the EOS, that the sink
 * has also received the EOS, since there may be multiple levels of queuing
 * between the decoder and the sink (ie. in a GstQueue element, and also in
 * the GstBaseSink class).
 */
GstEvent *
gst_goo_event_new_reverse_eos (void)
{
	return gst_event_new_custom ( GST_EVENT_CUSTOM_UPSTREAM,
			gst_structure_empty_new ("GstGooReverseEosEvent") );
}

/**
 * Helper function to determine if a received event is a reverse-eol event.
 */
gboolean
gst_goo_event_is_reverse_eos (GstEvent *evt)
{
	const GstStructure *structure = gst_event_get_structure (evt);
	if (structure)
	{
		return strcmp (gst_structure_get_name (structure), "GstGooReverseEosEvent") == 0;
	}
	return FALSE;
}


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/* some utilities to manage state changes of goo/OMX components... add to these
 * as-needed
 */

void
gst_goo_util_ensure_executing (GooComponent *component)
{
	if (goo_component_get_state (component) == OMX_StateLoaded)
	{
		goo_component_set_state_idle (component);
	}

	if (goo_component_get_state (component) == OMX_StateIdle)
	{
		goo_component_set_state_executing (component);
	}
}

/**
 * Utility function which post a message of type (GST_MESSAGE_ELEMENT)
 * with a @structure_name. The field type would be set to G_TYPE_DOUBLE, and its value
 * will be obtained using g_get_current_time
 *
 * @element the component
 * @structure_name
 */

void
gst_goo_util_post_message (GstElement* self,
							gchar* structure_name)
{

  GstMessage *msg = NULL;
  GstStructure *stru = NULL;
  gchar *str_timestamp = NULL;

  gdouble timestamp;
  GTimeVal current_time;

  /* Get the current time */
  g_get_current_time(&current_time);
  timestamp = current_time.tv_sec + current_time.tv_usec/1e6;
  str_timestamp = g_strdup_printf("%f",timestamp);

  if(structure_name)
  {
	stru = gst_structure_new(structure_name,
	                         "timestamp",
	                         G_TYPE_STRING,
	                         str_timestamp,
	                         NULL);
  }

  /* Create the message and send it to the bus */
  msg = gst_message_new_custom (GST_MESSAGE_ELEMENT, GST_OBJECT (self), stru);

  g_assert ( gst_element_post_message (GST_ELEMENT(self), msg));

  g_free ( str_timestamp );
  return;
}
