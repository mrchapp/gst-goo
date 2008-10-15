#!/bin/sh

mkdir -p /tmp/gst

echo "Starting with Video decoders to display and file sink"

# MPEG4 QCIF - UYVY - DISPLAY
echo "* MPEG4 decoder qcif - uyvy - display "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder qcif - uyvy - display filename=carphone_qcif_vsp1_15fps_64kbps.m4v" > /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia/patterns/carphone_qcif_vsp1_15fps_64kbps.m4v" ! \
video/mpeg, width=176, height=144, framerate=15/1, mpegversion=4, systemstream=\(boolean\)false ! \
goodec_mpeg4 process-mode=1 ! \
video/x-raw-yuv, format=\(fourcc\)UYVY, width=176, height=144, framerate=15/1 ! \
goosink_pp >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

# MPEG4 QCIF - I420 - FILESINK
echo "* MPEG4 decoder qcif - i420 - filesink "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder qcif - i420 - filesink filename=carphone_qcif_vsp1_15fps_64kbps.m4v" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia/patterns/carphone_qcif_vsp1_15fps_64kbps.m4v" ! \
video/mpeg, width=176, height=144, framerate=15/1, mpegversion=4, systemstream=\(boolean\)false ! \
goodec_mpeg4 process-mode=1 ! \
video/x-raw-yuv, format=\(fourcc\)I420, width=176, height=144, framerate=15/1 ! \
filesink location=/tmp/gst/goodec_mpeg4_qcif_i420.yuv >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

# MPEG4 VGA - UYVY - DISPLAY
echo "* MPEG4 decoder vga - uyvy - display "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder vga - uyvy - display filename=foreman_vga_vsp4a_30fps_4mbps.m4v" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia/patterns/foreman_vga_vsp4a_30fps_4mbps.m4v" ! \
video/mpeg, width=640, height=480, framerate=15/1, mpegversion=4, systemstream=\(boolean\)false ! \
goodec_mpeg4 process-mode=1 ! \
video/x-raw-yuv, format=\(fourcc\)UYVY, width=640, height=480, framerate=15/1 ! \
goosink_pp >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

echo "Starting with MPEG4 combo tests with GStreamer Clock"

#MPEG4 QCIF 15FPS AMR NB
echo "* MPEG4 decoder qcif - uyvy - display - amrnb "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder qcif - uyvy - display - amrnb filename=MPEG4_QCIF15fps_64kbps_Narrow-Band_AMR_8KHz,4.75kbps.MP4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/MPEG4/MPEG4_with_amr_nb/MPEG4_QCIF15fps_64kbps_Narrow-Band_AMR_8KHz,4.75kbps.MP4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp \
demux.audio_00 ! queue ! goodec_nbamr ! dasfsink >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

#MPEG4 CIF 15FPS AMR NB
echo "* MPEG4 decoder cif - uyvy - display - amrnb "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder cif - uyvy - display - amrnb filename=MPEG4_CIF_30fps_384kbpsNarrow-Band_AMR_8KHz,7.40kbps.mp4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/MPEG4/MPEG4_with_amr_nb/MPEG4_CIF_30fps_384kbpsNarrow-Band_AMR_8KHz,7.40kbps.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp \
demux.audio_00 ! queue ! goodec_nbamr ! dasfsink >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

#MPEG4 VGA 15FPS AMR NB
echo "* MPEG4 decoder vga - uyvy - display - amrnb "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder vga - uyvy - display - amrnb filename=MPEG4_VGA30fps_3-4Mbps_Narrow-BandAMR_8KHz,12.2kbps.mp4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/MPEG4/MPEG4_with_amr_nb/MPEG4_VGA30fps_3-4Mbps_Narrow-BandAMR_8KHz,12.2kbps.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp x-scale=50 y-scale=50 rotation=90 \
demux.audio_00 ! queue ! goodec_nbamr ! dasfsink >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

#MPEG4 QCIF 15FPS AMR wB
echo "* MPEG4 decoder qcif - uyvy - display - amrwb "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder qcif - uyvy - display - amrwb filename=MPEG4QCIF15fps_64kbps_Wide-Band_AMR_16KHz,6.60kbps.MP4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/MPEG4/MPEG4_with_amr_wb/MPEG4QCIF15fps_64kbps_Wide-Band_AMR_16KHz,6.60kbps.MP4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp \
demux.audio_00 ! queue ! goodec_wbamr ! dasfsink >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

