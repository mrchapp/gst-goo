#!/bin/sh

mkdir -p /tmp/gst

# qcif blocksize="50688"
echo "* qcif UYVY"
gst-launch filesrc location=/omx/patterns/JPGE_CONF_003.yuv ! video/x-raw-yuv, width=176, height=144, format=\(fourcc\)UYVY,framerate=0/1 ! gooenc_jpeg ! filesink location=/tmp/gst/JPGE_CONF_003.jpeg

# qcif blocksize="38016"
echo "* qcif I420"
gst-launch filesrc location=/omx/patterns/JPGE_CONF_011.yuv ! video/x-raw-yuv, width=176, height=144, format=\(fourcc\)I420,framerate=0/1 ! gooenc_jpeg ! filesink location=/tmp/gst/JPGE_CONF_011.jpeg

# qvga blocksize="153600"
echo "* qvga UYVY"
gst-launch filesrc location=/omx/patterns/JPGE_CONF_004.yuv ! video/x-raw-yuv, width=320, height=240, format=\(fourcc\)UYVY,framerate=0/1 ! gooenc_jpeg ! filesink location=/tmp/gst/JPGE_CONF_004.jpeg

# cif blocksize="202752"
echo "* cif UYVY"
gst-launch filesrc location=/omx/patterns/JPGE_CONF_005.yuv ! video/x-raw-yuv, width=352, height=288, format=\(fourcc\)UYVY,framerate=0/1 ! gooenc_jpeg ! filesink location=/tmp/gst/JPGE_CONF_005.jpeg

# vga blocksize="614400"
echo "* vga UYVY"
gst-launch filesrc location=/omx/patterns/JPGE_CONF_006.yuv ! video/x-raw-yuv, width=640, height=480, format=\(fourcc\)UYVY,framerate=0/1 ! gooenc_jpeg ! filesink location=/tmp/gst/JPGE_CONF_006.jpeg

# svga blocksize="960000"
#gst-launch filesrc location=/tmp/gst/camera_shot_svga.yuv ! video/x-raw-yuv, width=800, height=600, format=\(fourcc\)UYVY,framerate=0/1 ! gooenc_jpeg ! filesink location=/tmp/gst/camera_shot_svga.jpeg

# ??? blocksize="811008"
# gst-launch filesrc location=/omx/patterns/JPGE_CONF_007.yuv ! video/x-raw-yuv, width=320, height=240, format=\(fourcc\)UYVY,framerate=0/1 ! gooenc_jpeg ! filesink location=/tmp/gst/JPGE_CONF_007.jpeg

# custom blocksize="811008"
echo "* 704x576 I420"
gst-launch filesrc location=/omx/patterns/JPGE_CONF_037.yuv !  video/x-raw-yuv, width=704, height=576, format=\(fourcc\)I420, framerate=0/1 ! gooenc_jpeg ! filesink location=/tmp/gst/JPGE_CONF_037.jpeg

# sxga blocksize="2621440"
echo "* sxga UYVY"
gst-launch filesrc location=/omx/patterns/sxga.yuv ! video/x-raw-yuv, width=1280, height=1024, format=\(fourcc\)UYVY,framerate=0/1 ! gooenc_jpeg ! filesink location=/tmp/gst/sxga.jpeg

# uxga blocksize="3840000"
echo "* uxga UYVY"
gst-launch filesrc location=/omx/patterns/uxga.yuv ! video/x-raw-yuv, width=1600, height=1200, format=\(fourcc\)UYVY,framerate=0/1 ! gooenc_jpeg ! filesink location=/tmp/gst/uxga.jpeg

# custom blocksize="3538944"
echo "* 1536x1152 UYVY"
gst-launch filesrc location=/omx/patterns/stockholm.yuv ! video/x-raw-yuv, width=1536, height=1152, format=\(fourcc\)UYVY, framerate=0/1 ! gooenc_jpeg ! filesink location=/tmp/gst/stockholm.jpeg

# custom blocksize="11520000"
echo "* 1600x3600 UYVY"
gst-launch filesrc location=/omx/patterns/6MP_1600x3600.yuv ! video/x-raw-yuv, width=1600, height=3600, format=\(fourcc\)UYVY, framerate=0/1 ! gooenc_jpeg ! filesink location=/tmp/gst/6MP_1600x3600.jpeg

# custom
# not valid input
#gst-launch filesrc location=/omx/patterns/7_8MP_1600x4800.yuv blocksize="15360000" !  video/x-raw-yuv, width=1600, height=4800, format=\(fourcc\)UYVY,framerate=0/1 ! gooenc_jpeg ! filesink location=/tmp/gst/7_8MP_1600x4800.jpeg
