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

#include <gst/interfaces/mixer.h>
#include "gstdasfsrc.h"
#include "gstgoobuffer.h"
#include <string.h>

#define NUM_OUTPUT_BUFFERS_DEFAULT 1

GST_DEBUG_CATEGORY_STATIC (gst_dasf_src_debug);
#define GST_CAT_DEFAULT gst_dasf_src_debug

/* signals */
enum
{
        LAST_SIGNAL
};

/* args */
enum
{
	PROP_0
		/* PROP_NUM_OUTPUT_BUFFERS, */
};

#define GST_DASF_SRC_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_DASF_SRC, GstDasfSrcPrivate))
static GstClockTime
gst_dasf_src_get_time (GstClock * clock, GstDasfSrc * src);

struct _GstDasfSrcPrivate
{
        guint num_output_buffers;
		guint incount;
		guint outcount;
        guint volume;
        gboolean mute;
};

static const GstElementDetails details =
        GST_ELEMENT_DETAILS (
                "OpenMAX DASF src",
                "Src/Audio",
                "Input sound via DASF",
                "Texas Instrument"
                );

static GstStaticPadTemplate src_factory =
        GST_STATIC_PAD_TEMPLATE (
                "src",
                GST_PAD_SRC,
                GST_PAD_ALWAYS,
				GST_STATIC_CAPS ("audio/x-raw-int, "
                                 "signed = (boolean) true, "
                                 "width = (int) [ 8, 32 ], "
                                 "depth = (int) [ 8, 32 ], "
                                 "rate = (int) { 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 56000, 64000, 80000, 96000 },"
                                 "channels = (int) [ 1, 8 ]")
                );




static gboolean
gst_dasf_src_interface_supported (GstImplementsInterface *iface,
                                   GType iface_type)
{
        g_return_val_if_fail (iface_type == GST_TYPE_MIXER, FALSE);

        return TRUE;
}

static void
gst_dasf_src_implements_interface_init (GstImplementsInterfaceClass* iface)
{
        iface->supported =
                GST_DEBUG_FUNCPTR (gst_dasf_src_interface_supported);

        return;
}

static const GList*
gst_dasf_src_list_tracks (GstMixer *mixer)
{
        GstDasfSrc* self = GST_DASF_SRC (mixer);

        return (const GList *) self->tracks;
}

static void
gst_dasf_src_set_volume (GstMixer* mixer, GstMixerTrack* track,
                          gint* volumes)
{
        GstDasfSrc* self = GST_DASF_SRC (mixer);
        GstDasfSrcPrivate* priv = GST_DASF_SRC_GET_PRIVATE (self);

        priv->volume = volumes[0];

        GST_INFO ("Volume set to %d", priv->volume);

        if (self->component != NULL)
        {
                goo_ti_audio_component_set_volume (self->component,
                                                   priv->volume);
        }

        return;
}

static void
gst_dasf_src_get_volume (GstMixer* mixer, GstMixerTrack* track,
                          gint* volumes)
{
        GstDasfSrc* self = GST_DASF_SRC (mixer);
        GstDasfSrcPrivate* priv = GST_DASF_SRC_GET_PRIVATE (self);

        GST_LOG ("");

        if (self->component != NULL)
        {
                priv->volume =
                        goo_ti_audio_component_get_volume (self->component);
        }

        volumes[0] = priv->volume;

        return;
}

static void
gst_dasf_src_set_mute (GstMixer* mixer, GstMixerTrack* track, gboolean mute)
{
        GstDasfSrc* self = GST_DASF_SRC (mixer);
        GstDasfSrcPrivate* priv = GST_DASF_SRC_GET_PRIVATE (self);

        GST_LOG ("");

        priv->mute = mute;

        if (self->component != NULL)
        {
                goo_ti_audio_component_set_mute (self->component,
                                                 priv->mute);
        }

        return;
}

