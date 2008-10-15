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

#include <goo-ti-h264dec.h>
#include "gstgoodech264.h"
#include <string.h>

GST_BOILERPLATE (GstGooDecH264, gst_goo_dech264, GstGooVideoDec, GST_TYPE_GOO_VIDEODEC);

GST_DEBUG_CATEGORY_STATIC (gst_goo_dech264_debug);
#define GST_CAT_DEFAULT gst_goo_dech264_debug

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

/** Global variable for a 4 byte workaround
    on the demuxers. Refer to extra bufer
    processing function **/
static gboolean gst_goo_dech264_parsed_header = FALSE;

/* default values */
#define DEFAULT_WIDTH 352
#define DEFAULT_HEIGHT 288
#define DEFAULT_FRAMERATE 30
#define DEFAULT_COLOR_FORMAT OMX_COLOR_FormatCbYCrY

static const GstElementDetails details =
        GST_ELEMENT_DETAILS (
                "OpenMAX H264 decoder",
                "Codedc/Decoder/Video",
                "Decodes H264 streams with OpenMAX",
                "Texas Instrument"
                );

static GstStaticPadTemplate sink_factory =
        GST_STATIC_PAD_TEMPLATE ("sink",
                GST_PAD_SINK,
                GST_PAD_ALWAYS,
                GST_STATIC_CAPS ("video/x-h264,"
				"width = (int) [16, 4096], "
				"height = (int) [16, 4096], "
				"framerate = (GstFraction) [1/1, 60/1]"));


static void
gst_goo_dech264_base_init (gpointer g_klass)
{
        GST_DEBUG_CATEGORY_INIT (gst_goo_dech264_debug, "goodech264", 0,
                                 "OpenMAX H264 decoder element");

        GstElementClass* e_klass = GST_ELEMENT_CLASS (g_klass);

        gst_element_class_add_pad_template (e_klass,
                                            gst_static_pad_template_get
                                            (&sink_factory));

        gst_element_class_set_details (e_klass, &details);

        return;
}

