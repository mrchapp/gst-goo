#!/bin/sh

mkdir -p /tmp/gst

echo "Starting with H263 to display and file sink"

# H263 CIF - UYVY - DISPLAY
echo "* H263 decoder cif - uyvy - display "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder cif - uyvy - display filename=foreman_cif_bp45_30fps_2mbps.263" > /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/omx/patterns/foreman_cif_bp45_30fps_2mbps.263" ! \
video/x-h263, width=352, height=288, framerate=15/1 ! \
goodec_h263 process-mode=1 ! \
video/x-raw-yuv, format=\(fourcc\)UYVY, width=352, height=288, framerate=15/1 ! \
goosink_pp >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 CIF - I420 - FILE
echo "* H263 decoder cif - I420 - filesink "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder cif - I420 - filesink filename=foreman_cif_bp45_30fps_2mbps.263" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/omx/patterns/foreman_cif_bp45_30fps_2mbps.263" ! \
video/x-h263, width=352, height=288, framerate=15/1 ! \
goodec_h263 process-mode=1 ! \
video/x-raw-yuv, format=\(fourcc\)I420, width=352, height=288, framerate=15/1 ! \
filesink location=/tmp/gst/goodec_h263_cif_i420.yuv >> /tmp/gst/gst_goo_h263_decoder.log 2>&1


# H263 QCIF - UYVY - DISPLAY
echo "* H263 decoder qcif - UYVY - display "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder qcif - UYVY - display filename=carphone_qcif_bp45_15fps_128kbps.263" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/omx/patterns/carphone_qcif_bp45_15fps_128kbps.263" ! \
video/x-h263, width=176, height=144, framerate=15/1 ! \
goodec_h263 process-mode=1 ! \
video/x-raw-yuv, format=\(fourcc\)UYVY, width=176, height=144, framerate=15/1 ! \
goosink_pp >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 QCIF - I420 - FILE
echo "* H263 decoder qcif - I420 - filesink "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder qcif - I420 - filesink filename=carphone_qcif_bp45_15fps_128kbps.263" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/omx/patterns/carphone_qcif_bp45_15fps_128kbps.263" ! \
video/x-h263, width=176, height=144, framerate=15/1 ! \
goodec_h263 process-mode=1 ! \
video/x-raw-yuv, format=\(fourcc\)I420, width=176, height=144, framerate=15/1 ! \
filesink location=/tmp/gst/goodec_h263_qcif_i420.yuv >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

echo "Starting with H263 combo tests GSTREAMER clock tunnel"

# H263 QCIF 15FPS AMR NB
echo "* H263 decoder qcif - uyvy - display - amrnb "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder qcif - uyvy - display - amrnb filename=hellobaby_h263_qcif_15fps_64kbps_nbamr_8khz_4.75kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_amr_nb/hellobaby_h263_qcif_15fps_64kbps_nbamr_8khz_4.75kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h263 ! goosink_pp \
demux.audio_00 ! queue ! goodec_nbamr ! dasfsink clock-source=0 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 CIF 15FPS AMR NB
echo "* H263 decoder cif - uyvy - display - amrnb "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder cif - uyvy - display - amrnb filename=/multimedia-repository/Video_Files/containers/h263/h263_with_amr_nb/hellobaby_h263_cif_30fps_384kbps_nbamr_8khz_7.40kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_amr_nb/hellobaby_h263_cif_30fps_384kbps_nbamr_8khz_7.40kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h263 ! goosink_pp \
demux.audio_00 ! queue ! goodec_nbamr ! dasfsink clock-source=0 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 QCIF 15FPS AMR WB
echo "* H263 decoder qcif - uyvy - display - amrwb "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder qcif - uyvy - display - amrwb filename=hellobaby_h263_qcif_15fps_64kbps_wbamr_16khz_6.60kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia/repository/hellobaby_h263_qcif_15fps_64kbps_wbamr_16khz_6.60kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h263 ! goosink_pp \
demux.audio_00 ! queue ! goodec_wbamr ! dasfsink clock-source=0 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 CIF 15FPS AMR WB
echo "* H263 decoder cif - uyvy - display - amrwb "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder cif - uyvy - display - amrwb filename=hellobaby_h263_cif_30fps_384kbps_wbamr_16khz_15.85kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_amr_wb/hellobaby_h263_cif_30fps_384kbps_wbamr_16khz_15.85kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h263 ! goosink_pp \
demux.audio_00 ! queue ! goodec_wbamr ! dasfsink clock-source=0 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 QCIF 15FPS MP3
echo "* H263 decoder qcif - uyvy - display - mp3 "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder qcif - uyvy - display - mp3 filename=hellobaby_h263_qcif_15fps_64kbps_mp3_44khz_32kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_mp3/hellobaby_h263_qcif_15fps_64kbps_mp3_44khz_32kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h263 ! goosink_pp \
demux.audio_00 ! queue ! goodec_mp3 ! dasfsink clock-source=0 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 CIF 15FPS AMR WB
echo "* H263 decoder cif - uyvy - display - mp3 "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder cif - uyvy - display - mp3 filename=hellobaby_h263_cif_30fps_384kbps_mp3_44khz_128kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_mp3/hellobaby_h263_cif_30fps_384kbps_mp3_44khz_128kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h263 ! goosink_pp \
demux.audio_00 ! queue ! goodec_mp3 ! dasfsink clock-source=0 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 QCIF 15FPS AAC
echo "* H263 decoder qcif - uyvy - display - aac "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder qcif - uyvy - display - aac filename=H263_QCIF_15fps_64kbps_Aac_44.1khz_128kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_aac/H263_QCIF_15fps_64kbps_Aac_44.1khz_128kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h263 ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=176, height=144, framerate=15/1 ! goosink_pp \
demux.audio_00 ! queue ! goodec_aac ! dasfsink clock-source=0 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

