#!/bin/sh

mkdir -p /tmp/gst

#==========[ images ]=================================================

# qcif
echo "* shot qcif"
gst-launch goocamera num-buffers="10" brightness="10" contrast="-90" ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=176, height=144, framerate=0/1 ! filesink location=/tmp/gst/camera_shot_qcif.yuv

# cif
echo "* shot cif"
gst-launch goocamera num-buffers="10" brightness="10" contrast="-90" ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=352, height=288, framerate=0/1 ! filesink location=/tmp/gst/camera_shot_cif.yuv

# qvga
echo "* shot qvga"
gst-launch goocamera num-buffers="10" brightness="100" contrast="100" ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=320, height=240, framerate=0/1 ! filesink location=/tmp/gst/camera_shot_qvga.yuv

# vga
echo "* shot vga"
gst-launch goocamera num-buffers="10" brightness="100" contrast="100" ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=640, height=480, framerate=0/1 ! filesink location=/tmp/gst/camera_shot_vga.yuv

# custom
echo "* shot custom"
gst-launch goocamera num-buffers="5" brightness="70" contrast="-90" ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=720, height=560, framerate=0/1 ! filesink location=/tmp/gst/camera_shot_720x560.yuv

# svga
echo "* shot svga"
gst-launch goocamera num-buffers="3" brightness="10" contrast="-90" ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=800, height=600, framerate=0/1 ! filesink location=/tmp/gst/camera_shot_svga.yuv

# xga
echo "* shot xga"
gst-launch goocamera num-buffers="1" brightness="100" contrast="100" ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=1024, height=768, framerate=0/1 ! filesink location=/tmp/gst/camera_shot_xga.yuv

# sxga
echo "* shot sxga"
gst-launch goocamera num-buffers="1" brightness="100" contrast="100" ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=1280, height=1024, framerate=0/1 ! filesink location=/tmp/gst/camera_shot_sxga.yuv

# uxga
echo "* shot uxga"
gst-launch goocamera num-buffers="1" brightness="10" contrast="-90" ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=1600, height=1200, framerate=0/1 ! filesink location=/tmp/gst/camera_shot_uxga.yuv

#==========[ videos ]=================================================

# qcif
echo "* video qcif"
gst-launch goocamera num-buffers="30" ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=176, height=144, framerate=30/1 ! fakesink -v

# cif
echo "* video cif"
gst-launch goocamera num-buffers="30" ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=352, height=288, framerate=30/1 ! fakesink -v

# qvga
echo "* video qvga"
gst-launch goocamera num-buffers="30" ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=320, height=240, framerate=30/1 ! fakesink -v

# vga
echo "* video vga"
gst-launch goocamera num-buffers="30" ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=640, height=480, framerate=30/1 ! fakesink -v
