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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include "gstgoodecwmv.h"

GST_BOILERPLATE (GstGooDecWMV, gst_goo_decwmv, GstGooVideoDec, GST_TYPE_GOO_VIDEODEC);

GST_DEBUG_CATEGORY_STATIC (gst_goo_decwmv_debug);
#define GST_CAT_DEFAULT gst_goo_decwmv_debug

/* signals */
enum
{
	LAST_SIGNAL
};

/* args */
enum
{
	PROP_0
};


static const GstElementDetails details =
GST_ELEMENT_DETAILS (
	"OpenMAX WMV decoder",
	"Codedc/Decoder/Video",
	"Decodes WMV streams with OpenMAX",
	"Texas Instrument"
	);

static GstStaticPadTemplate sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
		GST_PAD_SINK,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS ("video/x-wmv,"
				"wmvversion = (int) 3, "
				"width = (int) [16, 4096], "
				"height = (int) [16, 4096], "
				"framerate = (GstFraction) [1/1, 1000/1]"));

static void
gst_goo_decwmv_wait_for_done (GstGooVideoFilter* self)
{
	g_assert (self != NULL);

	/* flushing the last buffers in adapter */

	OMX_BUFFERHEADERTYPE* omx_buffer;
	OMX_PARAM_PORTDEFINITIONTYPE* param =
		GOO_PORT_GET_DEFINITION (self->inport);
	GstGooAdapter* adapter = self->adapter;
	int omxbufsiz = param->nBufferSize;
	int avail = gst_goo_adapter_available (adapter);

	if (goo_port_is_tunneled (self->inport))
	{
		GST_INFO ("Input port is tunneled: Setting done");
		goo_component_set_done (self->component);
		return;
	}

	if (avail < omxbufsiz && avail > 0)
	{
		GST_INFO ("Marking EOS buffer");
		omx_buffer = goo_port_grab_buffer (self->inport);
		GST_DEBUG ("Peek to buffer %d bytes", avail);
		gst_goo_adapter_peek (adapter, avail, omx_buffer);
		omx_buffer->nFilledLen = avail;
		/* let's send the EOS flag right now */
		omx_buffer->nFlags |= OMX_BUFFERFLAG_EOS;
		goo_component_release_buffer (self->component, omx_buffer);
	}
	else if (avail == 0)
	{
		GST_DEBUG ("Sending empty buffer with EOS flag in it");
		goo_component_send_eos (self->component);
	}
	else
	{
		/* For some reason the adapter didn't extract all
		   possible buffers */
		GST_ERROR ("Adapter algorithm error!");
		goo_component_send_eos (self->component);
	}

	gst_goo_adapter_clear (adapter);

	GST_INFO ("Waiting for done signal");
	if (goo_port_is_tunneled (self->outport))
	{
		GST_INFO ("Outport is tunneled: Setting done");
		goo_component_set_done (self->component);
	}
	else
	{
		GST_INFO ("Waiting for done signal");
		goo_component_wait_for_done (self->component);
	}

	return;
}

static gboolean
gst_goo_decwmv_src_event (GstPad* pad, GstEvent* event)
{
	GST_LOG("");
	
	GstGooVideoFilter* self = GST_GOO_VIDEO_FILTER (gst_pad_get_parent (pad));
	
	gboolean ret;
	
	g_assert (self->component != NULL);
	
	switch (GST_EVENT_TYPE (event))
	{
	case GST_EVENT_SEEK:
		GST_INFO ("Seek Event");
		self->seek_active = TRUE;
		ret = gst_pad_push_event (self->sinkpad, event);
		break;
	default:
		ret = FALSE;
		break;
	}

	gst_object_unref (self);
	return ret;
}