static void
gst_dasf_src_mixer_interface_init (GstMixerClass* iface)
{
        GST_MIXER_TYPE (iface) = GST_MIXER_HARDWARE;

        iface->list_tracks = GST_DEBUG_FUNCPTR (gst_dasf_src_list_tracks);
        iface->set_volume = GST_DEBUG_FUNCPTR (gst_dasf_src_set_volume);
        iface->get_volume = GST_DEBUG_FUNCPTR (gst_dasf_src_get_volume);

	return;
}

static void
gst_dasf_enable (GstDasfSrc* self)
{
	GST_INFO ("");
	GstPad *peer;
	GstElement *next_element;
	GooComponent *component;
	GstBaseSrc *base_src;

	if (self->component != NULL)
	{
		return;
	}

	peer = gst_pad_get_peer (GST_BASE_SRC_PAD (self));

	if (G_UNLIKELY (peer == NULL))
	{
		GST_INFO ("No next pad");
		return;
	}

	next_element = GST_ELEMENT (gst_pad_get_parent (peer));

	if (G_UNLIKELY (next_element == NULL))
	{
		GST_INFO ("Cannot find a next element");
		goto done;
	}

	/** expecting a capsfilter between dasfsrc and goo audio component **/
	while (GST_IS_BASE_TRANSFORM (next_element))
	{
		GST_DEBUG_OBJECT(self, "next element name: %s", gst_element_get_name (next_element));

		gst_object_unref (peer);
		peer = gst_pad_get_peer (GST_BASE_TRANSFORM_SRC_PAD (next_element));
		gst_object_unref (next_element);
		next_element = GST_ELEMENT(gst_pad_get_parent (peer)) ;

		GST_DEBUG_OBJECT (self, "one after element name: %s", gst_element_get_name(next_element));
	}

	/** capsfilter might be found
	 *  element next to the caps filter should be goo **/

	component = GOO_COMPONENT (g_object_get_data
							   (G_OBJECT (next_element), "goo"));

	if (G_UNLIKELY (component == NULL))
	{
		GST_INFO ("Previous element does not have a Goo component");
		goto done;
	}

	if (!GOO_IS_TI_AUDIO_COMPONENT (component))
	{
		GST_WARNING ("The component in previous element is not TI Audio");
		goto done;
	}

	self->component = GOO_TI_AUDIO_COMPONENT (component);
	goo_ti_audio_component_set_dasf_mode (self->component, TRUE);
	GST_DEBUG_OBJECT (self, "set data path");
	goo_ti_audio_component_set_data_path (self->component, 0);

	/** getting num-buffers from base src **/
	base_src = GST_BASE_SRC (self);
	goo_ti_audio_encoder_set_number_buffers (GOO_TI_AUDIO_ENCODER (component), base_src->num_buffers);

done:
	gst_object_unref (peer);
	gst_object_unref (next_element);

	GST_DEBUG_OBJECT (self, "peer refcount = %d",
			  G_OBJECT (peer)->ref_count);

	GST_DEBUG_OBJECT (self, "next element refcount = %d",
			  G_OBJECT (next_element)->ref_count);

	return;
}

