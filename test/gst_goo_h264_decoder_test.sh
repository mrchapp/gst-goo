#!/bin/sh

mkdir -p /tmp/gst

echo "Starting with H264 to display and file sink"

# H264 QCIF - UYVY - DISPLAY
echo "* H264 decoder qcif - uyvy - display "
echo "" >> /tmp/gst/gst_goo_h264_decoder.log
echo "* H264 decoder qcif - uyvy - display filename=H264_QCIF_100f_15fps_128kbits_L1_b.264" > /tmp/gst/gst_goo_h264_decoder.log
gst-launch filesrc location="/omx/patterns/H264_QCIF_100f_15fps_128kbits_L1_b.264" ! \
video/x-h264, width=176, height=144, framerate=15/1 ! \
goodec_h264 process-mode=1 ! \
video/x-raw-yuv, format=\(fourcc\)UYVY, width=176, height=144, framerate=15/1 ! \
goosink_pp >> /tmp/gst/gst_goo_h264_decoder.log 2>&1

# H264 QCIF - I420 - FILE
echo "* H264 decoder qcif - i420 - filesink "
echo "" >> /tmp/gst/gst_goo_h264_decoder.log
echo "* H264 decoder qcif - i420 - filesink filename=H264_QCIF_100f_15fps_128kbits_L1_b.264" >> /tmp/gst/gst_goo_h264_decoder.log
gst-launch filesrc location="/omx/patterns/H264_QCIF_100f_15fps_128kbits_L1_b.264" ! \
video/x-h264, width=176, height=144, framerate=15/1 ! \
goodec_h264 process-mode=1 ! \
video/x-raw-yuv, format=\(fourcc\)I420, width=176, height=144, framerate=15/1 ! \
filesink location=/tmp/gst/goodec_h264_qcif_i420.yuv >> /tmp/gst/gst_goo_h264_decoder.log 2>&1

# H264 CIF - UYVY - DISPLAY
echo "* H264 decoder cif - uyvy - display "
echo "" >> /tmp/gst/gst_goo_h264_decoder.log
echo "* H264 decoder cif - uyvy - display filename=H264_CIF_150f_7_5fps_192kbits_L1_1.264" >> /tmp/gst/gst_goo_h264_decoder.log
gst-launch filesrc location="/omx/patterns/H264_CIF_150f_7_5fps_192kbits_L1_1.264" ! \
video/x-h264, width=352, height=288, framerate=15/1 ! \
goodec_h264 process-mode=1 ! \
video/x-raw-yuv, format=\(fourcc\)UYVY, width=352, height=288, framerate=15/1 ! \
goosink_pp >> /tmp/gst/gst_goo_h264_decoder.log 2>&1

echo "Starting with H264 combo tests with GStreamer Clock"

#H264 CIF 15FPS AMR NB
echo "* H264 decoder cif - uyvy - display - amrnb "
echo "" >> /tmp/gst/gst_goo_h264_decoder.log
echo "* H264 decoder cif - uyvy - display - amrnb filename=CIF_15fps_128kbps_8khz_7.40_Amr_NB.mp4" >> /tmp/gst/gst_goo_h264_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h264/h264_with_ffmpeg_PROFILE_2.2/h264_amr_nb/CIF_15fps_128kbps_8khz_7.40_Amr_NB.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h264 ! goosink_pp \
demux.audio_00 ! queue ! goodec_nbamr ! dasfsink >> /tmp/gst/gst_goo_h264_decoder.log 2>&1

#H264 QCIF 15FPS AMR NB
echo "* H264 decoder qcif - uyvy - display - amrnb "
echo "" >> /tmp/gst/gst_goo_h264_decoder.log
echo "* H264 decoder qcif - uyvy - display - amrnb filename=QCIF_15fps_128kbps_8khz_4.5kbps_AMR_NB.mp4" >> /tmp/gst/gst_goo_h264_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h264/h264_with_ffmpeg_PROFILE_2.2/h264_amr_nb/QCIF_15fps_128kbps_8khz_4.5kbps_AMR_NB.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h264 ! goosink_pp \
demux.audio_00 ! queue ! goodec_nbamr ! dasfsink >> /tmp/gst/gst_goo_h264_decoder.log 2>&1

#H264 CIF 15FPS AMR WB
echo "* H264 decoder cif - uyvy - display - amrwb "
echo "" >> /tmp/gst/gst_goo_h264_decoder.log
echo "* H264 decoder cif - uyvy - display - amrwb filename=CIF_15fps_128kbps_16khz_15.85kbps_AMR_WB.mp4" >> /tmp/gst/gst_goo_h264_decoder.log
gst-launch filesrc location="/multimedia/repository/CIF_15fps_128kbps_16khz_15.85kbps_AMR_WB.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h264 ! goosink_pp \
demux.audio_00 ! queue ! goodec_wbamr ! dasfsink >> /tmp/gst/gst_goo_h264_decoder.log 2>&1

