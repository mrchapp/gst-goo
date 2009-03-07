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
		 */
		g_hash_table_insert (visited_nodes, key, key);
		return FALSE;
	}
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

typedef struct {
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
		GST_INFO ("");

		g_signal_connect (elem, "pad-added", (GCallback)pad_added, ctx);

		/* Since it is likely that some pads are already hooked, let's iterate them
		 * (since there will be no pad-added signel for them) and simulate the
	 	* the missed signal
 *
		 * XXX here we probably should do something sane if the underlying data
		 * structure changes as we iterate.. hmm
		 */
		for( itr = gst_element_iterate_pads (elem);
	    	 gst_iterator_next (itr, &item) == GST_ITERATOR_OK;
	     	gst_object_unref (item) )
		{
			GstPad *pad = GST_PAD(item);
			pad_added (elem, pad, ctx);
		}

		/* in case this elem is a bin, we need to hook all the elements within
		 * the bin too:
		 */
		if( GST_IS_BIN (elem) )
		{
			for( itr = gst_bin_iterate_elements (elem);
		    	 gst_iterator_next (itr, &item) == GST_ITERATOR_OK;
		    	 gst_object_unref (item) )
			{
				hook_element (GST_ELEMENT (item), ctx);
			}
		}
	}

	if (ctx->cb)
	{
		GST_INFO ("going to call callback");
		ctx->cb (ctx->elem);
	}
}

static void pad_linked (GstElement *pad, GstPad *peer, PipelineChangeListenerContext *ctx)
{
	GST_INFO ("");
	GstElement *elem = NULL;

	/* in the case of GstGhostPad (and probably other proxy pads)
	 * the parent is actually the pad we are a proxy for, so
	 * keep looping until we find the GstElement
	 */
	while (peer != NULL)
	{
		GstObject *obj = gst_pad_get_parent (peer);
		GST_INFO ("obj=%s", G_OBJECT_TYPE_NAME (obj));
		if( GST_IS_PAD(obj) )
		{
			//???gst_object_unref (peer);
			peer = GST_PAD (obj);
		}
		else
		{
			elem = GST_ELEMENT (obj);
			break;
		}
	}

	if (G_UNLIKELY (elem == NULL))
	{
		GST_INFO ("hmm?  elem is null?  how does this happen?");
		return;
	}

	hook_element (elem, ctx);
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
	}
	else
	{
		g_signal_connect (pad, "linked", pad_linked, ctx);
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
 */
void
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
	ctx->cb = cb;

	/* just in case everything is already wired up, call the cb once
	 * to simulate the signal:
	 */
	cb (elem);
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
 * helper for gst_goo_utils_find_goo_component() to check if the current element
 * is the one we are searching for
 */
static GooComponent *
check_for_goo_component (GstElement *elem, SearchContext *ctx)
{
	GooComponent *component = GOO_COMPONENT (g_object_get_data (G_OBJECT (elem), "goo"));

	if (component != NULL)
	{
		GST_INFO("found GOO component: %s", G_OBJECT_TYPE_NAME (component));
		if (!G_TYPE_CHECK_INSTANCE_TYPE (component, ctx->type))
		{
			/* XXX do we need to release a ref here?? */
			component = NULL;
		}
	}

	return component;
}

/**
 * helper for gst_goo_utils_find_goo_component() to iterate and search
 * the members of a bin
 */
static GooComponent *
find_goo_component_in_bin (GstBin *bin, SearchContext *ctx)
{
	GstIterator  *itr;
	gpointer      item;

	GST_INFO ("bin=%s (%s)", gst_element_get_name (bin), G_OBJECT_TYPE_NAME (bin));

	/* note: we don't handle the case of the underlying data structure changing
	 * while iterating.. we just bail out and the user needs to restart.
	 */
	for( itr = gst_bin_iterate_elements (bin);
	     gst_iterator_next (itr, &item) == GST_ITERATOR_OK;
	     gst_object_unref (item) )
	{
		GooComponent *component;
		GstElement   *elem = GST_ELEMENT (item);

		component = check_for_goo_component (elem, ctx);

		if( component == NULL )
		{
			component = find_goo_component (elem, ctx);
		}

		if( component != NULL )
		{
			return component;
		}
	}
	return NULL;
}


/**
 * recursively iterate all our pads and search adjacent elements
 */
static GooComponent *
find_goo_component (GstElement *elem, SearchContext *ctx)
{
	GstIterator  *itr;
	gpointer      item;
	GstElement   *adjacent_elem;

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
	     gst_iterator_next (itr, &item) == GST_ITERATOR_OK;
	     gst_object_unref (item) )
	{
		GooComponent *component;
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
			GST_INFO ("Cannot find a adjacent element");
			gst_object_unref (peer);
			continue;
	}

		GST_INFO ("found adjacent_elem: %s", gst_element_get_name (adjacent_elem));

		component = check_for_goo_component (adjacent_elem, ctx);

		if (component == NULL)
		{
			/* if adjacent_elem is itself a bin, we need to search the
			 * contents of that bin:
			 */
			if( GST_IS_BIN (adjacent_elem) )
	{
				component = find_goo_component_in_bin (GST_BIN (adjacent_elem), ctx);
			}
	}

		if (component == NULL)
		{
			/* if we didn't find our component, recursively search
			 * the contents of adjacent_elem's pads:
			 */
			component = find_goo_component (adjacent_elem, ctx);
		}

		/* cleanup: */

		gst_object_unref (adjacent_elem);
		gst_object_unref (peer);

		if( component != NULL )
		{
			return component;
		}
	}

	/* if we get here, we didn't find 'nothin */
	return NULL;
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
gst_goo_utils_find_goo_component (GstElement *elem, GType type)
{
	SearchContext ctx;  /* only needed until this fxn returns, so alloc on stack */

	GST_INFO("type=%s", g_type_name(type));

	ctx.type = type;
	ctx.visited_nodes = g_hash_table_new (g_direct_hash, g_direct_equal);

	GooComponent *component = find_goo_component (elem, &ctx);

	g_hash_table_destroy (ctx.visited_nodes);

	return component;
}