#H263 CIF 15FPS AAC
echo "* H263 decoder cif - uyvy - display - aac "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder cif - uyvy - display - aac filename=H263_CIF_30fps_2Mbps_AAC_96KHz,320kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_aac/H263_CIF_30fps_2Mbps_AAC_96KHz,320kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h263 ! goosink_pp \
demux.audio_00 ! queue ! goodec_aac ! dasfsink clock-source=0 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

echo "Starting with H263 combo tests GSTREAMER clock decodebin"

# H263 QCIF 15FPS AMR NB
echo "* H263 decoder qcif - uyvy - display - amrnb "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder qcif - uyvy - display - amrnb filename=hellobaby_h263_qcif_15fps_64kbps_nbamr_8khz_4.75kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_amr_nb/hellobaby_h263_qcif_15fps_64kbps_nbamr_8khz_4.75kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! decodebin ! goosink_pp \
demux.audio_00 ! queue ! goodec_nbamr ! dasfsink clock-source=0 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 CIF 15FPS AMR NB
echo "* H263 decoder cif - uyvy - display - amrnb "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder cif - uyvy - display - amrnb filename=/multimedia-repository/Video_Files/containers/h263/h263_with_amr_nb/hellobaby_h263_cif_30fps_384kbps_nbamr_8khz_7.40kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_amr_nb/hellobaby_h263_cif_30fps_384kbps_nbamr_8khz_7.40kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! decodebin ! goosink_pp \
demux.audio_00 ! queue ! goodec_nbamr ! dasfsink clock-source=0 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 QCIF 15FPS AMR WB
echo "* H263 decoder qcif - uyvy - display - amrwb "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder qcif - uyvy - display - amrwb filename=hellobaby_h263_qcif_15fps_64kbps_wbamr_16khz_6.60kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia/repository/hellobaby_h263_qcif_15fps_64kbps_wbamr_16khz_6.60kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! decodebin ! goosink_pp \
demux.audio_00 ! queue ! goodec_wbamr ! dasfsink clock-source=0 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 CIF 15FPS AMR WB
echo "* H263 decoder cif - uyvy - display - amrwb "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder cif - uyvy - display - amrwb filename=hellobaby_h263_cif_30fps_384kbps_wbamr_16khz_15.85kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_amr_wb/hellobaby_h263_cif_30fps_384kbps_wbamr_16khz_15.85kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! decodebin ! goosink_pp \
demux.audio_00 ! queue ! goodec_wbamr ! dasfsink clock-source=0 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 QCIF 15FPS MP3
echo "* H263 decoder qcif - uyvy - display - mp3 "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder qcif - uyvy - display - mp3 filename=hellobaby_h263_qcif_15fps_64kbps_mp3_44khz_32kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_mp3/hellobaby_h263_qcif_15fps_64kbps_mp3_44khz_32kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! decodebin ! goosink_pp \
demux.audio_00 ! queue ! goodec_mp3 ! dasfsink clock-source=0 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 CIF 15FPS AMR WB
echo "* H263 decoder cif - uyvy - display - mp3 "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder cif - uyvy - display - mp3 filename=hellobaby_h263_cif_30fps_384kbps_mp3_44khz_128kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_mp3/hellobaby_h263_cif_30fps_384kbps_mp3_44khz_128kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! decodebin ! goosink_pp \
demux.audio_00 ! queue ! goodec_mp3 ! dasfsink clock-source=0 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 QCIF 15FPS AAC
echo "* H263 decoder qcif - uyvy - display - aac "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder qcif - uyvy - display - aac filename=H263_QCIF_15fps_64kbps_Aac_44.1khz_128kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_aac/H263_QCIF_15fps_64kbps_Aac_44.1khz_128kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h263 ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=176, height=144, framerate=15/1 ! goosink_pp \
demux.audio_00 ! queue ! goodec_aac ! dasfsink clock-source=0 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