#MPEG4 CIF 15FPS AMR WB
echo "* MPEG4 decoder cif - uyvy - display - amrwb "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder cif - uyvy - display - amrwb filename=MPEG4CIF30fps_384kbps_Wide-Band_AMR_16KHz_15.85kbps.MP4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia/repository/MPEG4CIF30fps_384kbps_Wide-Band_AMR_16KHz_15.85kbps.MP4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp \
demux.audio_00 ! queue ! goodec_wbamr ! dasfsink >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

#MPEG4 VGA 15FPS AMR WB
echo "* MPEG4 decoder vga - uyvy - display - amrwb "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder vga - uyvy - display - amrwb filename=MPEG4VGA30fps_3-5Mbps_Wide-Band_AMR_16KHz,24kbps.MP4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/MPEG4/MPEG4_with_amr_wb/MPEG4VGA30fps_3-5Mbps_Wide-Band_AMR_16KHz,24kbps.MP4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp \
demux.audio_00 ! queue ! goodec_wbamr ! dasfsink >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

#MPEG4 QCIF 15FPS MP3
echo "* MPEG4 decoder qcif - uyvy - display - mp3 "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder qcif - uyvy - display - mp3 filename=MPEG4QCIF_15fps_64kbps_MP344KHz,32kbps.mp4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/MPEG4/MPEG4_with_mp3/MPEG4QCIF_15fps_64kbps_MP344KHz,32kbps.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp \
demux.audio_00 ! queue ! goodec_mp3 ! dasfsink >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

#MPEG4 CIF 30FPS MP3
echo "* MPEG4 decoder cif - uyvy - display - mp3 "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder cif - uyvy - display - mp3 filename=MPEG4CIF30fps_384kbps_MP3_44KHz,128kbps.mp4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia/repository/MPEG4CIF30fps_384kbps_MP3_44KHz,128kbps.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp \
demux.audio_00 ! queue ! goodec_mp3 ! dasfsink >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

#MPEG4 VGA 30FPS MP3
echo "* MPEG4 decoder vga - uyvy - display - mp3 "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder vga - uyvy - display - mp3 filename=MPEG4_VGA30fps_3-4Mbps_MP3_48KHz,320kbps.mp4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia/repository/MPEG4_VGA30fps_3-4Mbps_MP3_48KHz,320kbps.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp \
demux.audio_00 ! queue ! goodec_mp3 ! dasfsink >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

#MPEG4 VGA 30FPS AAC
echo "* MPEG4 decoder vga - uyvy - display - aac "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder vga - uyvy - display - aac filename=MPEG4VGA30fps_3-4_Mbps_AAC48KHz128kbps.MP4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/MPEG4/MPEG4_with_AAC/MPEG4VGA30fps_3-4_Mbps_AAC48KHz128kbps.MP4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp \
demux.audio_00 ! queue ! goodec_aac ! dasfsink >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

echo "Starting with MPEG4 combo tests with OMX Clock"

#MPEG4 QCIF 15FPS AMR NB
echo "* MPEG4 decoder qcif - uyvy - display - amrnb "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder qcif - uyvy - display - amrnb filename=MPEG4_QCIF15fps_64kbps_Narrow-Band_AMR_8KHz,4.75kbps.MP4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/MPEG4/MPEG4_with_amr_nb/MPEG4_QCIF15fps_64kbps_Narrow-Band_AMR_8KHz,4.75kbps.MP4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp \
demux.audio_00 ! queue ! goodec_nbamr ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

#MPEG4 CIF 15FPS AMR NB
echo "* MPEG4 decoder cif - uyvy - display - amrnb "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder cif - uyvy - display - amrnb filename=MPEG4_CIF_30fps_384kbpsNarrow-Band_AMR_8KHz,7.40kbps.mp4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/MPEG4/MPEG4_with_amr_nb/MPEG4_CIF_30fps_384kbpsNarrow-Band_AMR_8KHz,7.40kbps.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp \
demux.audio_00 ! queue ! goodec_nbamr ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

