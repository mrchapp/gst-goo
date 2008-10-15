#!/bin/sh

mkdir -p /tmp/gst

echo "Starting with MPEG4 encoder standalone"


for LEVEL in 1 2 4 8 10
 do
for BITRATE in 384000 
 do

echo "* MPEG4 GOOCAMERA YUY2  QCIF LEVEL $LEVEL BITRATE $BITRATE"
echo "" >> /tmp/gst/gst_goo_mpeg4_encoder.log
echo "* MPEG4 GOOCAMERA YUY2  QCIF LEVEL $LEVEL BITRATE $BITRATE" >> /tmp/gst/gst_goo_mpeg4_encoder.log
gst-launch goocamera num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=176, height=144, framerate=15/1 ! gooenc_mpeg4 level=$LEVEL bitrate=$BITRATE ! \
filesink location=/tmp/gst/gooenc-mpeg4-qcif-yuy2-15fps-level$LEVEL-$BITRATE-bps.m4v >> /tmp/gst/gst_goo_mpeg4_encoder.log 2>&1

echo "* MPEG4 GOOCAMERA YUY2  CIF LEVEL $LEVEL BITRATE $BITRATE "
echo "" >> /tmp/gst/gst_goo_mpeg4_encoder.log
echo "* MPEG4 GOOCAMERA YUY2  CIF LEVEL $LEVEL BITRATE $BITRATE" >> /tmp/gst/gst_goo_mpeg4_encoder.log
gst-launch goocamera num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=352, height=288, framerate=15/1 ! gooenc_mpeg4 level=$LEVEL bitrate=$BITRATE ! \
filesink location=/tmp/gst/gooenc-mpeg4-cif-yuy2-15fps-level$LEVEL-$BITRATE-bps.m4v >> /tmp/gst/gst_goo_mpeg4_encoder.log 2>&1

echo "* MPEG4 VIDEOTESTSRC I420  CIF LEVEL $LEVEL BITRATE $BITRATE "
echo "" >> /tmp/gst/gst_goo_mpeg4_encoder.log
echo "* MPEG4 VIDEOTESTSRC I420  CIF LEVEL $LEVEL BITRATE $BITRATE" >> /tmp/gst/gst_goo_mpeg4_encoder.log
gst-launch videotestsrc num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)I420, width=352, height=288, framerate=15/1 ! gooenc_mpeg4 level=$LEVEL bitrate=$BITRATE ! \
filesink location=/tmp/gst/gooenc-mpeg4-cif-i420-15fps-level$LEVEL-$BITRATE-bps.m4v >> /tmp/gst/gst_goo_mpeg4_encoder.log 2>&1

echo "* MPEG4 VIDEOTESTSRC I420  QCIF LEVEL $LEVEL BITRATE $BITRATE "
echo "" >> /tmp/gst/gst_goo_mpeg4_encoder.log
echo "* MPEG4 VIDEOTESTSRC I420  QCIF LEVEL $LEVEL BITRATE $BITRATE" >> /tmp/gst/gst_goo_mpeg4_encoder.log
gst-launch videotestsrc num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)I420, width=176, height=144, framerate=15/1 ! gooenc_mpeg4 level=$LEVEL bitrate=$BITRATE ! \
filesink location=/tmp/gst/gooenc-mpeg4-qcif-i420-15fps-level$LEVEL-$BITRATE-bps.m4v >> /tmp/gst/gst_goo_mpeg4_encoder.log 2>&1

done
done

for LEVEL in 20 40
 do
for BITRATE in 2000000 
 do

echo "* MPEG4 GOOCAMERA YUY2  CIF LEVEL $LEVEL BITRATE $BITRATE "
echo "" >> /tmp/gst/gst_goo_mpeg4_encoder.log
echo "* MPEG4 GOOCAMERA YUY2  CIF LEVEL $LEVEL BITRATE $BITRATE" >> /tmp/gst/gst_goo_mpeg4_encoder.log
gst-launch goocamera num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=352, height=288, framerate=15/1 ! gooenc_mpeg4 level=$LEVEL bitrate=$BITRATE ! \
filesink location=/tmp/gst/gooenc-mpeg4-cif-yuy2-15fps-level$LEVEL-$BITRATE-bps.m4v >> /tmp/gst/gst_goo_mpeg4_encoder.log 2>&1

echo "* MPEG4 VIDEOTESTSRC I420  CIF LEVEL $LEVEL BITRATE $BITRATE "
echo "" >> /tmp/gst/gst_goo_mpeg4_encoder.log
echo "* MPEG4 VIDEOTESTSRC I420  CIF LEVEL $LEVEL BITRATE $BITRATE" >> /tmp/gst/gst_goo_mpeg4_encoder.log
gst-launch videotestsrc num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)I420, width=352, height=288, framerate=15/1 ! gooenc_mpeg4 level=$LEVEL bitrate=$BITRATE ! \
filesink location=/tmp/gst/gooenc-mpeg4-cif-i420-15fps-level$LEVEL-$BITRATE-bps.m4v >> /tmp/gst/gst_goo_mpeg4_encoder.log 2>&1

