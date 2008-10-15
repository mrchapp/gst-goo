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

#include <config.h>

#include <gst/gst.h>

#include "gstgoodecwbamr.h"
#include "gstgoodecnbamr.h"
#include "gstgooencpcm.h"
#include "gstgoodecmp3.h"
#include "gstgoodecwma.h"
#include "gstgoodecpcm.h"
#include "gstgoodecgsmhr.h"
#include "gstgoodecg722.h"
#include "gstgoodecg711.h"
#include "gstgoodecaac.h"
#include "gstgooencnbamr.h"
#include "gstgooencgsmhr.h"
#include "gstgooencwbamr.h"
#include "gstgooencgsmfr.h"
#include "gstgooencaac.h"
#include "gstgoodecmpeg4.h"
#include "gstdasfsrc.h"
#include "gstdasfsink.h"
#ifdef TI_CAMERA
#include "gstgoocamera.h"
#endif
#include "gstgooencjpeg.h"
#include "gstgoodecjpeg.h"
#include "gstgoodech264.h"
#include "gstgoodech263.h"
#include "gstgoodecwmv.h"
#include "gstgooencmpeg4.h"
#include "gstgooench264.h"
#include "gstgooench263.h"
#include "gstgoosinkpp.h"
#include "gstgoofiltervpp.h"
#include "gstgoodecmpeg2.h"
#include "gstgoodecgsmfr.h"

struct _element_entry
{
        gchar* name;
        guint rank;
        GType (*type) (void);
};

static struct _element_entry _elements[] = {
	{ "gooenc_pcm", GST_RANK_PRIMARY, gst_goo_encpcm_get_type },
	{ "goodec_mp3", GST_RANK_PRIMARY, gst_goo_decmp3_get_type },
	{ "goodec_wma", GST_RANK_PRIMARY, gst_goo_decwma_get_type },
	{ "goodec_pcm", GST_RANK_PRIMARY, gst_goo_decpcm_get_type },
	{ "goodec_gsmhr", GST_RANK_PRIMARY, gst_goo_decgsmhr_get_type },
	{ "goodec_g722", GST_RANK_PRIMARY, gst_goo_decg722_get_type },
	{ "goodec_gsmfr", GST_RANK_PRIMARY, gst_goo_decgsmfr_get_type },
	{ "goodec_g711", GST_RANK_PRIMARY, gst_goo_decg711_get_type },
	{ "goodec_aac", GST_RANK_PRIMARY, gst_goo_decaac_get_type },
	{ "gooenc_nbamr", GST_RANK_PRIMARY, gst_goo_encnbamr_get_type },
	{ "gooenc_gsmhr", GST_RANK_PRIMARY, gst_goo_encgsmhr_get_type },
	{ "gooenc_wbamr", GST_RANK_PRIMARY, gst_goo_encwbamr_get_type },
	{ "gooenc_gsmfr", GST_RANK_PRIMARY, gst_goo_encgsmfr_get_type },
	{ "gooenc_aac", GST_RANK_PRIMARY, gst_goo_encaac_get_type },
	{ "goodec_mpeg4", GST_RANK_PRIMARY, gst_goo_decmpeg4_get_type },
	{ "dasfsrc", GST_RANK_PRIMARY, gst_dasf_src_get_type },
	{ "dasfsink", GST_RANK_PRIMARY, gst_dasf_sink_get_type },
#ifdef TI_CAMERA
	{ "goocamera", GST_RANK_PRIMARY, gst_goo_camera_get_type },
#endif
	{ "gooenc_jpeg", GST_RANK_PRIMARY, gst_goo_encjpeg_get_type },
	{ "goodec_jpeg", GST_RANK_PRIMARY, gst_goo_decjpeg_get_type },
	{ "goodec_h264", GST_RANK_PRIMARY, gst_goo_dech264_get_type },
	{ "goodec_h263", GST_RANK_PRIMARY, gst_goo_dech263_get_type },
	{ "goodec_wmv", GST_RANK_PRIMARY, gst_goo_decwmv_get_type },
	{ "gooenc_mpeg4", GST_RANK_PRIMARY, gst_goo_encmpeg4_get_type },
	{ "gooenc_h264", GST_RANK_PRIMARY, gst_goo_ench264_get_type },
	{ "gooenc_h263", GST_RANK_PRIMARY, gst_goo_ench263_get_type },
	{ "goosink_pp", GST_RANK_PRIMARY, gst_goo_sinkpp_get_type },
	{ "goofilter_vpp", GST_RANK_PRIMARY, gst_goo_filtervpp_get_type },
	{ "goodec_mpeg2", GST_RANK_PRIMARY, gst_goo_decmpeg2_get_type },
	{ "goodec_nbamr", GST_RANK_PRIMARY, gst_goo_decnbamr_get_type },
	{ "goodec_wbamr", GST_RANK_PRIMARY, gst_goo_decwbamr_get_type },
	{ NULL, 0 },
};

static gboolean
plugin_init (GstPlugin* plugin)
{
        struct _element_entry *my_elements = _elements;

        while ((*my_elements).name)
        {
                if (!gst_element_register (plugin,
                                           (*my_elements).name,
                                           (*my_elements).rank,
                                           ((*my_elements).type) ()))
                {
                        return FALSE;
                }
                my_elements++;
        }
        return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
				   GST_VERSION_MINOR,
				   "gstgoo",
				   "GObject OpenMAX TI elements",
				   plugin_init,
				   VERSION,
				   "LGPL",
				   GST_PACKAGE_NAME,
				   GST_PACKAGE_ORIGIN);
