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

#include <goo-ti-wmadec.h>
#include <gst/riff/riff-ids.h>
#include "gstgoodecwma.h"
#include <string.h>
#define OMX 0x0101

#define U32(A) (*(guint32*)(A))
#define U16(A) (*(guint16*)(A))
#define U8(A) (*(guint8*)(A))


GST_BOILERPLATE (GstGooDecWma, gst_goo_decwma, GstGooAudioFilter, GST_TYPE_GOO_AUDIO_FILTER);

GST_DEBUG_CATEGORY_STATIC (gst_goo_decwma_debug);
#define GST_CAT_DEFAULT gst_goo_decwma_debug

/* signals */
enum
{
        LAST_SIGNAL
};

/* args */
enum
{
	PROP_0,
	PROP_WMATXTFILE
};

static gboolean gst_goo_decwma_src_setcaps (GstPad *pad, GstCaps *caps);
static gboolean gst_goo_decwma_sink_setcaps (GstPad *pad, GstCaps *caps);

#define GST_GOO_DECWMA_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_GOO_DECWMA, GstGooDecWmaPrivate))

struct _GstGooDecWmaPrivate
{
	guint incount;
	guint outcount;

	/* Wma context variables */
	guint sample_rate;
	guint channels;
	guint block_align;
	guint bitrate;
	guint wmaversion;

	WMA_HeadInfo wma_context;

};

/* default values */
#define CHANNELS_DEFAULT 1
#define SAMPLERATE_DEFAULT 44100
#define INPUT_BUFFERSIZE_DEFAULT 4096
#define OUTPUT_BUFFERSIZE_DEFAULT (4096*2*2)
#define NUM_INPUT_BUFFERS_DEFAULT 1
#define NUM_OUTPUT_BUFFERS_DEFAULT 1
#define DEFAULT_WIDTH 16
#define DEFAULT_DEPTH 16

static const GstElementDetails details =
        GST_ELEMENT_DETAILS (
                "OpenMAX WMA decoder",
                "Codec/Decoder/Audio",
                "Decodes Windows Media Audio streams with OpenMAX",
                "Texas Instrument"
                );

static GstStaticPadTemplate src_factory =
GST_STATIC_PAD_TEMPLATE ("src",
		GST_PAD_SRC,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS ("audio/x-raw-int, "
				"rate = (int) [8000, 192000], "
				"channels = (int) [1, 8], "
				"width = (int) [8, 4096], "
				"depth = (int) [8, 4096] "
				));

static GstStaticPadTemplate sink_factory =
        GST_STATIC_PAD_TEMPLATE (
                "sink",
                GST_PAD_SINK,
                GST_PAD_ALWAYS,
                GST_STATIC_CAPS ("audio/x-wma, "
				"wmaversion = (int) 2, "
				"rate = (int) [8000, 96000], "
				"bitrate = (int) [0, 192000], "
				"channels = (int) [1, 8], "
				"block_align = (int) [0, 100000]"
				));

static gboolean
gst_goo_decwma_process_mode (GstGooAudioFilter *self, guint value)
{
	GooComponent *component = self->component;

	g_object_set (G_OBJECT (component), "frame-mode", value ? TRUE : FALSE, NULL);

	return TRUE;
}

static void
gst_goo_decwma_set_property (GObject* object, guint prop_id,
				const GValue* value, GParamSpec* pspec)
{
	g_assert (GST_IS_GOO_DECWMA (object));
	GstGooDecWma* self = GST_GOO_DECWMA (object);
	GooComponent *component = GST_GOO_AUDIO_FILTER (self)->component;
	GST_DEBUG_OBJECT (self, "set_property");

	switch (prop_id)
	{
		case PROP_WMATXTFILE:
		{
			const gchar *wmatxtfile = NULL;
			wmatxtfile = g_value_get_string (value);
			if (G_LIKELY (self->wmatxtfile))
				g_free ((gpointer)self->wmatxtfile);
			self->wmatxtfile = g_strdup(wmatxtfile);

			if (g_file_test (self->wmatxtfile, G_FILE_TEST_IS_REGULAR))
				g_object_set (G_OBJECT (component),
					"wmatxtfile", self->wmatxtfile, NULL);

			break;
		}

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}

	return;
}

