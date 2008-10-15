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

/**
 * GST_GOO_UTILS_GET_PEER_COMPONENT:
 * @pad: a #GstPad instance
 *
 * Checks if the next element is a GstGoo element. If so
 * returns a reference of the GooComponent. Unref after usage.
 *
 * Return value: The GooComponent associated to the peer element.
 * Null if the peer element doesn't have a GooComponent.
 */

GooComponent*
gst_goo_utils_get_peer_component (GstPad *pad)
{
	g_assert (pad != NULL);
	GstElement *next_element;
	GstObject *tmp_object;
	GstPad *peer;
	GooComponent *peer_component = NULL;

	peer = gst_pad_get_peer (pad);

	if (G_UNLIKELY (peer == NULL))
	{
		GST_INFO ("Cannot get a peer pad");
		return NULL;
	}

	tmp_object = gst_pad_get_parent (peer);

	if ( GST_IS_GHOST_PAD (tmp_object) )
	{
		GST_INFO ("Peer element is a ghost pad");

		if (tmp_object != NULL)
			g_object_unref (tmp_object);

		goto peer_unref;
	}

	next_element = GST_ELEMENT (tmp_object);

	if (!GST_IS_ELEMENT (next_element))
	{
		GST_INFO ("Tunnel not setup. Cannot get the GstElement of the peer");
		goto next_element_unref;
	}

	peer_component = GOO_COMPONENT (g_object_get_data
					   (G_OBJECT (next_element), "goo"));

	if (G_UNLIKELY (peer_component == NULL))
	{
		GST_INFO ("Cannot get GOO component from the next element");
		goto next_element_unref;
	}

next_element_unref:
	if (next_element != NULL)
		g_object_unref (next_element);
peer_unref:
	if (peer != NULL)
		g_object_unref (peer);

	return peer_component;
}