static void
gst_dasf_change_peer_omx_state (GstDasfSrc* self)
{
	GST_INFO ("");
	GstPad *peer;
	GstElement *next_element;
	GooComponent *component;
	GstBaseSrc *base_src;

	peer = gst_pad_get_peer (GST_BASE_SRC_PAD (self));

	if (G_UNLIKELY (peer == NULL))
	{
		GST_INFO ("No next pad");
		return;
	}

	next_element = GST_ELEMENT (gst_pad_get_parent (peer));

	if (G_UNLIKELY (next_element == NULL))
	{
		GST_INFO ("Cannot find a next element");
		gst_object_unref (next_element);
		return;
	}

	/** expecting a capsfilter between dasfsrc and goo audio component **/
	while (GST_IS_BASE_TRANSFORM (next_element))
	{
		GST_DEBUG_OBJECT(self, "next element name: %s", gst_element_get_name (next_element));

		gst_object_unref (peer);
		peer = gst_pad_get_peer (GST_BASE_TRANSFORM_SRC_PAD (next_element));
		gst_object_unref (next_element);
		next_element = GST_ELEMENT(gst_pad_get_parent (peer)) ;

		GST_DEBUG_OBJECT (self, "one after element name: %s", gst_element_get_name(next_element));
	}

	/** capsfilter might be found
	 *  element next to the caps filter should be goo **/

	component = GOO_COMPONENT (g_object_get_data
							   (G_OBJECT (next_element), "goo"));

	if (G_UNLIKELY (component == NULL))
	{
		GST_INFO ("Previous element does not have a Goo component");
		gst_object_unref (peer);
		gst_object_unref (next_element);
		return;
	}

	if (!GOO_IS_TI_AUDIO_COMPONENT (component))
	{
		GST_WARNING ("The component in previous element is not TI Audio");
		gst_object_unref (peer);
		gst_object_unref (next_element);
		return;
	}

	self->peer_element = g_object_ref (GST_GOO_AUDIO_FILTER (next_element));

 	/* Work with a queue on the output buffers */
	g_object_set (GST_GOO_AUDIO_FILTER (next_element)->component, "outport-queue", TRUE, NULL);

	/** This fixates the caps on the next goo element to configure the output omx port  **/
	gst_goo_audio_filter_check_fixed_src_caps (GST_GOO_AUDIO_FILTER (next_element));

	g_object_set (GST_GOO_AUDIO_FILTER (next_element)->inport,
		"buffercount",
		1, NULL);
	g_object_set (GST_GOO_AUDIO_FILTER (next_element)->outport,
		"buffercount",
		1, NULL);

	GST_INFO ("Setting peer omx component to idle");
	goo_component_set_state_idle (GST_GOO_AUDIO_FILTER (next_element)->component);
	GST_INFO ("Setting peer omx component to executing");
	goo_component_set_state_executing (GST_GOO_AUDIO_FILTER (next_element)->component);

	gst_object_unref (peer);
	gst_object_unref (next_element);

	GST_DEBUG_OBJECT (self, "peer refcount = %d",
			  G_OBJECT (peer)->ref_count);

	GST_DEBUG_OBJECT (self, "next element refcount = %d",
			  G_OBJECT (next_element)->ref_count);

	return;

}

/** This group of functions need to be overriden so that the ring
 * buffer becomes useless in our implementation**/

static gboolean
gst_dasf_src_open_device (GstAudioSrc *src)
{
        GstDasfSrc* self = GST_DASF_SRC (src);
        gboolean ret = TRUE;

        GST_LOG ("No need to open device");

        return ret;
}

static gboolean
gst_dasf_src_prepare_device (GstAudioSrc *src, GstRingBufferSpec *spec)
{
        GstDasfSrc* self = GST_DASF_SRC (src);
        gboolean ret = TRUE;

        GST_LOG ("No need to prepare device");

        return ret;
}

static gboolean
gst_dasf_src_unprepare_device (GstAudioSrc *src )
{
        GstDasfSrc* self = GST_DASF_SRC (src);
        gboolean ret = TRUE;

        GST_LOG ("No need to unprepare device");

        return ret;
}

static gboolean
gst_dasf_src_close_device (GstAudioSrc *src)
{
        GstDasfSrc* self = GST_DASF_SRC (src);
        gboolean ret = TRUE;

        GST_LOG ("No need to close device");

        return ret;
}

static guint
gst_dasf_src_read_device (GstAudioSrc *src, gpointer data, guint length)
{
        GstDasfSrc* self = GST_DASF_SRC (src);
        guint ret = 0;

        GST_LOG ("Writting %d bytes", ret);

        return ret;
}

static guint
gst_dasf_src_delay_device (GstAudioSrc *src)
{
        GstDasfSrc* self = GST_DASF_SRC (src);
        guint ret = 0;

        GST_LOG ("Delaying %d bytes", ret);

        return ret;
}

static void
gst_dasf_src_reset_device (GstAudioSrc *src)
{
        GstDasfSrc* self = GST_DASF_SRC (src);
        guint ret = 0;

        GST_LOG ("Reset Device");

}

