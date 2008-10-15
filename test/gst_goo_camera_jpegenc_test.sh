#!/bin/sh

mkdir -p /tmp/gst

# qvga
echo "* qvga"
gst-launch goocamera num-buffers="10" brightness="100" contrast="100" output-buffers="2" ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=320, height=240, framerate=0/1 ! gooenc_jpeg quality="100" input-buffers="2" output-buffers="2" ! multifilesink location=/tmp/gst/camera_qvga_%d.jpeg

# vga
echo "* vga"
gst-launch goocamera num-buffers="10" brightness="100" contrast="100" output-buffers="2" ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=640, height=480, framerate=0/1 ! gooenc_jpeg quality="100" input-buffers="2" output-buffers="2" ! multifilesink location=/tmp/gst/camera_vga_%d.jpeg

# custom
echo "* custom"
gst-launch goocamera num-buffers="5" brightness="70" brightness="10" contrast="-90" ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=720, height=560, framerate=0/1 ! gooenc_jpeg ! multifilesink location=/tmp/gst/camera_720x560_%d.jpeg

# svga
echo "* svga"
gst-launch goocamera num-buffers="3" brightness="10" contrast="-90" ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=800, height=600, framerate=0/1 ! gooenc_jpeg ! multifilesink location=/tmp/gst/camera_svga_%d.jpeg

# xga
echo "* xga"
gst-launch goocamera num-buffers="1" brightness="100" contrast="100" ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=1024, height=768, framerate=0/1 ! gooenc_jpeg ! filesink location=/tmp/gst/camera_xga.jpeg

# sxga
echo "* sxga"
gst-launch goocamera num-buffers="1" brightness="100" contrast="100" ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=1280, height=1024, framerate=0/1 ! gooenc_jpeg quality="50" ! filesink location=/tmp/gst/camera_sxga.jpeg

# uxga
echo "* uxga"
gst-launch goocamera num-buffers="1" brightness="10" contrast="-90" ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=1600, height=1200, framerate=0/1 ! gooenc_jpeg quality="50" ! filesink location=/tmp/gst/camera_uxga.jpeg