#H263 CIF 15FPS AAC
echo "* H263 decoder cif - uyvy - display - aac "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder cif - uyvy - display - aac filename=H263_CIF_30fps_2Mbps_AAC_96KHz,320kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_aac/H263_CIF_30fps_2Mbps_AAC_96KHz,320kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h263 ! goosink_pp \
demux.audio_00 ! queue ! goodec_aac ! dasfsink clock-source=0 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

echo "Starting with H263 combo tests OMX clock"

# H263 QCIF 15FPS AMR NB
echo "* H263 decoder qcif - uyvy - display - amrnb "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder qcif - uyvy - display - amrnb filename=hellobaby_h263_qcif_15fps_64kbps_nbamr_8khz_4.75kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_amr_nb/hellobaby_h263_qcif_15fps_64kbps_nbamr_8khz_4.75kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h263 ! goosink_pp \
demux.audio_00 ! queue ! goodec_nbamr ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 CIF 15FPS AMR NB
echo "* H263 decoder cif - uyvy - display - amrnb "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder cif - uyvy - display - amrnb filename=/multimedia-repository/Video_Files/containers/h263/h263_with_amr_nb/hellobaby_h263_cif_30fps_384kbps_nbamr_8khz_7.40kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_amr_nb/hellobaby_h263_cif_30fps_384kbps_nbamr_8khz_7.40kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h263 ! goosink_pp \
demux.audio_00 ! queue ! goodec_nbamr ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 QCIF 15FPS AMR WB
echo "* H263 decoder qcif - uyvy - display - amrwb "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder qcif - uyvy - display - amrwb filename=hellobaby_h263_qcif_15fps_64kbps_wbamr_16khz_6.60kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia/repository/hellobaby_h263_qcif_15fps_64kbps_wbamr_16khz_6.60kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h263 ! goosink_pp \
demux.audio_00 ! queue ! goodec_wbamr ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 CIF 15FPS AMR WB
echo "* H263 decoder cif - uyvy - display - amrwb "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder cif - uyvy - display - amrwb filename=hellobaby_h263_cif_30fps_384kbps_wbamr_16khz_15.85kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_amr_wb/hellobaby_h263_cif_30fps_384kbps_wbamr_16khz_15.85kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h263 ! goosink_pp \
demux.audio_00 ! queue ! goodec_wbamr ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 QCIF 15FPS MP3
echo "* H263 decoder qcif - uyvy - display - mp3 "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder qcif - uyvy - display - mp3 filename=hellobaby_h263_qcif_15fps_64kbps_mp3_44khz_32kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_mp3/hellobaby_h263_qcif_15fps_64kbps_mp3_44khz_32kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h263 ! goosink_pp \
demux.audio_00 ! queue ! goodec_mp3 ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 CIF 15FPS AMR WB
echo "* H263 decoder cif - uyvy - display - mp3 "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder cif - uyvy - display - mp3 filename=hellobaby_h263_cif_30fps_384kbps_mp3_44khz_128kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_mp3/hellobaby_h263_cif_30fps_384kbps_mp3_44khz_128kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h263 ! goosink_pp \
demux.audio_00 ! queue ! goodec_mp3 ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

# H263 QCIF 15FPS AAC
echo "* H263 decoder qcif - uyvy - display - aac "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder qcif - uyvy - display - aac filename=H263_QCIF_15fps_64kbps_Aac_44.1khz_128kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_aac/H263_QCIF_15fps_64kbps_Aac_44.1khz_128kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h263 ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=176, height=144, framerate=15/1 ! goosink_pp \
demux.audio_00 ! queue ! goodec_aac ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

#H263 CIF 15FPS AAC
echo "* H263 decoder cif - uyvy - display - aac "
echo "" >> /tmp/gst/gst_goo_h263_decoder.log
echo "* H263 decoder cif - uyvy - display - aac filename=H263_CIF_30fps_2Mbps_AAC_96KHz,320kbps.3gp" >> /tmp/gst/gst_goo_h263_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h263/h263_with_aac/H263_CIF_30fps_2Mbps_AAC_96KHz,320kbps.3gp" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h263 ! goosink_pp \
demux.audio_00 ! queue ! goodec_aac ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_h263_decoder.log 2>&1