static GstCaps *
gst_dasf_src_getcaps (GstBaseSrc *base_src)
{
	GstDasfSrc *self = GST_DASF_SRC (base_src);
	GstCaps *caps;
	gchar *caps_str;

	GST_DEBUG ("Enter");

	caps = gst_caps_copy (gst_pad_get_pad_template_caps
						  (GST_BASE_SRC_PAD (base_src)));

	caps_str = gst_caps_to_string (caps);
	GST_DEBUG_OBJECT (self, "src_caps: %s ", caps_str);


	return caps;
}

static GstFlowReturn
gst_dasf_src_create (GstAudioSrc *audiosrc,
						guint64 offset,
						guint length,
						GstBuffer **buffer)
{
	GstDasfSrc* self = GST_DASF_SRC (audiosrc);
	GstBaseAudioSrc *baseaudiosrc = GST_BASE_AUDIO_SRC (self);
	GstBuffer* gst_buffer = NULL;
	OMX_BUFFERHEADERTYPE* omx_buffer = NULL;
	GstDasfSrcPrivate* priv = GST_DASF_SRC_GET_PRIVATE (self);
	GstGooAudioFilter* me = self->peer_element;

	GST_DEBUG ("");

	if (me->component->cur_state != OMX_StateExecuting)
	{
		return GST_FLOW_UNEXPECTED;
	}

	GST_DEBUG ("goo stuff");
	{
		omx_buffer = goo_port_grab_buffer (me->outport);

		if (gst_pad_alloc_buffer (GST_BASE_SRC_PAD (self),
					  priv->outcount,
					  omx_buffer->nFilledLen,
					  GST_PAD_CAPS (GST_BASE_SRC_PAD (self)),
					  &gst_buffer) == GST_FLOW_OK)
		{
			if (GST_IS_GOO_BUFFER (gst_buffer))
			{
				memcpy (GST_BUFFER_DATA (gst_buffer),
					omx_buffer->pBuffer,
					omx_buffer->nFilledLen);

				goo_component_release_buffer (me->component,
							      omx_buffer);
			}
			else
 			{
				gst_buffer_unref (gst_buffer);
				gst_buffer = (GstBuffer*) gst_goo_buffer_new ();
				gst_goo_buffer_set_data (gst_buffer,
							 me->component,
							 omx_buffer);
			}
		}
		else
		{
			goto fail;
		}
	}


	GST_DEBUG ("gst stuff");
	{
		GstClock* clock = NULL;
		GstClockTime timestamp, duration;
		clock = gst_element_get_clock (GST_ELEMENT (self));

		timestamp = gst_clock_get_time (clock);
		timestamp -= gst_element_get_base_time (GST_ELEMENT (self));
		gst_object_unref (clock);

		GST_BUFFER_TIMESTAMP (gst_buffer) = gst_util_uint64_scale_int (GST_SECOND, priv->outcount, 50);

		/* Set 20 millisecond duration */
		duration = gst_util_uint64_scale_int
			(GST_SECOND,
			 1,
			 50);

		GST_BUFFER_DURATION (gst_buffer) = duration;
		GST_BUFFER_OFFSET (gst_buffer) = priv->outcount++;
		GST_BUFFER_OFFSET_END (gst_buffer) = priv->outcount;

		gst_buffer_set_caps (gst_buffer,
		GST_PAD_CAPS (GST_BASE_SRC_PAD (self)));
	}

beach:
	*buffer = gst_buffer;
	return GST_FLOW_OK;

fail:
	if (G_LIKELY (*buffer))
	{
		gst_buffer_unref (*buffer);
	}

	return GST_FLOW_ERROR;
}


static void
_do_init (GType dasfsrc_type)
{
        static const GInterfaceInfo implements_interface_info =
                {
                        (GInterfaceInitFunc) gst_dasf_src_implements_interface_init,
                        NULL,
                        NULL
                };

        static const GInterfaceInfo mixer_interface_info =
                {
                        (GInterfaceInitFunc) gst_dasf_src_mixer_interface_init,
                        NULL,
                        NULL
                };

        g_type_add_interface_static (dasfsrc_type,
                                     GST_TYPE_IMPLEMENTS_INTERFACE,
                                     &implements_interface_info);

        g_type_add_interface_static (dasfsrc_type,
                                     GST_TYPE_MIXER,
                                     &mixer_interface_info);

        return;
}

