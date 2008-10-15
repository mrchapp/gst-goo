#!/bin/sh

echo "* qcif UYVY 30/1 layer 1"
gst-launch videotestsrc num-buffers="50" ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=176, height=144, framerate=30/1 ! goosink_pp videolayer=1 x-pos=20 y-pos=20

echo "* cif YUY2 30/1 layer 2 mirror"
gst-launch videotestsrc num-buffers="50" ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=352, height=288, framerate=30/1 ! goosink_pp videolayer=2 mirror=true

echo "* qvga UYVY 30/1 layer 1 rotated"
gst-launch videotestsrc num-buffers="50" ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=320, height=240, framerate=30/1 ! goosink_pp videolayer=1 rotation=180

echo "* vga YUY2 30/1 layer 2"
gst-launch videotestsrc num-buffers="50" ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=640, height=480, framerate=30/1 ! goosink_pp videolayer=2

echo "* vga YUY2 30/1 layer 2 scaled"
gst-launch videotestsrc num-buffers="50" ! video/x-raw-yuv, format=\(fourcc\)UYVY, width=640, height=480, framerate=30/1 ! goosink_pp videolayer=2 x-scale=50 y-scale=50
