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
#include <gst/base/gstbasetransform.h>
#include <goo-ti-video-decoder.h>

#include "gstdasfsink.h"
#include "gstgoovideofilter.h"

#define NUM_INPUT_BUFFERS_DEFAULT 4

GST_DEBUG_CATEGORY_STATIC (gst_dasf_sink_debug);
#define GST_CAT_DEFAULT gst_dasf_sink_debug

/* signals */
enum
{
	LAST_SIGNAL
};

/* args */
enum
{
	PROP_0,
	PROP_NUM_INPUT_BUFFERS,
	PROP_CLOCK_SOURCE,
	PROP_CLOCK_REQUIRED,
	PROP_HALTED
};

#define GSTREAMER_CLOCK 0
#define OMX_CLOCK 1
#define AUTO_CLOCK 2

#define GST_DASF_SINK_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_DASF_SINK, GstDasfSinkPrivate))

struct _GstDasfSinkPrivate
{
	guint num_input_buffers;
	guint incount;
	guint volume;
	gboolean mute;

	guint clock_source;
	guint clock_required;
	gboolean first_time_playing;
};

#define GST_DASF_SINK_CLOCK_SOURCE \
	(gst_dasf_sink_clock_source_get_type ())

static GType
gst_dasf_sink_clock_source_get_type ()
{
	static GType gst_dasf_sink_clock_source_type = 0;
	static GEnumValue gst_dasf_sink_clock_source[] =
	{
		{ GSTREAMER_CLOCK,	"0", "GStreamer clock source" },
		{ OMX_CLOCK,   "1", "OMX Clock source" },
		{ AUTO_CLOCK,   "2", "Automatic Clock source" },
		{ 0, NULL, NULL },
	};

	if (!gst_dasf_sink_clock_source_type)
	{
		gst_dasf_sink_clock_source_type =
			g_enum_register_static ("GstDasfSinkClockSource",
						gst_dasf_sink_clock_source);
	}

	return gst_dasf_sink_clock_source_type;
}

static const GstElementDetails details =
	GST_ELEMENT_DETAILS (
		"OpenMAX DASF sink",
		"Sink/Audio",
		"Output sound via DASF",
		"Texas Instrument"
		);

static GstStaticPadTemplate sink_factory =
	GST_STATIC_PAD_TEMPLATE (
		"sink",
		GST_PAD_SINK,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS ("audio/x-raw-int, "
				 "rate = (int) { 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000 }, "
				 "channels = (int) { 1, 2 }, "
				 "endiannes = (int) BYTE_ORDER, "
				 "width = (int) 16, "
				 "depth = (int) 16, "
				 "signed = (boolean) true")
		);

static gboolean
gst_dasf_sink_interface_supported (GstImplementsInterface *iface,
				   GType iface_type)
{
	g_return_val_if_fail (iface_type == GST_TYPE_MIXER, FALSE);

	return TRUE;
}

static void
gst_dasf_sink_implements_interface_init (GstImplementsInterfaceClass* iface)
{
	iface->supported =
		GST_DEBUG_FUNCPTR (gst_dasf_sink_interface_supported);

	return;
}

static const GList*
gst_dasf_sink_list_tracks (GstMixer *mixer)
{
	GstDasfSink* self = GST_DASF_SINK (mixer);

	return (const GList *) self->tracks;
}