static GstBuffer*
gst_goo_dech264_codec_data_processing (GstGooVideoFilter *filter, GstBuffer *buf)
{

	GstGooDecH264 *self = GST_GOO_DECH264 (filter);

	if (GST_IS_BUFFER (GST_GOO_VIDEODEC(self)->video_header))
	{
		GST_DEBUG_OBJECT (self, "Adding H.264 header info to buffer");

		/** We need to make this workaround for buffers
		    after parsing the headers **/
		gst_goo_dech264_parsed_header = TRUE;

		gint index=0, profile=0, level=0, NALU_size_bytes=0;
		gint m_DecoderSpecificSize=0, numOfSPS=0;
		gint numOfPPS=0, pos=0, i=0;
		guint sps_len=0, pps_len=0;
		char *buff, *pSPS, *pPPS, *m_pDecoderSpecific;
		GstBuffer *new_buf;

		buff = GST_BUFFER_DATA (GST_GOO_VIDEODEC(self)->video_header);

		/* TODO: verify header == 1 */
		index++;
		/* profile ID */
		profile = buff[index++];
		/* TODO: read compatible profiles */
		index++;
		/* level */
		level = buff[index++];
		GST_DEBUG_OBJECT (self, "H.264 ProfileID=%d, Level=%d", profile, level);

		/* 6 bits reserved (111111) + 2 bits nal size length - 1 (11) */
		NALU_size_bytes = (buff[index++] & 0x03) + 1;
		GST_DEBUG_OBJECT (self, "NALU size = %d", NALU_size_bytes);
		/* TODO: validate value == 1, 2, or 4 */

		/* 3 bits reserved (111) + 5 bits number of SPS */
		numOfSPS = buff[index++] & 0x1f;
		GST_DEBUG_OBJECT (self, "Number of SPS's = %d", numOfSPS);

		/* WORKAROUND: Make it look like MP4Box */
		/* Add 4 bytes to the data size */
		m_DecoderSpecificSize = 4;

		for (i=0; i < numOfSPS; i++)
		{
			/* Remember first SPS NAL unit */
			if (i == 0)
				pSPS = buff + index;

			/* SPS length */
			sps_len = buff[index++]*0x100;
			sps_len+= buff[index++];
			GST_DEBUG_OBJECT (self, "SPS #%d: sps_len=%d", i, sps_len);
			m_DecoderSpecificSize += sps_len + NALU_size_bytes;
			/* skip SPS content */
			index += sps_len;
		}

		/* number of PPS */
		numOfPPS = buff[index++];
		GST_DEBUG_OBJECT (self, "Number of PPS's = %d", numOfPPS);

		for (i=0; i < numOfPPS; i++)
		{
			/* Remember first PPS NAL unit */
			if (i == 0)
				pPPS = buff + index;

			/* PPS length */
			pps_len = buff[index++]*0x100;
			pps_len+= buff[index++];
			GST_DEBUG_OBJECT (self, "PPS #%d: pps_len=%d", i, pps_len);
			m_DecoderSpecificSize += pps_len + NALU_size_bytes;
			/* skip PPS content */
			index += pps_len;
		}

		GST_DEBUG_OBJECT (self, "SPS's=%d, PPS's=%d, size=%d",
				numOfSPS, numOfPPS, m_DecoderSpecificSize);

		/* store SPS and PPS */
		/* FIXME: use gstbuffer; and validate */
		GstBuffer *temp_buffer = gst_buffer_new_and_alloc (m_DecoderSpecificSize);
		m_pDecoderSpecific = GST_BUFFER_DATA (temp_buffer);
		GST_DEBUG_OBJECT (self, "temp_buffer's size=%d", GST_BUFFER_SIZE (temp_buffer));

		index = 0;
		for (i=0; i < numOfSPS; i++)
		{
			/* SPS length */
			sps_len = pSPS[index++]*0x100;
			sps_len+= pSPS[index++];
			switch (NALU_size_bytes)
			{
			case 1:
				m_pDecoderSpecific[pos++] = sps_len & 0xFF;
				break;
			case 2:
				m_pDecoderSpecific[pos++] = (sps_len >> 8) & 0xFF;
				m_pDecoderSpecific[pos++] = (sps_len >> 0) & 0xFF;
				break;
			case 4:
				m_pDecoderSpecific[pos++] = 0;
				m_pDecoderSpecific[pos++] = 0;
				/* WORKAROUND: Make it look like MP4Box */
				/* m_pDecoderSpecific[pos++] = (sps_len / 0x100) & 0xFF;
				   m_pDecoderSpecific[pos++] = (sps_len % 0x100) & 0xFF; */
				m_pDecoderSpecific[pos++] = 0;
				m_pDecoderSpecific[pos++] = 1;
				GST_DEBUG_OBJECT (self, "[%02x %02x]",
						m_pDecoderSpecific[pos-2], m_pDecoderSpecific[pos-1]);
				break;
			}
			GST_DEBUG_OBJECT (self, "memcpy here. pos=%d, index=%d, sps_len=%d",
					pos, index, sps_len);
			memcpy (m_pDecoderSpecific+pos, pSPS+index, sps_len);
			/* skip SPS content */
			index += sps_len;
			pos += sps_len;
		}

		index = 0;
		for (i=0; i < numOfPPS; i++)
		{
			/* PPS length */
			pps_len = pPPS[index++]*0x100;
			pps_len+= pPPS[index++];
			switch (NALU_size_bytes)
			{
			case 1:
				m_pDecoderSpecific[pos++] = pps_len & 0xFF;
				break;
			case 2:
				m_pDecoderSpecific[pos++] = (pps_len >> 8) & 0xFF;
				m_pDecoderSpecific[pos++] = (pps_len >> 0) & 0xFF;
				break;
			case 4:
				m_pDecoderSpecific[pos++] = 0;
				m_pDecoderSpecific[pos++] = 0;
				/* WORKAROUND: Make it look like MP4Box */
				/* m_pDecoderSpecific[pos++] = (pps_len / 0x100) & 0xFF;
				   m_pDecoderSpecific[pos++] = (pps_len % 0x100) & 0xFF; */
				m_pDecoderSpecific[pos++] = 0;
				m_pDecoderSpecific[pos++] = 1;
				GST_DEBUG_OBJECT (self, "[%02x %02x]",
						m_pDecoderSpecific[pos-2], m_pDecoderSpecific[pos-1]);
				break;
			}
			GST_DEBUG_OBJECT (self, "memcpy2 here. pos=%d, index=%d, pps_len=%d",
					pos, index, pps_len);
			memcpy (m_pDecoderSpecific+pos, pPPS+index, pps_len);
			/* skip PPS content */
			index += pps_len;
			pos += pps_len;
		}

		/* WORKAROUND: Make it look like MP4Box */

		m_pDecoderSpecific[pos++] = 0;
		m_pDecoderSpecific[pos++] = 0;
		m_pDecoderSpecific[pos++] = 0;
		m_pDecoderSpecific[pos++] = 1;

		/* WORKAROUND: Make it look like MP4Box */
		GST_DEBUG_OBJECT (self, "working around stuff 1");

		GST_BUFFER_DATA (buf) += 4;
		GST_BUFFER_SIZE (buf) -= 4;

		new_buf = gst_buffer_merge (GST_BUFFER (temp_buffer), GST_BUFFER (buf));

		if (G_LIKELY (GST_BUFFER (buf)))
		{
			gst_buffer_unref (GST_BUFFER (buf));
		}

		if (G_LIKELY (GST_BUFFER (temp_buffer)))
		{
			gst_buffer_unref (GST_BUFFER (temp_buffer));
		}

		/* WORKAROUND: Make it look like MP4Box */
		GST_DEBUG_OBJECT (self, "working around stuff 2");
/* 	GST_BUFFER_DATA (new_buf)[545] = 0; */
/* 	GST_BUFFER_DATA (new_buf)[546] = 1; */

		return new_buf;


	}

	return buf;

}