static void
gst_goo_decwma_dispose (GObject* object)
{
        G_OBJECT_CLASS (parent_class)->dispose (object);
        GstGooDecWma* self = GST_GOO_DECWMA (object);
        if (self->wmatxtfile)
        {
        	g_free ((gpointer)self->wmatxtfile);
        	self->wmatxtfile = NULL;
        }

	if (self->audio_header)
	{
		//gst_buffer_unref (self->audio_header);
		self->audio_header = NULL;
	}

        return;
}

static void
gst_goo_decwma_wait_for_done (GstGooAudioFilter* self)
{
	g_assert (self != NULL);

	/* flushing the last buffers in adapter */

	OMX_BUFFERHEADERTYPE* omx_buffer;
	OMX_PARAM_PORTDEFINITIONTYPE* param =
		GOO_PORT_GET_DEFINITION (self->inport);
	GstGooAdapter* adapter = self->adapter;
	int omxbufsiz = param->nBufferSize;
	int avail = gst_goo_adapter_available (adapter);

	if (avail < omxbufsiz && avail > 0)
	{
		GST_INFO ("Marking EOS buffer");
		omx_buffer = goo_port_grab_buffer (self->inport);
		GST_DEBUG ("Peek to buffer %d bytes", avail);
		gst_goo_adapter_peek (adapter, avail, omx_buffer);
		omx_buffer->nFilledLen = avail;
		/* let's send the EOS flag right now */
		omx_buffer->nFlags = OMX_BUFFERFLAG_EOS;
		/** @todo timestamp the buffers */
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
	goo_component_wait_for_done (self->component);

	return;
}

static gboolean
gst_goo_decwma_sink_event (GstPad* pad, GstEvent* event)
{
	GST_LOG ("");

	GstGooAudioFilter* self = GST_GOO_AUDIO_FILTER (gst_pad_get_parent (pad));

	gboolean ret;

	g_assert (self->component != NULL);

	switch (GST_EVENT_TYPE (event))
	{
	case GST_EVENT_EOS:
		GST_INFO ("EOS event");
		gst_goo_decwma_wait_for_done (self);
		ret = gst_pad_push_event (self->srcpad, event);
		break;
	default:
		ret = gst_pad_event_default (pad, event);
		break;
	}

	gst_object_unref (self);
	return ret;
}

static void
gst_goo_decwma_base_init (gpointer g_klass)
{
        GST_DEBUG_CATEGORY_INIT (gst_goo_decwma_debug, "goodecwma", 0,
                                 "OpenMAX Windows Media Audio decoder element");

        GstElementClass* e_klass = GST_ELEMENT_CLASS (g_klass);

		gst_element_class_add_pad_template (e_klass,
                                            gst_static_pad_template_get
                                            (&sink_factory));

        gst_element_class_add_pad_template (e_klass,
                                            gst_static_pad_template_get
                                            (&src_factory));

        gst_element_class_set_details (e_klass, &details);

        return;
}

static gboolean
gst_goo_decwma_check_fixed_src_caps (GstGooAudioFilter* filter)
{
	GstGooDecWma *self = GST_GOO_DECWMA (filter);
	GooTiWmaDec *component = GOO_TI_WMADEC (filter->component);
	GooTiAudioComponent *audio = GOO_TI_AUDIO_COMPONENT (filter->component);
	OMX_AUDIO_PARAM_PCMMODETYPE *param;
	GstCaps *src_caps;
	WMA_HeadInfo* pHeaderInfo = GOO_TI_AUDIO_COMPONENT (component)->audioinfo->wmaHeaderInfo;

	if (pHeaderInfo->iFormatTag == 0)
	{
		if (self->wmatxtfile)
			if (!g_file_test (self->wmatxtfile, G_FILE_TEST_IS_REGULAR))
			{
				 GST_ELEMENT_ERROR (self, RESOURCE, NOT_FOUND,
					("No such wmatxtfile \"%s\"", self->wmatxtfile),
					(NULL)
				);
				return FALSE;
			}

		GST_ELEMENT_ERROR (self, STREAM, FORMAT,
			("Audio header is invalid"),
			(NULL)
		);
		return FALSE;


	}

	param = GOO_TI_WMADEC_GET_OUTPUT_PARAM (component);
	src_caps = gst_caps_new_simple (
			"audio/x-raw-int",
			"endianness", G_TYPE_INT, G_BYTE_ORDER,
			"signed", G_TYPE_BOOLEAN, TRUE,
			"width", G_TYPE_INT, 16,
			"depth", G_TYPE_INT, 16,
			"rate", G_TYPE_INT, param->nSamplingRate,
			"channels", G_TYPE_INT, param->nChannels,
			NULL
			);

	gst_pad_set_caps (GST_GOO_AUDIO_FILTER (self)->srcpad, src_caps);
	gst_caps_unref (src_caps);

	return TRUE;
}

typedef struct __rca_header {
	guint8 data_packet_count[2][4]; //0
	guint8 play_duration[2][4]; //8
	guint8 max_data_packet_size[4]; //16
	struct {
		guint8 data1[4];
		guint8 data2[2];
		guint8 data3[2];
		guint8 data4[8];
	} stream_type_guid;//20
	guint8 type_specific_data_len[4]; //36
	guint8 stream_number[2]; //40
	guint8 format_tag[2]; //42
	guint8 channels[2]; //44
	guint8 samples_per_second[4]; //46
	guint8 avg_bytes_per_second[4]; //50
	guint8 block_align[2]; //54
	guint8 valid_bits_per_sample[2]; //56
	guint8 unused1[2]; //58
	guint8 samples_per_block[4]; //60
	guint8 encode_options[2]; //64
	guint8 unused2[4]; //66
} rca_header;

static GstBuffer*
gst_goo_decwma_extra_buffer_processing(GstGooAudioFilter *filter, GstBuffer *buffer)
{

	GstGooDecWma *self = GST_GOO_DECWMA (filter);
	GooTiAudioComponent *audio = GOO_TI_AUDIO_COMPONENT (filter->component);
	WMA_HeadInfo *pHeaderInfo = audio->audioinfo->wmaHeaderInfo;
	GstGooDecWmaPrivate *priv = GST_GOO_DECWMA_GET_PRIVATE (self);

	/* We process the codec data here since we need to component in
	     Executing so that we can send the header buffer to OMX */
	if (GST_IS_BUFFER (self->audio_header))
	{
		GstBuffer *header_buffer = NULL;
		if (self->parsed_header == FALSE)
		{
			int i;
			GST_DEBUG ("Parsing WMA RCA Header info");
			header_buffer = gst_buffer_new_and_alloc(70);
			rca_header *header = (rca_header *)GST_BUFFER_DATA(header_buffer);

			U32(header->data_packet_count[0]) = pHeaderInfo->iPackets.dwLo;
			U32(header->data_packet_count[1]) = pHeaderInfo->iPackets.dwHi;

			U32(header->play_duration[0]) = pHeaderInfo->iPlayDuration.dwLo;
			U32(header->play_duration[1]) = pHeaderInfo->iPlayDuration.dwHi;
			U32(header->max_data_packet_size) = pHeaderInfo->iMaxPacketSize;

			U32(header->stream_type_guid.data1) = pHeaderInfo->iStreamType.Data1;
			U16(header->stream_type_guid.data2) = pHeaderInfo->iStreamType.Data2;
			U16(header->stream_type_guid.data3) = pHeaderInfo->iStreamType.Data3;
			for(i=0; i<8; i++)
				header->stream_type_guid.data4[i] = pHeaderInfo->iStreamType.Data4[i];
			U32(header->type_specific_data_len) = pHeaderInfo->iTypeSpecific;
			U16(header->stream_number) = pHeaderInfo->iStreamNum;
			U16(header->format_tag) =  pHeaderInfo->iFormatTag;
			U32(header->samples_per_second) = pHeaderInfo->iSamplePerSec;
			U32(header->avg_bytes_per_second) = pHeaderInfo->iAvgBytesPerSec;
			U16(header->block_align) = pHeaderInfo->iBlockAlign;
			U16(header->channels) = pHeaderInfo->iChannel;
			U32(header->valid_bits_per_sample) = pHeaderInfo->iValidBitsPerSample;
			U16(header->encode_options) = pHeaderInfo->iEncodeOptV;
			U32(header->samples_per_block) = pHeaderInfo->iSamplePerBlock;
		}

		GstBuffer *frm_buffer;
		frm_buffer = gst_buffer_new_and_alloc (5);
		GST_BUFFER_DATA (frm_buffer)[0] = 0x00;
		U32(GST_BUFFER_DATA(frm_buffer)+1) = GST_BUFFER_SIZE(buffer);
		buffer = gst_buffer_join (frm_buffer, buffer);

		if (self->parsed_header == FALSE)
		{
			g_assert(header_buffer != NULL);
			GST_DEBUG ("Adding header %d bytes", 70);
			buffer = gst_buffer_join (header_buffer, buffer);
			self->parsed_header = TRUE;
		}
	}

	#if 0
	{
		static FILE *f = NULL;
		if(!f)
			f = fopen("dump.rca", "wb");
		g_print("Dumping buffer\n");
		fwrite(GST_BUFFER_DATA(buffer), GST_BUFFER_SIZE(buffer), 1, f);
		fflush(f);
	}
	#endif

	return buffer;

}


static void
gst_goo_decwma_class_init (GstGooDecWmaClass* klass)
{
	GObjectClass* g_klass = G_OBJECT_CLASS (klass);
	GParamSpec* pspec;
	GstElementClass* gst_klass;

	/* gobject */

	g_type_class_add_private (klass, sizeof (GstGooDecWmaPrivate));

	g_klass->set_property =
		GST_DEBUG_FUNCPTR (gst_goo_decwma_set_property);
	g_klass->dispose = GST_DEBUG_FUNCPTR (gst_goo_decwma_dispose);

	GParamSpec* spec;
	spec = g_param_spec_string ("wmatxtfile", "Header info file",
					"Header text info file",
					"", G_PARAM_WRITABLE);
	g_object_class_install_property (g_klass, PROP_WMATXTFILE, spec);


	/* GST GOO FILTER */
	GstGooAudioFilterClass* gst_c_klass = GST_GOO_AUDIO_FILTER_CLASS (klass);
	gst_c_klass->check_fixed_src_caps_func = GST_DEBUG_FUNCPTR (gst_goo_decwma_check_fixed_src_caps);
	gst_c_klass->set_process_mode_func = GST_DEBUG_FUNCPTR (gst_goo_decwma_process_mode);
	gst_c_klass->extra_buffer_processing_func = GST_DEBUG_FUNCPTR (gst_goo_decwma_extra_buffer_processing);

	return;
}

static void
gst_goo_decwma_init (GstGooDecWma* self, GstGooDecWmaClass* klass)
{
	GST_DEBUG ("");

	GST_GOO_AUDIO_FILTER (self)->component = goo_component_factory_get_component
		(GST_GOO_AUDIO_FILTER (self)->factory, GOO_TI_WMA_DECODER);

	GooComponent* component = GST_GOO_AUDIO_FILTER (self)->component;

	/* component default parameters */
	{
		/* input port */
                GOO_TI_WMADEC_GET_INPUT_PARAM (component)->nBitRate = 8000;
                GOO_TI_WMADEC_GET_INPUT_PARAM (component)->nChannels = CHANNELS_DEFAULT;
		/* output port */
	}

	/* input port */
	GST_GOO_AUDIO_FILTER (self)->inport = goo_component_get_port (component, "input0");
	g_assert (GST_GOO_AUDIO_FILTER (self)->inport != NULL);

	{
		GooPort* port = GST_GOO_AUDIO_FILTER (self)->inport;
		GOO_PORT_GET_DEFINITION (port)-> nBufferSize = INPUT_BUFFERSIZE_DEFAULT;
		GOO_PORT_GET_DEFINITION (port)-> format.audio.eEncoding = OMX_AUDIO_CodingWMA;
	}

	GST_GOO_AUDIO_FILTER (self)->outport = goo_component_get_port (component, "output0");
	g_assert (GST_GOO_AUDIO_FILTER (self)->outport != NULL);

	/* output port */
	{
		GooPort* port = GST_GOO_AUDIO_FILTER (self)->outport;

		GOO_PORT_GET_DEFINITION (port)-> nBufferSize = OUTPUT_BUFFERSIZE_DEFAULT;
		GOO_PORT_GET_DEFINITION (port)-> format.audio.eEncoding = OMX_AUDIO_CodingPCM;

		/** Use the PARENT's callback function **/
		goo_port_set_process_buffer_function (port, gst_goo_audio_filter_outport_buffer);

	}

	GstGooDecWmaPrivate* priv = GST_GOO_DECWMA_GET_PRIVATE (self);
	priv->incount = 0;
	priv->outcount = 0;

	self->wmatxtfile = NULL;
	self->audio_header = NULL;
	self->parsed_header = FALSE;

	/* Setcaps functions */
	gst_pad_set_setcaps_function (GST_GOO_AUDIO_FILTER(self)->sinkpad, gst_goo_decwma_sink_setcaps);
	gst_pad_set_setcaps_function (GST_GOO_AUDIO_FILTER(self)->srcpad, gst_goo_decwma_src_setcaps);
	gst_pad_set_event_function (GST_GOO_AUDIO_FILTER(self)->sinkpad, GST_DEBUG_FUNCPTR (gst_goo_decwma_sink_event));

	g_object_set_data (G_OBJECT (GST_GOO_AUDIO_FILTER (self)->component), "gst", self);
	g_object_set_data (G_OBJECT (self), "goo", GST_GOO_AUDIO_FILTER (self)->component);
	g_object_set(G_OBJECT (self), "process-mode", 1, NULL);

        return;
}

static gboolean
gst_goo_decwma_sink_setcaps (GstPad *pad, GstCaps *caps)
{
	GstStructure *structure;
	GstGooDecWma *self;
	GooComponent *component;
	GstGooDecWmaPrivate *priv;
	gchar *str_caps;
	guint sample_rate = SAMPLERATE_DEFAULT;
	guint channels = CHANNELS_DEFAULT;
	guint eFormat = OMX_AUDIO_WMAFormat8, wmaversion = 2;
	guint block_align = 3000;
	guint bitrate = 64000;
	guint depth = DEFAULT_DEPTH;


	self = GST_GOO_DECWMA (GST_PAD_PARENT (pad));
	component = (GooComponent *) GST_GOO_AUDIO_FILTER (self)->component;
	priv = GST_GOO_DECWMA_GET_PRIVATE (self);

	GST_DEBUG_OBJECT (self, "sink_setcaps");

	g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE );

	structure = gst_caps_get_structure (caps, 0);
	str_caps = gst_structure_to_string (structure);
	GST_DEBUG_OBJECT (self, "sink caps: %s", str_caps);
	g_free (str_caps);

	gst_structure_get_int (structure, "rate", &sample_rate);
	gst_structure_get_int (structure, "channels", &channels);
	gst_structure_get_int (structure, "wmaversion", &wmaversion);
	gst_structure_get_int (structure, "block_align", &block_align);
	gst_structure_get_int (structure, "bitrate", &bitrate);
	gst_structure_get_int (structure, "depth", &depth);

	/** Fill the private structure with the caps values  for use in
		codec data processing and extra	buffer processing **/
	priv->bitrate = bitrate;
	priv->channels = channels;
	priv->sample_rate = sample_rate;
	priv->block_align = block_align;
	priv->wmaversion = wmaversion;

	switch(wmaversion)
	{
		case 1:
			eFormat = OMX_AUDIO_WMAFormat7;
			break;
		case 2:
			eFormat = OMX_AUDIO_WMAFormat8;
			break;
		case 3:
			eFormat = OMX_AUDIO_WMAFormat9;
			break;
	}

	GOO_TI_WMADEC_GET_INPUT_PARAM (component)->nChannels = channels;
	GOO_TI_WMADEC_GET_INPUT_PARAM (component)->eFormat = eFormat;

	/* Get Header info */
	{
		gchar *str;
		const GValue *value;
		str = gst_structure_to_string (structure);
		GST_DEBUG_OBJECT (self, "set_caps (sink) = %s", str);
		g_free (str);
		value = gst_structure_get_value (structure, "codec_data");

		if (value)
		{
			GST_DEBUG_OBJECT (self, "Got codec_data");
			self->audio_header = gst_value_get_buffer (value);
		}

	}

#if 1
	/* Set component's audio header */
	{
		WMA_HeadInfo* pHeaderInfo;
		pHeaderInfo = GOO_TI_AUDIO_COMPONENT (component)->audioinfo->wmaHeaderInfo;

		if(pHeaderInfo->iFormatTag == 0)
		{
		        pHeaderInfo->iFormatTag = wmaversion + GST_RIFF_WAVE_FORMAT_WMAV1 - 1;
		        pHeaderInfo->iSamplePerSec = sample_rate;
		        pHeaderInfo->iAvgBytesPerSec  = bitrate >> 3;
		        pHeaderInfo->iBlockAlign = block_align;
		        pHeaderInfo->iChannel = channels;
		        pHeaderInfo->iValidBitsPerSample = depth;
		        pHeaderInfo->iMaxPacketSize = 64000;
			pHeaderInfo->iTypeSpecific = 28;
			pHeaderInfo->iStreamNum = 1;
			goo_component_set_config_by_name (component,
                                GOO_TI_AUDIO_COMPONENT (component)->dasf_param_name,
                                GOO_TI_AUDIO_COMPONENT (component)->audioinfo);
                        if(self->audio_header)
                        {
                        	guint8 *codec_data = GST_BUFFER_DATA(self->audio_header);
				pHeaderInfo->iSamplePerBlock= U32(codec_data);
				pHeaderInfo->iEncodeOptV = U16(codec_data+4);
                        }
		}
	}
#endif
	return gst_pad_set_caps (pad, caps);
}

static gboolean
gst_goo_decwma_src_setcaps (GstPad *pad, GstCaps *caps)
{
	GstStructure *structure;
	GstGooDecWma *self;
	GooPort *outport;
	guint width = DEFAULT_WIDTH;
	guint depth = DEFAULT_DEPTH;
	gchar *str_caps;

	self = GST_GOO_DECWMA (GST_PAD_PARENT (pad));
	outport = (GooPort *) GST_GOO_AUDIO_FILTER (self)->outport;

	GST_DEBUG_OBJECT (self, "src_setcaps");
	GST_LOG ("caps are %" GST_PTR_FORMAT, caps);
	g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE);

	structure = gst_caps_get_structure (caps, 0);

	gst_structure_get_int (structure, "width", &width);
	gst_structure_get_int (structure, "depth", &depth);

	return gst_pad_set_caps (pad, caps);
}