static void
gst_dasf_sink_set_volume (GstMixer* mixer, GstMixerTrack* track,
			  gint* volumes)
{
	GstDasfSink* self = GST_DASF_SINK (mixer);
	GstDasfSinkPrivate* priv = GST_DASF_SINK_GET_PRIVATE (self);

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
gst_dasf_sink_get_volume (GstMixer* mixer, GstMixerTrack* track,
			  gint* volumes)
{
	GstDasfSink* self = GST_DASF_SINK (mixer);
	GstDasfSinkPrivate* priv = GST_DASF_SINK_GET_PRIVATE (self);

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
gst_dasf_sink_set_mute (GstMixer* mixer, GstMixerTrack* track, gboolean mute)
{
	GstDasfSink* self = GST_DASF_SINK (mixer);
	GstDasfSinkPrivate* priv = GST_DASF_SINK_GET_PRIVATE (self);

	GST_LOG ("");

	priv->mute = mute;

	if (self->component != NULL)
	{
		goo_ti_audio_component_set_mute (self->component,
						 priv->mute);

		if (mute)
		{
    			track->flags |= GST_MIXER_TRACK_MUTE;
  		}
		else
		{
    			track->flags &= ~GST_MIXER_TRACK_MUTE;
  		}
	}

	return;
}

static void
gst_dasf_sink_mixer_interface_init (GstMixerClass* iface)
{
	GST_MIXER_TYPE (iface) = GST_MIXER_HARDWARE;

	iface->list_tracks = GST_DEBUG_FUNCPTR (gst_dasf_sink_list_tracks);
	iface->set_volume = GST_DEBUG_FUNCPTR (gst_dasf_sink_set_volume);
	iface->get_volume = GST_DEBUG_FUNCPTR (gst_dasf_sink_get_volume);
	iface->set_mute = GST_DEBUG_FUNCPTR (gst_dasf_sink_set_mute);
	/* iface->get_mute = GST_DEBUG_FUNCPTR (gst_dasf_sink_get_mute); */

	return;
}

static void
factory_get_video_decoder(GooComponent *component, GooTiVideoDecoder **video_dec)
{
	if(*video_dec == NULL)
		if(g_type_is_a(G_OBJECT_TYPE(component), GOO_TYPE_TI_VIDEO_DECODER))
		{
			*video_dec = GOO_TI_VIDEO_DECODER(component);
		}
}


static guint
gst_dasf_clock_required (GstDasfSink* self)
{
	guint retvalue = 0;
	GooComponentFactory *factory = NULL;
	GooTiPostProcessor *peer_component = NULL;

	factory = GOO_COMPONENT_FACTORY( goo_object_get_owner(GOO_OBJECT(self->component)));

	if(factory == NULL)
		goto done;

	GooList *list = factory->components;
	GooTiVideoDecoder *video_decoder = NULL;

	goo_list_foreach(list, (GFunc)factory_get_video_decoder, &video_decoder);

	if(video_decoder == NULL)
		goto done;

	GST_DEBUG_OBJECT (self, "Found a video decoder %s\n", GOO_OBJECT_NAME(GOO_OBJECT(video_decoder)));
	GstGooVideoFilter *gst_video_dec =  GST_GOO_VIDEO_FILTER(g_object_get_data(G_OBJECT(video_decoder), "gst"));

	peer_component = GOO_TI_POST_PROCESSOR (gst_goo_utils_get_peer_component (gst_video_dec->srcpad));
	if (peer_component == NULL)
		goto done;

	self->pp = peer_component;
	retvalue = 1;

	done:
	if (factory != NULL)
		g_object_unref (factory);
	return retvalue;
}


static void
gst_dasf_enable (GstDasfSink* self)
{
	GST_INFO ("");
	GstPad *peer;
	GstElement *prev_element;
	GooComponent *component;
	GstDasfSinkPrivate* priv = GST_DASF_SINK_GET_PRIVATE (self);

	if (self->component != NULL)
	{
		return;
	}

	peer = gst_pad_get_peer (GST_BASE_AUDIO_SINK_PAD (self));

	if (G_UNLIKELY (peer == NULL))
	{
		GST_INFO ("No previous pad");
		return;
	}

	prev_element = GST_ELEMENT (gst_pad_get_parent (peer));

	if (G_UNLIKELY (prev_element == NULL))
	{
		GST_INFO ("Cannot find a previous element");
		goto done;
	}

	/** expecting a capsfilter between dasfsink and goo audio component **/
	while (GST_IS_BASE_TRANSFORM (prev_element))
	{
		GST_DEBUG_OBJECT(self, "previous element name: %s", gst_element_get_name (prev_element));

		gst_object_unref (peer);
		peer = gst_pad_get_peer (GST_BASE_TRANSFORM_SINK_PAD (prev_element));
		gst_object_unref (prev_element);
		prev_element = GST_ELEMENT(gst_pad_get_parent (peer)) ;

		GST_DEBUG_OBJECT (self, "one before element name: %s", gst_element_get_name(prev_element));
	}

	/** capsfilter might be found
	 *  element previous to the caps filter should be goo **/

	component = GOO_COMPONENT (g_object_get_data (G_OBJECT (prev_element), "goo"));

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
	priv->clock_required = gst_dasf_clock_required (self);
	if (priv->clock_source == AUTO_CLOCK)
	{
		priv->clock_source = priv->clock_required;
	}

	/*  Check if OMX will be the clock source and get a new clock instance
		if true */
	if (priv->clock_source == OMX_CLOCK)
	{

		self->clock =
			goo_component_factory_get_component( self->factory,
							    GOO_TI_CLOCK);

		GST_DEBUG ("Clock component clock refcount %d",
				G_OBJECT(self->clock)->ref_count);
	}


	goo_ti_audio_component_set_dasf_mode (self->component, TRUE);
	GST_DEBUG_OBJECT (self, "set data path");
	goo_ti_audio_component_set_data_path (self->component, 0);

	if (priv->clock_source == OMX_CLOCK)
	{
		goo_component_set_clock (component, self->clock);

		GST_DEBUG_OBJECT (self, "Settting clock to idle");
		goo_component_set_state_idle (self->clock);
		GST_DEBUG_OBJECT (self, "Settting clock to executing");
		goo_component_set_state_executing(self->clock);

	}

done:
	gst_object_unref (peer);
	gst_object_unref (prev_element);

	return;
}

/* This group of functions need to be overriden so that the ring
 * buffer becomes useless in our implementation
 */

static gboolean
gst_dasf_sink_open_device (GstAudioSink *sink)
{
	GstDasfSink* self = GST_DASF_SINK (sink);
	gboolean ret = TRUE;

	GST_LOG ("No need to open device");

	return ret;
}

static gboolean
gst_dasf_sink_prepare_device (GstAudioSink *sink, GstRingBufferSpec *spec)
{
	GstDasfSink* self = GST_DASF_SINK (sink);
	gboolean ret = TRUE;

	GST_LOG ("No need to prepare device");

	return ret;
}

static gboolean
gst_dasf_sink_unprepare_device (GstAudioSink *sink )
{
	GstDasfSink* self = GST_DASF_SINK (sink);
	gboolean ret = TRUE;

	GST_LOG ("No need to unprepare device");

	return ret;
}

static gboolean
gst_dasf_sink_close_device (GstAudioSink *sink)
{
	GstDasfSink* self = GST_DASF_SINK (sink);
	gboolean ret = TRUE;

	GST_LOG ("No need to close device");

	return ret;
}

static guint
gst_dasf_sink_write_device (GstAudioSink *sink, gpointer data, guint length)
{
	GstDasfSink* self = GST_DASF_SINK (sink);
	guint ret = 0;

	GST_LOG ("Writting %d bytes", ret);

	return ret;
}

static guint
gst_dasf_sink_delay_device (GstAudioSink *sink)
{
	GstDasfSink* self = GST_DASF_SINK (sink);
	guint ret = 0;

	GST_LOG ("Delaying %d bytes", ret);

	return ret;
}

static void
gst_dasf_sink_reset_device (GstAudioSink *sink)
{
	GstDasfSink* self = GST_DASF_SINK (sink);
	guint ret = 0;

	GST_LOG ("Reset Device");

}


static GstFlowReturn
gst_dasf_sink_render (GstBaseSink *sink, GstBuffer *buffer)
{
	GstDasfSink* self = GST_DASF_SINK (sink);
	GstFlowReturn ret = GST_FLOW_OK;

	GST_LOG ("");

	return ret;
}

static GstFlowReturn
gst_dasf_sink_preroll (GstBaseSink *sink, GstBuffer *buffer)
{
	GstDasfSink* self = GST_DASF_SINK (sink);
	GstFlowReturn ret = GST_FLOW_OK;

	GST_LOG ("");

	return ret;
}

static void
_do_init (GType dasfsink_type)
{
	static const GInterfaceInfo implements_interface_info =
		{
			(GInterfaceInitFunc) gst_dasf_sink_implements_interface_init,
			NULL,
			NULL
		};

	static const GInterfaceInfo mixer_interface_info =
		{
			(GInterfaceInitFunc) gst_dasf_sink_mixer_interface_init,
			NULL,
			NULL
		};

	g_type_add_interface_static (dasfsink_type,
				     GST_TYPE_IMPLEMENTS_INTERFACE,
				     &implements_interface_info);

	g_type_add_interface_static (dasfsink_type,
				     GST_TYPE_MIXER,
				     &mixer_interface_info);

	return;
}

GST_BOILERPLATE_FULL (GstDasfSink, gst_dasf_sink, GstAudioSink,
		      GST_TYPE_AUDIO_SINK, _do_init);

static GstStateChangeReturn
gst_dasf_sink_change_state (GstElement* element, GstStateChange transition)
{
	GST_LOG ("");

	GstDasfSink* self = GST_DASF_SINK (element);
	GstDasfSinkPrivate* priv = GST_DASF_SINK_GET_PRIVATE (self);

	switch (transition)
	{
		case GST_STATE_CHANGE_NULL_TO_READY:
			gst_dasf_enable (self);
			break;
		case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
			if (priv->clock_source == OMX_CLOCK)
			{
				g_object_set (G_OBJECT (self), "halted", TRUE, NULL);
				return GST_STATE_CHANGE_SUCCESS;
			}
			break;
		case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
			if (priv->clock_source == OMX_CLOCK)
			{
				if(priv->first_time_playing)
					priv->first_time_playing = FALSE;
				else
				{
					g_object_set (G_OBJECT (self), "halted", FALSE, NULL);
					return GST_STATE_CHANGE_SUCCESS;
				}
			}
			break;
		case GST_STATE_CHANGE_PAUSED_TO_READY:
			if (priv->clock_source == OMX_CLOCK)
			{
				g_object_set (G_OBJECT (self), "halted", FALSE, NULL);
				GST_INFO ("Setting clock to idle");
				goo_component_set_state_idle (self->clock);
				GST_INFO ("Setting clock to loaded");
				goo_component_set_state_loaded (self->clock);
			}
			break;
		default:
			break;
	}

	return GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
}

void
gst_dasf_sink_set_halted (GstDasfSink* self, gboolean halted)
{
	GstDasfSinkPrivate* priv = GST_DASF_SINK_GET_PRIVATE (self);

	GooComponent *component = NULL;
	GooComponent *pp = NULL;

	if(self->component != NULL)
		component = GOO_COMPONENT (self->component);

	if(self->pp != NULL)
		pp = GOO_COMPONENT (self->pp);


	if (halted)
	{
		if (component != NULL && component->cur_state == OMX_StateExecuting)
			goo_component_set_state_pause (component);
		if (pp != NULL && pp->cur_state == OMX_StateExecuting)
			goo_component_set_state_pause (pp);
	}
	else
	{
		if (pp != NULL && pp->cur_state == OMX_StatePause)
			goo_component_set_state_executing (pp);
		if (component != NULL  && component->cur_state == OMX_StatePause)
			goo_component_set_state_executing (component);
	}
}

static void
gst_dasf_sink_set_property (GObject* object, guint prop_id,
			    const GValue* value, GParamSpec* pspec)
{
	GstDasfSinkPrivate* priv = GST_DASF_SINK_GET_PRIVATE (object);
	GstDasfSink* self = GST_DASF_SINK (object);

	switch (prop_id)
	{
	case PROP_NUM_INPUT_BUFFERS:
		priv->num_input_buffers = g_value_get_uint (value);
		break;
	case PROP_CLOCK_SOURCE:
		priv->clock_source = g_value_get_enum (value);
		break;
	case PROP_HALTED:
		gst_dasf_sink_set_halted (self, g_value_get_boolean (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_dasf_sink_get_property (GObject* object, guint prop_id,
			    GValue* value, GParamSpec* pspec)
{
	GstDasfSinkPrivate* priv = GST_DASF_SINK_GET_PRIVATE (object);

	switch (prop_id)
	{
	case PROP_NUM_INPUT_BUFFERS:
		g_value_set_uint (value, priv->num_input_buffers);
		break;
	case PROP_CLOCK_SOURCE:
		g_value_set_enum (value, priv->clock_source);
		break;
	case PROP_CLOCK_REQUIRED:
		g_value_set_enum (value, priv->clock_required);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	return;
}

static void
gst_dasf_sink_dispose (GObject* object)
{
	GstDasfSink* self = GST_DASF_SINK (object);
	GstDasfSinkPrivate* priv = GST_DASF_SINK_GET_PRIVATE (self);

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

	if (G_LIKELY (self->clock))
	{
		GST_DEBUG ("Unrefing clock component clock refcount %d -> %d",
			G_OBJECT(self->clock)->ref_count,
			(G_OBJECT(self->clock)->ref_count - 1));

		g_object_unref (self->clock);

	}

	if (G_LIKELY (self->factory))
	{
		GST_DEBUG ("unrefing factory");
		g_object_unref (self->factory);
	}

	self->component = NULL;
}

static void
gst_dasf_sink_base_init (gpointer g_klass)
{
	GST_DEBUG_CATEGORY_INIT (gst_dasf_sink_debug, "dasfsink", 0,
				 "TI DASF sink element");

	GstElementClass* e_klass = GST_ELEMENT_CLASS (g_klass);

	gst_element_class_add_pad_template (e_klass,
					    gst_static_pad_template_get
					    (&sink_factory));

	gst_element_class_set_details (e_klass, &details);

	return;
}

static void
gst_dasf_sink_class_init (GstDasfSinkClass* klass)
{
	GObjectClass* g_klass;
	GParamSpec* pspec;
	GstAudioSinkClass* gst_klass;
	GstBaseAudioSinkClass *gst_base_audio_klass;
	GstBaseSinkClass* gst_base_klass;

	/* gobject */
	g_klass = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GstDasfSinkPrivate));

	g_klass->set_property =
		GST_DEBUG_FUNCPTR (gst_dasf_sink_set_property);
	g_klass->get_property =
		GST_DEBUG_FUNCPTR (gst_dasf_sink_get_property);
	g_klass->dispose = GST_DEBUG_FUNCPTR (gst_dasf_sink_dispose);

	pspec = g_param_spec_uint ("input-buffers", "Input buffers",
				   "The number of OMX input buffers",
				   1, 10, NUM_INPUT_BUFFERS_DEFAULT,
				   G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (g_klass, PROP_NUM_INPUT_BUFFERS,
					 pspec);

	pspec = g_param_spec_enum ("clock-source", "Clock Source",
				   "Selects the clock source to synchronize",
				   GST_DASF_SINK_CLOCK_SOURCE,
				   AUTO_CLOCK, G_PARAM_READWRITE);

	g_object_class_install_property (g_klass, PROP_CLOCK_SOURCE,
					 pspec);

	pspec = g_param_spec_enum ("clock-required", "Clock Required",
				   "The clock more suitable for the pipeline",
				   GST_DASF_SINK_CLOCK_SOURCE,
				   GSTREAMER_CLOCK, G_PARAM_READABLE);

	g_object_class_install_property (g_klass, PROP_CLOCK_REQUIRED,
					 pspec);

	pspec = g_param_spec_boolean ("halted", "",
				   "Halt the peer sinks",
				   FALSE, G_PARAM_WRITABLE);

	g_object_class_install_property (g_klass, PROP_HALTED,
					 pspec);

	/* GST */
	gst_klass = GST_AUDIO_SINK_CLASS (klass);
	gst_base_audio_klass = GST_BASE_AUDIO_SINK_CLASS (klass);
	gst_base_klass = GST_BASE_SINK_CLASS (klass);


	/** GST AUDIO SINK **/
	gst_klass->open =
		GST_DEBUG_FUNCPTR (gst_dasf_sink_open_device);
	gst_klass->prepare =
		GST_DEBUG_FUNCPTR (gst_dasf_sink_prepare_device);
	gst_klass->unprepare =
		GST_DEBUG_FUNCPTR (gst_dasf_sink_unprepare_device);
	gst_klass->close =
		GST_DEBUG_FUNCPTR (gst_dasf_sink_close_device);
	gst_klass->write =
		GST_DEBUG_FUNCPTR (gst_dasf_sink_write_device);
	gst_klass->delay =
		GST_DEBUG_FUNCPTR (gst_dasf_sink_delay_device);
	gst_klass->reset =
		GST_DEBUG_FUNCPTR (gst_dasf_sink_reset_device);

	/* GST BASE SINK */
	gst_base_klass->preroll =
		GST_DEBUG_FUNCPTR (gst_dasf_sink_preroll);
	gst_base_klass->render =
		GST_DEBUG_FUNCPTR (gst_dasf_sink_render);

	GST_ELEMENT_CLASS (klass)->change_state =
		GST_DEBUG_FUNCPTR (gst_dasf_sink_change_state);

	return;
}

static void
gst_dasf_sink_init (GstDasfSink* self, GstDasfSinkClass* klass)
{
	GST_DEBUG ("");

	GstDasfSinkPrivate* priv = GST_DASF_SINK_GET_PRIVATE (self);
	priv->num_input_buffers = NUM_INPUT_BUFFERS_DEFAULT;
	priv->incount = 0;
	priv->volume = 100;
	priv->mute = FALSE;
	priv->clock_source = AUTO_CLOCK;
	priv->clock_required = GSTREAMER_CLOCK;
	priv->first_time_playing = TRUE;
	self->component = NULL;
	self->pp = NULL;
	self->tracks = NULL;

	GstMixerTrack* track = g_object_new (GST_TYPE_MIXER_TRACK, NULL);
	track->label = g_strdup ("DASF Volume");
	track->num_channels = 1;
	track->min_volume = 0;
	track->max_volume = 100;
	track->flags = GST_MIXER_TRACK_OUTPUT | GST_MIXER_TRACK_MASTER;
	self->tracks = g_slist_append (self->tracks, track);

	self->factory = goo_ti_component_factory_get_instance ();

	return;
}