static GstBuffer*
gst_goo_dech264_extra_buffer_processing (GstGooVideoFilter* filter, GstBuffer *buffer)
{

	GstGooDecH264 *self = GST_GOO_DECH264 (filter);
	static gboolean first_buffer = TRUE;

	GST_DEBUG ("Entering");

	/** Workaround to make it look like the MP4Box
	    extraction tool **/
	if ( gst_goo_dech264_parsed_header == TRUE && !first_buffer )
	{
		GST_DEBUG ("GST_BUFFER DATA Chaning 4 bytes");

		GST_BUFFER_DATA (buffer)[0] = 0;
		GST_BUFFER_DATA (buffer)[1] = 0;
		GST_BUFFER_DATA (buffer)[2] = 0;
		GST_BUFFER_DATA (buffer)[3] = 1;
	}

	first_buffer = FALSE;

	GST_DEBUG ("Exit");

	return buffer;
}


static void
gst_goo_dech264_class_init (GstGooDecH264Class* klass)
{
	GObjectClass* g_klass;
	GParamSpec* pspec;
	GstElementClass* gst_klass;

	/* GST GOO VIDEO_FILTER */
	GstGooVideoFilterClass* gst_c_klass = GST_GOO_VIDEO_FILTER_CLASS (klass);
	gst_c_klass->codec_data_processing_func = GST_DEBUG_FUNCPTR (gst_goo_dech264_codec_data_processing);
	gst_c_klass->extra_buffer_processing_func = GST_DEBUG_FUNCPTR (gst_goo_dech264_extra_buffer_processing);

	return;
}

static void
gst_goo_dech264_init (GstGooDecH264* self, GstGooDecH264Class* klass)
{
	GST_DEBUG ("");

	GST_GOO_VIDEO_FILTER (self)->component = goo_component_factory_get_component
		(GST_GOO_VIDEO_FILTER (self)->factory, GOO_TI_H264_DECODER);

	GooComponent* component = GST_GOO_VIDEO_FILTER (self)->component;

	/* Select Stream mode operation as default */
	g_object_set (G_OBJECT (component), "process-mode", GOO_TI_VIDEO_DECODER_FRAMEMODE, NULL);

	/* input port */
	GST_GOO_VIDEO_FILTER (self)->inport = goo_component_get_port (component, "input0");
	g_assert (GST_GOO_VIDEO_FILTER (self)->inport != NULL);

	{
		GooPort* port = GST_GOO_VIDEO_FILTER (self)->inport;
		GOO_PORT_GET_DEFINITION (port)->format.video.nFrameWidth = DEFAULT_WIDTH;
		GOO_PORT_GET_DEFINITION (port)->format.video.nFrameHeight = DEFAULT_HEIGHT;
		GOO_PORT_GET_DEFINITION (port)->format.video.eColorFormat = DEFAULT_COLOR_FORMAT;
	}

	GST_GOO_VIDEO_FILTER (self)->outport = goo_component_get_port (component, "output0");
	g_assert (GST_GOO_VIDEO_FILTER (self)->outport != NULL);

	/* output port */
	{
		GooPort* port = GST_GOO_VIDEO_FILTER (self)->outport;
		GOO_PORT_GET_DEFINITION (port)->format.video.nFrameWidth = DEFAULT_WIDTH;
		GOO_PORT_GET_DEFINITION (port)->format.video.nFrameHeight = DEFAULT_HEIGHT;
		GOO_PORT_GET_DEFINITION (port)->format.video.eColorFormat = DEFAULT_COLOR_FORMAT;

		/** Use the PARENT's callback function **/
		goo_port_set_process_buffer_function
			(port, gst_goo_video_filter_outport_buffer);

	}

	g_object_set_data (G_OBJECT (GST_GOO_VIDEO_FILTER (self)->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", GST_GOO_VIDEO_FILTER (self)->component);

        return;
}