GST_BOILERPLATE_FULL (GstDasfSrc, gst_dasf_src, GstAudioSrc,
                      GST_TYPE_AUDIO_SRC, _do_init);

static GstStateChangeReturn
gst_dasf_src_change_state (GstElement* element, GstStateChange transition)
{
        GST_LOG ("");

        GstDasfSrc* self = GST_DASF_SRC (element);

        switch (transition)
        {
        case GST_STATE_CHANGE_NULL_TO_READY:
                gst_dasf_enable (self);
                break;
		case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
				gst_dasf_change_peer_omx_state (self);
				break;
        default:
                break;
        }

        return GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
}

static gboolean
gst_dasf_src_check_get_range (GstBaseSrc *audiosrc)
{
	return FALSE;
}

static void
gst_dasf_src_get_times (GstBaseSrc *audiosrc,
						GstBuffer *buffer,
						GstClockTime *start,
						GstClockTime *end)
{
	GstDasfSrc *dasf_src;
	GstClockTime timestamp, duration;

	dasf_src = GST_DASF_SRC (audiosrc);
	timestamp = GST_BUFFER_TIMESTAMP (buffer);

	if (GST_CLOCK_TIME_IS_VALID (timestamp))
	{
		duration = GST_BUFFER_DURATION (buffer);

		if (GST_CLOCK_TIME_IS_VALID (duration))
		{
			*end = timestamp + duration;
		}
		*start = timestamp;
	}

}