#MPEG4 VGA 15FPS AMR NB
echo "* MPEG4 decoder vga - uyvy - display - amrnb "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder vga - uyvy - display - amrnb filename=MPEG4_VGA30fps_3-4Mbps_Narrow-BandAMR_8KHz,12.2kbps.mp4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/MPEG4/MPEG4_with_amr_nb/MPEG4_VGA30fps_3-4Mbps_Narrow-BandAMR_8KHz,12.2kbps.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp x-scale=50 y-scale=50 rotation=90 \
demux.audio_00 ! queue ! goodec_nbamr ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

#MPEG4 QCIF 15FPS AMR wB
echo "* MPEG4 decoder qcif - uyvy - display - amrwb "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder qcif - uyvy - display - amrwb filename=MPEG4QCIF15fps_64kbps_Wide-Band_AMR_16KHz,6.60kbps.MP4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/MPEG4/MPEG4_with_amr_wb/MPEG4QCIF15fps_64kbps_Wide-Band_AMR_16KHz,6.60kbps.MP4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp \
demux.audio_00 ! queue ! goodec_wbamr ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

#MPEG4 CIF 15FPS AMR WB
echo "* MPEG4 decoder cif - uyvy - display - amrwb "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder cif - uyvy - display - amrwb filename=MPEG4CIF30fps_384kbps_Wide-Band_AMR_16KHz_15.85kbps.MP4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia/repository/MPEG4CIF30fps_384kbps_Wide-Band_AMR_16KHz_15.85kbps.MP4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp \
demux.audio_00 ! queue ! goodec_wbamr ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

#MPEG4 VGA 15FPS AMR WB
echo "* MPEG4 decoder vga - uyvy - display - amrwb "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder vga - uyvy - display - amrwb filename=MPEG4VGA30fps_3-5Mbps_Wide-Band_AMR_16KHz,24kbps.MP4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/MPEG4/MPEG4_with_amr_wb/MPEG4VGA30fps_3-5Mbps_Wide-Band_AMR_16KHz,24kbps.MP4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp \
demux.audio_00 ! queue ! goodec_wbamr ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

#MPEG4 QCIF 15FPS MP3
echo "* MPEG4 decoder qcif - uyvy - display - mp3 "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder qcif - uyvy - display - mp3 filename=MPEG4QCIF_15fps_64kbps_MP344KHz,32kbps.mp4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/MPEG4/MPEG4_with_mp3/MPEG4QCIF_15fps_64kbps_MP344KHz,32kbps.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp \
demux.audio_00 ! queue ! goodec_mp3 ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

#MPEG4 CIF 30FPS MP3
echo "* MPEG4 decoder cif - uyvy - display - mp3 "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder cif - uyvy - display - mp3 filename=MPEG4CIF30fps_384kbps_MP3_44KHz,128kbps.mp4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia/repository/MPEG4CIF30fps_384kbps_MP3_44KHz,128kbps.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp \
demux.audio_00 ! queue ! goodec_mp3 ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

#MPEG4 VGA 30FPS MP3
echo "* MPEG4 decoder vga - uyvy - display - mp3 "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder vga - uyvy - display - mp3 filename=MPEG4_VGA30fps_3-4Mbps_MP3_48KHz,320kbps.mp4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia/repository/MPEG4_VGA30fps_3-4Mbps_MP3_48KHz,320kbps.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp \
demux.audio_00 ! queue ! goodec_mp3 ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1

#MPEG4 VGA 30FPS AAC
echo "* MPEG4 decoder vga - uyvy - display - aac "
echo "" >> /tmp/gst/gst_goo_mpeg4_decoder.log
echo "* MPEG4 decoder vga - uyvy - display - aac filename=MPEG4VGA30fps_3-4_Mbps_AAC48KHz128kbps.MP4" >> /tmp/gst/gst_goo_mpeg4_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/MPEG4/MPEG4_with_AAC/MPEG4VGA30fps_3-4_Mbps_AAC48KHz128kbps.MP4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_mpeg4 ! goosink_pp \
demux.audio_00 ! queue ! goodec_aac ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_mpeg4_decoder.log 2>&1