echo "* MPEG4 GOOCAMERA YUY2  VGA LEVEL $LEVEL BITRATE $BITRATE "
echo "" >> /tmp/gst/gst_goo_mpeg4_encoder.log
echo "* MPEG4 GOOCAMERA YUY2  VGA LEVEL $LEVEL BITRATE $BITRATE" >> /tmp/gst/gst_goo_mpeg4_encoder.log
gst-launch goocamera num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=640, height=480, framerate=15/1 ! gooenc_mpeg4 level=$LEVEL bitrate=$BITRATE ! \
filesink location=/tmp/gst/gooenc-mpeg4-vga-yuy2-15fps-level$LEVEL-$BITRATE-bps.m4v >> /tmp/gst/gst_goo_mpeg4_encoder.log 2>&1

echo "* MPEG4 VIDEOTESTSRC I420  VGA LEVEL $LEVEL BITRATE $BITRATE "
echo "" >> /tmp/gst/gst_goo_mpeg4_encoder.log
echo "* MPEG4 VIDEOTESTSRC I420  VGA LEVEL $LEVEL BITRATE $BITRATE" >> /tmp/gst/gst_goo_mpeg4_encoder.log
gst-launch videotestsrc num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)I420, width=640, height=480, framerate=15/1 ! gooenc_mpeg4 level=$LEVEL bitrate=$BITRATE ! \
filesink location=/tmp/gst/gooenc-mpeg4-vga-i420-15fps-level$LEVEL-$BITRATE-bps.m4v >> /tmp/gst/gst_goo_mpeg4_encoder.log 2>&1

done
done

echo "Starting with MPEG4 encoder camera inside a container"

for LEVEL in 1 2 4 8 10
 do
for BITRATE in 384000 
 do

echo "* MPEG4 GOOCAMERA YUY2  QCIF LEVEL $LEVEL BITRATE $BITRATE MP4"
echo "" >> /tmp/gst/gst_goo_mpeg4_encoder.log
echo "* MPEG4 GOOCAMERA YUY2  QCIF LEVEL $LEVEL BITRATE $BITRATE MP4" >> /tmp/gst/gst_goo_mpeg4_encoder.log
gst-launch ffmux_mp4 name=mux ! filesink location=/tmp/gst/gooenc-mpeg4-qcif-yuy2-15fps-level$LEVEL-$BITRATE-bps.mp4  \
goocamera num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=176, height=144, framerate=15/1 ! gooenc_mpeg4 level=$LEVEL bitrate=$BITRATE ! \
mux. >> /tmp/gst/gst_goo_mpeg4_encoder.log 2>&1

echo "* MPEG4 GOOCAMERA YUY2  CIF LEVEL $LEVEL BITRATE $BITRATE MP4"
echo "" >> /tmp/gst/gst_goo_mpeg4_encoder.log
echo "* MPEG4 GOOCAMERA YUY2  CIF LEVEL $LEVEL BITRATE $BITRATE MP4" >> /tmp/gst/gst_goo_mpeg4_encoder.log
gst-launch ffmux_mp4 name=mux ! filesink location=/tmp/gst/gooenc-mpeg4-cif-yuy2-15fps-level$LEVEL-$BITRATE-bps.mp4  \
goocamera num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=352, height=288, framerate=15/1 ! gooenc_mpeg4 level=$LEVEL bitrate=$BITRATE ! \
mux. >> /tmp/gst/gst_goo_mpeg4_encoder.log 2>&1


done
done

for LEVEL in 20 40
 do
for BITRATE in 2000000 
 do

echo "* MPEG4 GOOCAMERA YUY2  VGA LEVEL $LEVEL BITRATE $BITRATE MP4"
echo "" >> /tmp/gst/gst_goo_mpeg4_encoder.log
echo "* MPEG4 GOOCAMERA YUY2  VGA LEVEL $LEVEL BITRATE $BITRATE MP4" >> /tmp/gst/gst_goo_mpeg4_encoder.log
gst-launch ffmux_mp4 name=mux ! filesink location=/tmp/gst/gooenc-mpeg4-vga-yuy2-30fps-level$LEVEL-$BITRATE-bps.mp4  \
goocamera num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=640, height=480, framerate=30/1 ! gooenc_mpeg4 level=$LEVEL bitrate=$BITRATE ! \
mux. >> /tmp/gst/gst_goo_mpeg4_encoder.log 2>&1


done
done





