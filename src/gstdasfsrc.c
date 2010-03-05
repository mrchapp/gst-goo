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
#include <goo-ti-audio-component.h>
#include "gstdasfsrc.h"
#include "gstgoobuffer.h"
#include "gstgooutils.h"
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

#define GST_DASF_SRC_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_DASF_SRC, GstDasfSrcPrivate))

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
                                 "channels = (int) [ 1, 8 ]"));




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
	g_assert (self);

	GST_INFO ("");

	self->component = gst_goo_util_find_goo_component (
		GST_ELEMENT (self), GOO_TYPE_TI_AUDIO_COMPONENT);

	GST_INFO ("Audio component found %x", self->component);

	g_assert (self->component);

	GST_INFO("Changing to DASF mode");
	goo_ti_audio_component_set_dasf_mode (self->component, TRUE);

	GST_DEBUG_OBJECT (self, "set data path");
	goo_ti_audio_component_set_data_path (self->component, 0);

done:


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

	GST_DEBUG ("");

	GST_INFO_OBJECT (self, "Ghost Buffer creation");
  gst_buffer = GST_BUFFER (gst_ghost_buffer_new ());

	GST_DEBUG_OBJECT (self, "setting caps on ghost buffer");
	gst_buffer_set_caps (gst_buffer,
				     GST_PAD_CAPS (GST_BASE_SRC_PAD (self)));

	priv->outcount++;

	// Buffers have no timestamp/duration,
	// audio encoder will take care of this.
	GST_BUFFER_TIMESTAMP (gst_buffer) = GST_CLOCK_TIME_NONE;
	GST_BUFFER_DURATION (gst_buffer) = GST_CLOCK_TIME_NONE;

	GST_DEBUG_OBJECT(self,"setting the offset");
	{
		GST_BUFFER_OFFSET (gst_buffer) = priv->outcount;
		GST_BUFFER_OFFSET_END (gst_buffer) = priv->outcount;
	}

	GST_DEBUG_OBJECT (self, "dasfsrc create");
	*buffer = gst_buffer;
	return GST_FLOW_OK;

}

static gboolean
gst_goo_dasf_src_src_event (GstPad *pad, GstEvent *event)
{
	g_assert (pad);
	g_assert (event);

	GST_INFO ("dasfsrc %s", GST_EVENT_TYPE_NAME (event));
	gboolean ret;

	switch (GST_EVENT_TYPE (event))
	{
		case GST_EVENT_EOS:
			GST_INFO ("EOS event");
		default:
			ret = gst_pad_event_default (pad, event);
			break;
	}

	return ret;
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
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

  GST_LOG ("transition %d", transition);

  GstDasfSrc* self = GST_DASF_SRC (element);

  switch (transition)
  {
    case GST_STATE_CHANGE_NULL_TO_READY:
    {
      gst_dasf_enable (self);
      break;
    }
    default:
    {
      break;
    }
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  /* downwards state changes */


  return ret;


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
	GObjectClass* gobject_class;
	GstBaseSrcClass* gstbasesrc_class;
	GstAudioSrcClass *gstaudiosrc_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstbasesrc_class = GST_BASE_SRC_CLASS (klass);
  gstaudiosrc_class = GST_AUDIO_SRC_CLASS (klass);


	g_type_class_add_private (klass, sizeof (GstDasfSrcPrivate));


	gobject_class->dispose = GST_DEBUG_FUNCPTR (gst_dasf_src_dispose);
#if 0
	gobject_class->set_property = GST_DEBUG_FUNCPTR (gst_dasf_src_set_property);
	gobject_class->get_property = GST_DEBUG_FUNCPTR (gst_dasf_src_get_property);
#endif

	gstbasesrc_class->get_caps = GST_DEBUG_FUNCPTR (gst_dasf_src_getcaps);
	gstbasesrc_class->create = GST_DEBUG_FUNCPTR (gst_dasf_src_create);
	/** @todo When using seek functionality, consider using basesrc method **/
	gstbasesrc_class->check_get_range = GST_DEBUG_FUNCPTR (gst_dasf_src_check_get_range);
	gstbasesrc_class->get_times = GST_DEBUG_FUNCPTR (gst_dasf_src_get_times);


	gstaudiosrc_class->open = GST_DEBUG_FUNCPTR (gst_dasf_src_open_device);
	gstaudiosrc_class->prepare = GST_DEBUG_FUNCPTR (gst_dasf_src_prepare_device);
	gstaudiosrc_class->unprepare = GST_DEBUG_FUNCPTR (gst_dasf_src_unprepare_device);
	gstaudiosrc_class->close = GST_DEBUG_FUNCPTR (gst_dasf_src_close_device);
	gstaudiosrc_class->read =	GST_DEBUG_FUNCPTR (gst_dasf_src_read_device);
	gstaudiosrc_class->delay = GST_DEBUG_FUNCPTR (gst_dasf_src_delay_device);
	gstaudiosrc_class->reset = GST_DEBUG_FUNCPTR (gst_dasf_src_reset_device);

	GST_ELEMENT_CLASS (klass)->change_state = GST_DEBUG_FUNCPTR (gst_dasf_src_change_state);



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

	gst_base_src_set_live (GST_BASE_SRC (self), TRUE);

	gst_pad_set_event_function (GST_BASE_SRC (self)->srcpad,
		GST_DEBUG_FUNCPTR (gst_goo_dasf_src_src_event));

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