#if 0
static void
gst_dasf_src_set_property (GObject* object, guint prop_id,
						   const GValue* value, GParamSpec* pspec)
{
	GstDasfSrcPrivate* priv = GST_DASF_SRC_GET_PRIVATE (object);

	switch (prop_id)
	{
		case PROP_NUM_OUTPUT_BUFFERS:
		priv->num_output_buffers = g_value_get_uint (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_dasf_src_get_property (GObject* object, guint prop_id,
                            GValue* value, GParamSpec* pspec)
{
	GstDasfSrcPrivate* priv = GST_DASF_SRC_GET_PRIVATE (object);

	switch (prop_id)
	{
	case PROP_NUM_OUTPUT_BUFFERS:
		g_value_set_uint (value, priv->num_output_buffers);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}
#endif
static void
gst_dasf_src_dispose (GObject* object)
{
        GstDasfSrc* self = GST_DASF_SRC (object);

        G_OBJECT_CLASS (parent_class)->dispose (object);

        GSList* l;
        for (l = self->tracks; l != NULL; l = l->next)
        {
                g_object_unref (GST_MIXER_TRACK (l->data));
        }

        if (self->tracks != NULL)
        {
                g_slist_free (self->tracks);
                self->tracks = NULL;
        }


		GST_DEBUG ("GOO component refcount = %d",
				  G_OBJECT (self->peer_element)->ref_count);

		g_object_unref (self->peer_element);


        self->component = NULL;
}

static void
gst_dasf_src_base_init (gpointer g_klass)
{
        GST_DEBUG_CATEGORY_INIT (gst_dasf_src_debug, "dasfsrc", 0,
                                 "TI DASF src element");

        GstElementClass* e_klass = GST_ELEMENT_CLASS (g_klass);

        gst_element_class_add_pad_template (e_klass,
                                            gst_static_pad_template_get
                                            (&src_factory));

        gst_element_class_set_details (e_klass, &details);

        return;
}

static void
gst_dasf_src_class_init (GstDasfSrcClass* klass)
{
	GObjectClass* g_klass;
	GParamSpec* pspec;
	GstElementClass *gst_element_klass;
	GstBaseSrcClass* gst_base_klass;
	GstAudioSrcClass *gst_audio_klass;

	/* gobject */
	g_klass = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GstDasfSrcPrivate));

#if 0
	g_klass->set_property =
		GST_DEBUG_FUNCPTR (gst_dasf_src_set_property);
	g_klass->get_property =
		GST_DEBUG_FUNCPTR (gst_dasf_src_get_property);

	pspec = g_param_spec_uint ("output-buffers", "output buffers",
				   "The number of OMX output buffers",
				   1, 10, NUM_OUTPUT_BUFFERS_DEFAULT,
				   G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_NUM_OUTPUT_BUFFERS,
					 pspec);

#endif

	g_klass->dispose = GST_DEBUG_FUNCPTR (gst_dasf_src_dispose);

	/** GST ELEMENT **/


	/** GST AUDIO SRC **/
	gst_audio_klass = GST_AUDIO_SRC_CLASS (klass);

	gst_audio_klass->open = GST_DEBUG_FUNCPTR (gst_dasf_src_open_device);
	gst_audio_klass->prepare =
		GST_DEBUG_FUNCPTR (gst_dasf_src_prepare_device);
	gst_audio_klass->unprepare =
		GST_DEBUG_FUNCPTR (gst_dasf_src_unprepare_device);
	gst_audio_klass->close =
		GST_DEBUG_FUNCPTR (gst_dasf_src_close_device);
	gst_audio_klass->read =
		GST_DEBUG_FUNCPTR (gst_dasf_src_read_device);
	gst_audio_klass->delay =
		GST_DEBUG_FUNCPTR (gst_dasf_src_delay_device);
	gst_audio_klass->reset =
		GST_DEBUG_FUNCPTR (gst_dasf_src_reset_device);

	/** GST BASE SRC **/
	gst_base_klass = GST_BASE_SRC_CLASS (klass);

	gst_base_klass->get_caps = GST_DEBUG_FUNCPTR (gst_dasf_src_getcaps);

	gst_base_klass->create = (void *)
		GST_DEBUG_FUNCPTR (gst_dasf_src_create);

	/** @todo When using seek functionality, consider using basesrc method **/

	gst_base_klass->check_get_range = (void *)
		GST_DEBUG_FUNCPTR (gst_dasf_src_check_get_range);

	gst_base_klass->get_times = (void *)
		GST_DEBUG_FUNCPTR (gst_dasf_src_get_times);

	GST_ELEMENT_CLASS (klass)->change_state =
		GST_DEBUG_FUNCPTR (gst_dasf_src_change_state);






	return;
}

static void
gst_dasf_src_init (GstDasfSrc* self, GstDasfSrcClass* klass)
{
	GST_DEBUG ("");

	GstBaseAudioSrc *baseaudiosrc;
	baseaudiosrc = GST_BASE_AUDIO_SRC (self);

	GstDasfSrcPrivate* priv = GST_DASF_SRC_GET_PRIVATE (self);
	priv->num_output_buffers = NUM_OUTPUT_BUFFERS_DEFAULT;
	priv->incount = 0;
	priv->outcount = 0;
	priv->volume = 100;
	priv->mute = FALSE;

	baseaudiosrc->clock = gst_audio_clock_new ("GstDasfSrcClock", (GstAudioClockGetTimeFunc) gst_dasf_src_get_time, self);

	self->component = NULL;

	self->tracks = NULL;

	GstMixerTrack* track = g_object_new (GST_TYPE_MIXER_TRACK, NULL);
	track->label = g_strdup ("DASF Volume");
	track->num_channels = 1;
	track->min_volume = 0;
	track->max_volume = 100;
	track->flags = GST_MIXER_TRACK_MASTER | GST_MIXER_TRACK_MUTE;
	self->tracks = g_slist_append (self->tracks, track);

	return;
}



static GstClockTime
gst_dasf_src_get_time (GstClock * clock, GstDasfSrc * src)
{
	GstClockTime dasfsrcclock;
	static	guint outcount=0;

	GST_DEBUG ("Enter");
	dasfsrcclock = gst_util_uint64_scale_int (GST_SECOND, 160, 8000)*outcount;
	outcount++;
	GST_DEBUG ( "system clock: %" GST_TIME_FORMAT, GST_TIME_ARGS (dasfsrcclock));
	return dasfsrcclock;

}