#H264 QCIF 15FPS AMR WB
echo "* H264 decoder qcif - uyvy - display - amrwb "
echo "" >> /tmp/gst/gst_goo_h264_decoder.log
echo "* H264 decoder qcif - uyvy - display - amrwb filename=QCIF_15fps_128kbps_16khz_6.60kbps_AMR_WB.mp4" >> /tmp/gst/gst_goo_h264_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h264/h264_with_ffmpeg_PROFILE_2.2/h264_amr_wb/QCIF_15fps_128kbps_16khz_6.60kbps_AMR_WB.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h264 ! goosink_pp \
demux.audio_00 ! queue ! goodec_wbamr ! dasfsink >> /tmp/gst/gst_goo_h264_decoder.log 2>&1

#H264 QCIF 15FPS MP3
echo "* H264 decoder qcif - uyvy - display - mp3 "
echo "" >> /tmp/gst/gst_goo_h264_decoder.log
echo "* H264 decoder qcif - uyvy - display - mp3 filename=H.264_QCIF_15fps_128kbps_MP3_44KHz_32kbps_ff_profile_2.2.mp4" >> /tmp/gst/gst_goo_h264_decoder.log
gst-launch filesrc location="/multimedia/repository/H.264_QCIF_15fps_128kbps_MP3_44KHz_32kbps_ff_profile_2.2.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h264 ! goosink_pp \
demux.audio_00 ! queue ! goodec_mp3 ! dasfsink >> /tmp/gst/gst_goo_h264_decoder.log 2>&1

#H264 CIF 15FPS MP3
echo "* H264 decoder cif - uyvy - display - mp3 "
echo "" >> /tmp/gst/gst_goo_h264_decoder.log
echo "* H264 decoder cif - uyvy - display - mp3 filename=H.264_CIF_15fps_128kbps_MP3_44KHz_128kbps_ff_profile_2.2.mp4" >> /tmp/gst/gst_goo_h264_decoder.log
gst-launch filesrc location="/multimedia/repository/H.264_CIF_15fps_128kbps_MP3_44KHz_128kbps_ff_profile_2.2.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h264 ! goosink_pp \
demux.audio_00 ! queue ! goodec_mp3 ! dasfsink >> /tmp/gst/gst_goo_h264_decoder.log 2>&1

#H264 QCIF 15FPS AAC
echo "* H264 decoder qcif - uyvy - display - aac "
echo "" >> /tmp/gst/gst_goo_h264_decoder.log
echo "* H264 decoder qcif - uyvy - display - aac filename=H.264_QCIF_15fps_128kbps_AAC_44.1KHz_24kbps.MP4" >> /tmp/gst/gst_goo_h264_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h264/h264_with_ffmpeg_PROFILE_2.2/h264_aac/H.264_QCIF_15fps_128kbps_AAC_44.1KHz_24kbps.MP4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h264 ! goosink_pp \
demux.audio_00 ! queue ! goodec_aac ! dasfsink >> /tmp/gst/gst_goo_h264_decoder.log 2>&1

#H264 CIF 15FPS AAC
echo "* H264 decoder cif - uyvy - display - aac "
echo "" >> /tmp/gst/gst_goo_h264_decoder.log
echo "* H264 decoder cif - uyvy - display - aac filename=H.264_CIF_15fps_128kbps_AAC_44KHz,128kbps.MP4" >> /tmp/gst/gst_goo_h264_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h264/h264_with_ffmpeg_PROFILE_2.2/h264_aac/H.264_CIF_15fps_128kbps_AAC_44KHz,128kbps.MP4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h264 ! goosink_pp \
demux.audio_00 ! queue ! goodec_aac ! dasfsink >> /tmp/gst/gst_goo_h264_decoder.log 2>&1

echo "Starting with H264 combo tests with OMX Clock"

#H264 CIF 15FPS AMR NB
echo "* H264 decoder cif - uyvy - display - amrnb "
echo "" >> /tmp/gst/gst_goo_h264_decoder.log
echo "* H264 decoder cif - uyvy - display - amrnb filename=CIF_15fps_128kbps_8khz_7.40_Amr_NB.mp4" >> /tmp/gst/gst_goo_h264_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h264/h264_with_ffmpeg_PROFILE_2.2/h264_amr_nb/CIF_15fps_128kbps_8khz_7.40_Amr_NB.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h264 ! goosink_pp \
demux.audio_00 ! queue ! goodec_nbamr ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_h264_decoder.log 2>&1