static gboolean
gst_goo_decwmv_sink_event (GstPad* pad, GstEvent* event)
{
	GST_LOG ("");
	
	GstGooVideoFilter* self = GST_GOO_VIDEO_FILTER (gst_pad_get_parent (pad));
	
	gboolean ret;

	g_assert (self->component != NULL);

	switch (GST_EVENT_TYPE (event))
	{
	case GST_EVENT_EOS:
		GST_INFO ("EOS event");
		gst_goo_decwmv_wait_for_done (self);
		ret = gst_pad_push_event (self->srcpad, event);
		break;
	case GST_EVENT_FLUSH_START:
		GST_INFO ("Flush Start Event");
		goo_component_set_state_pause(self->component);
		goo_component_flush_all_ports(self->component);
		ret = gst_pad_push_event (self->srcpad, event);
		break;
	case GST_EVENT_FLUSH_STOP:
		GST_INFO ("Flush Stop Event");
		goo_component_set_state_executing(self->component);
		ret = gst_pad_push_event (self->srcpad, event);
		break;
	case GST_EVENT_NEWSEGMENT:
	{
		GST_INFO ("Newsegment Event");
		gboolean update;
		GstFormat fmt;
		gint64 stop, time;
		gdouble rate, arate;

		gst_event_parse_new_segment_full (event, &update, &rate, &arate,
			&fmt, &self->seek_time, &stop, &time);
		GST_DEBUG_OBJECT (self, "New Start Time %"GST_TIME_FORMAT,
			GST_TIME_ARGS (self->seek_time));
		ret = gst_pad_push_event (self->srcpad, event);
		break;
	}
	default:
		ret = gst_pad_event_default (pad, event);
		break;
	}

	gst_object_unref (self);
	return ret;
}

static void
gst_goo_decwmv_base_init (gpointer g_klass)
{
	GST_DEBUG_CATEGORY_INIT (gst_goo_decwmv_debug, "goodecwmv", 0,
			"OpenMAX WMV decoder element");

	GstElementClass* e_klass = GST_ELEMENT_CLASS (g_klass);

	gst_element_class_add_pad_template (e_klass,
			gst_static_pad_template_get
			(&sink_factory));

	gst_element_class_set_details (e_klass, &details);

	return;
}

static GooTiWMVDecFileType
gst_goo_decwmv_get_file_type (GstGooDecWMV* self)
{
	GooTiWMVDecFileType file_type;
	GooComponent *component = GST_GOO_VIDEO_FILTER (self)->component;
	g_object_get (G_OBJECT (component), "file-type", &file_type, NULL);
	return file_type;

}

static GstBuffer*
gst_goo_decwmv_extra_buffer_processing(GstGooVideoFilter *filter, GstBuffer *buffer)
{

	GstGooDecWMV *self = GST_GOO_DECWMV (filter);
	gboolean is_vc1 = gst_goo_decwmv_get_file_type(self) == GOO_TI_WMVDEC_FILE_TYPE_VC1;

	GstClockTime old_ts = GST_BUFFER_TIMESTAMP(buffer);
	guint64 old_off = GST_BUFFER_OFFSET(buffer);

	if(is_vc1)
	{
		guint32 mark_data = GUINT_TO_LE(0x0d010000);
		GstBuffer *mark = gst_buffer_new ();
		gst_buffer_set_data (mark, (gchar*)&mark_data, 4);
		buffer = gst_buffer_join (mark, buffer);
	}

	if (self->parsed_header == TRUE)
	;
	else if (GST_IS_BUFFER (GST_GOO_VIDEODEC(self)->video_header))
	{
		self->parsed_header = TRUE;
		GST_DEBUG ("Parsing WMV RCV Header info");
		if(is_vc1)
		{
			guint header_size =  GST_BUFFER_SIZE(GST_GOO_VIDEODEC(self)->video_header) - 1;
			GstBuffer *header = gst_buffer_new_and_alloc (header_size);
			memcpy(GST_BUFFER_DATA(header), GST_BUFFER_DATA(GST_GOO_VIDEODEC(self)->video_header)+1, header_size);
			buffer = gst_buffer_join (header, buffer);
		}
		else
		{
			GstBuffer *new_buf;
			new_buf = gst_buffer_new_and_alloc (20);

			/* Initial bytes for the Header info ,
			   hardcoded to WMV version 3 */
			((guint32*)GST_BUFFER_DATA (new_buf))[0] = GUINT_TO_LE(0x850002cb);

			/* Extended header size - VOL header size */
			((guint32*)GST_BUFFER_DATA (new_buf))[1] = GUINT_TO_LE(0x04);

			/* Get the Volume Header from the extra_info */
			((guint32*)GST_BUFFER_DATA (new_buf))[2] = GUINT_TO_LE(*(guint32*)(GST_BUFFER_DATA(GST_GOO_VIDEODEC(self)->video_header)));

			/* Use GOO port to obtain frame width and height information */
			GooPort* port = filter->inport;
			int frame_width;
			int frame_height;

			frame_height = GOO_PORT_GET_DEFINITION (port)->format.video.nFrameHeight;
			frame_width = GOO_PORT_GET_DEFINITION (port)->format.video.nFrameWidth;

			/* Copy the Frame Height */
			((guint32*)GST_BUFFER_DATA (new_buf))[3] = GUINT_TO_LE(frame_height);

			/* Copy the Frame Width */
			((guint32*)GST_BUFFER_DATA (new_buf))[4] = GUINT_TO_LE(frame_width);

			/* Send OMX the 20 byte RCV Header information */
			buffer = gst_buffer_join (new_buf, buffer);
		}
	}

	GST_BUFFER_TIMESTAMP(buffer) = old_ts;
	GST_BUFFER_OFFSET(buffer) = old_off;
	return buffer;

}

