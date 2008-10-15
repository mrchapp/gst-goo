#!/bin/sh

mkdir -p /tmp/gst

#==========[ images ]=================================================

echo "vpp & jpeg decoder"

for f in 4MP_internet_seq.jpg penguin_UXGA.jpg stone_1600x1200.jpg 5MP_internet_seq.jpg 6MP_internet_seq.jpg stockholm.jpg; do
    gst-launch filesrc location=/omx/patterns/$f ! goodec_jpeg ! goofilter_vpp ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=320, height=240 ! goosink_pp rotation=90
done

echo "vpp & jpeg encoder"

gst-launch filesrc location=/omx/patterns/uxga.yuv blocksize="3840000" ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=1600, height=1200, framerate=0/1 ! goofilter_vpp rotation=90 ! video/x-raw-yuv, format=\(fourcc\)I420, width=480, height=640 ! gooenc_jpeg ! filesink location=/tmp/gst/uxga2vga.jpeg

gst-launch filesrc location=/omx/patterns/stockholm.yuv blocksize="3538944" ! video/x-raw-yuv, width=1536, height=1152, format=\(fourcc\)UYVY, framerate=0/1 ! goofilter_vpp ! video/x-raw-yuv, width=640, height=480 ! gooenc_jpeg ! filesink location=/tmp/gst/stockholm2vga.jpeg

echo "videotestsrc & jpeg encoder"
gst-launch videotestsrc num-buffers=1 ! goofilter_vpp rotation=270 ! gooenc_jpeg ! filesink location=/tmp/gst/videotestsrc.jpeg

#==========[ video ]=================================================

echo "videotestsrc"
gst-launch-probe --padprobe=goosinkpp0:sink --timer videotestsrc num-buffers=100 ! goofilter_vpp rotation=270 ! goofilter_vpp ! goosink_pp -v

echo "3gp MPEG4+MP3"
gst-launch-probe --padprobe=goosinkpp0:sink --timer filesrc location="/multimedia/repository/MPEG4QCIF_15fps_64kbps_MP344KHz,32kbps.mp4" ! ffdemux_mov_mp4_m4a_3gp_3g2_mj2 name=demux demux.video_00 ! queue ! goodec_mpeg4 ! goofilter_vpp !  video/x-raw-yuv, width=320, height=240 ! goosink_pp rotation=90 demux.audio_00 ! queue ! goodec_mp3 ! dasfsink