#H264 QCIF 15FPS AMR NB
echo "* H264 decoder qcif - uyvy - display - amrnb "
echo "" >> /tmp/gst/gst_goo_h264_decoder.log
echo "* H264 decoder qcif - uyvy - display - amrnb filename=QCIF_15fps_128kbps_8khz_4.5kbps_AMR_NB.mp4" >> /tmp/gst/gst_goo_h264_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h264/h264_with_ffmpeg_PROFILE_2.2/h264_amr_nb/QCIF_15fps_128kbps_8khz_4.5kbps_AMR_NB.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h264 ! goosink_pp \
demux.audio_00 ! queue ! goodec_nbamr ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_h264_decoder.log 2>&1

#H264 CIF 15FPS AMR WB
echo "* H264 decoder cif - uyvy - display - amrwb "
echo "" >> /tmp/gst/gst_goo_h264_decoder.log
echo "* H264 decoder cif - uyvy - display - amrwb filename=CIF_15fps_128kbps_16khz_15.85kbps_AMR_WB.mp4" >> /tmp/gst/gst_goo_h264_decoder.log
gst-launch filesrc location="/multimedia/repository/CIF_15fps_128kbps_16khz_15.85kbps_AMR_WB.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h264 ! goosink_pp \
demux.audio_00 ! queue ! goodec_wbamr ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_h264_decoder.log 2>&1

#H264 QCIF 15FPS AMR WB
echo "* H264 decoder qcif - uyvy - display - amrwb "
echo "" >> /tmp/gst/gst_goo_h264_decoder.log
echo "* H264 decoder qcif - uyvy - display - amrwb filename=QCIF_15fps_128kbps_16khz_6.60kbps_AMR_WB.mp4" >> /tmp/gst/gst_goo_h264_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h264/h264_with_ffmpeg_PROFILE_2.2/h264_amr_wb/QCIF_15fps_128kbps_16khz_6.60kbps_AMR_WB.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h264 ! goosink_pp \
demux.audio_00 ! queue ! goodec_wbamr ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_h264_decoder.log 2>&1

#H264 QCIF 15FPS MP3
echo "* H264 decoder qcif - uyvy - display - mp3 "
echo "" >> /tmp/gst/gst_goo_h264_decoder.log
echo "* H264 decoder qcif - uyvy - display - mp3 filename=H.264_QCIF_15fps_128kbps_MP3_44KHz_32kbps_ff_profile_2.2.mp4" >> /tmp/gst/gst_goo_h264_decoder.log
gst-launch filesrc location="/multimedia/repository/H.264_QCIF_15fps_128kbps_MP3_44KHz_32kbps_ff_profile_2.2.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h264 ! goosink_pp \
demux.audio_00 ! queue ! goodec_mp3 ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_h264_decoder.log 2>&1

#H264 CIF 15FPS MP3
echo "* H264 decoder cif - uyvy - display - mp3 "
echo "" >> /tmp/gst/gst_goo_h264_decoder.log
echo "* H264 decoder cif - uyvy - display - mp3 filename=H.264_CIF_15fps_128kbps_MP3_44KHz_128kbps_ff_profile_2.2.mp4" >> /tmp/gst/gst_goo_h264_decoder.log
gst-launch filesrc location="/multimedia/repository/H.264_CIF_15fps_128kbps_MP3_44KHz_128kbps_ff_profile_2.2.mp4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h264 ! goosink_pp \
demux.audio_00 ! queue ! goodec_mp3 ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_h264_decoder.log 2>&1

#H264 QCIF 15FPS AAC
echo "* H264 decoder qcif - uyvy - display - aac "
echo "" >> /tmp/gst/gst_goo_h264_decoder.log
echo "* H264 decoder qcif - uyvy - display - aac filename=H.264_QCIF_15fps_128kbps_AAC_44.1KHz_24kbps.MP4" >> /tmp/gst/gst_goo_h264_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h264/h264_with_ffmpeg_PROFILE_2.2/h264_aac/H.264_QCIF_15fps_128kbps_AAC_44.1KHz_24kbps.MP4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h264 ! goosink_pp \
demux.audio_00 ! queue ! goodec_aac ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_h264_decoder.log 2>&1

#H264 CIF 15FPS AAC
echo "* H264 decoder cif - uyvy - display - aac "
echo "" >> /tmp/gst/gst_goo_h264_decoder.log
echo "* H264 decoder cif - uyvy - display - aac filename=H.264_CIF_15fps_128kbps_AAC_44KHz,128kbps.MP4" >> /tmp/gst/gst_goo_h264_decoder.log
gst-launch filesrc location="/multimedia-repository/Video_Files/containers/h264/h264_with_ffmpeg_PROFILE_2.2/h264_aac/H.264_CIF_15fps_128kbps_AAC_44KHz,128kbps.MP4" ! \
ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux \
demux.video_00 ! queue ! goodec_h264 ! goosink_pp \
demux.audio_00 ! queue ! goodec_aac ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_h264_decoder.log 2>&1