static GstBuffer*
gst_goo_decwmv_codec_data_processing (GstGooVideoFilter *filter, GstBuffer *buffer)
{

	/* @Todo: Added this ugly hack for time purposes. Should be fixed by extracting
			   the type of stream VC1 or RCV */

	GstGooDecWMV *self = GST_GOO_DECWMV (filter);
	GooComponent *component = GST_GOO_VIDEO_FILTER (self)->component;
	gboolean is_vc1;

	is_vc1 = GST_BUFFER_SIZE(GST_GOO_VIDEODEC(self)->video_header) == 22;

	if(is_vc1)
	{
		g_object_set (G_OBJECT (component), "file-type", GOO_TI_WMVDEC_FILE_TYPE_VC1, NULL);
		g_object_set (G_OBJECT (self), "process-mode", GOO_TI_VIDEO_DECODER_FRAMEMODE, NULL);
		//GOO_TI_VIDEO_DECODER_STREAMMODE, omx not support stream mode yet
	}
	else
	{
		g_object_set (G_OBJECT (component), "file-type", GOO_TI_WMVDEC_FILE_TYPE_RCV, NULL);
		if (GST_IS_BUFFER (GST_GOO_VIDEODEC(self)->video_header))
			g_object_set (G_OBJECT (self), "process-mode", GOO_TI_VIDEO_DECODER_FRAMEMODE, NULL);
	}

	return buffer;
}


static void
gst_goo_decwmv_class_init (GstGooDecWMVClass* klass)
{
	GObjectClass* g_klass;
	GParamSpec* pspec;
	GstElementClass* gst_klass;

	/* GST GOO VIDEO_FILTER */
	GstGooVideoFilterClass* gst_c_klass = GST_GOO_VIDEO_FILTER_CLASS (klass);
	gst_c_klass->codec_data_processing_func = GST_DEBUG_FUNCPTR (gst_goo_decwmv_codec_data_processing);
	gst_c_klass->extra_buffer_processing_func = GST_DEBUG_FUNCPTR (gst_goo_decwmv_extra_buffer_processing);

	return;
}

static void
gst_goo_decwmv_init (GstGooDecWMV* self, GstGooDecWMVClass* klass)
{
	GST_DEBUG ("");

	self->parsed_header = FALSE;
	GST_GOO_VIDEO_FILTER (self)->component = goo_component_factory_get_component
		(GST_GOO_VIDEO_FILTER (self)->factory, GOO_TI_WMV_DECODER);

	GooComponent* component = GST_GOO_VIDEO_FILTER (self)->component;

	/* Select Stream mode operation as default */
	g_object_set (G_OBJECT (self), "process-mode", GOO_TI_VIDEO_DECODER_FRAMEMODE, NULL);

	/* input port */
	GST_GOO_VIDEO_FILTER (self)->inport = goo_component_get_port (component, "input0");
	g_assert (GST_GOO_VIDEO_FILTER (self)->inport != NULL);

	GST_GOO_VIDEO_FILTER (self)->outport = goo_component_get_port (component, "output0");
	g_assert (GST_GOO_VIDEO_FILTER (self)->outport != NULL);

	/** Use the PARENT's callback function **/
	goo_port_set_process_buffer_function
		(GST_GOO_VIDEO_FILTER (self)->outport, gst_goo_video_filter_outport_buffer);
	
	gst_pad_set_event_function (GST_GOO_VIDEO_FILTER(self)->sinkpad, GST_DEBUG_FUNCPTR (gst_goo_decwmv_sink_event));
	gst_pad_set_event_function (GST_GOO_VIDEO_FILTER(self)->srcpad, GST_DEBUG_FUNCPTR (gst_goo_decwmv_src_event));

	g_object_set_data (G_OBJECT (GST_GOO_VIDEO_FILTER (self)->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", GST_GOO_VIDEO_FILTER (self)->component);

	return;
}

