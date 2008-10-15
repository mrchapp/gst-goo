#!/bin/sh

mkdir -p /tmp/gst

echo "Starting with H263 encoder standalone"


for LEVEL in 1 2 4 8
 do

for BITRATE in 128000 386000
 do

echo "* H263 GOOCAMERA YUY2  QCIF LEVEL $LEVEL BITRATE $BITRATE"
echo "" >> /tmp/gst/gst_goo_h263_encoder.log
echo "* H263 GOOCAMERA YUY2  QCIF LEVEL $LEVEL BITRATE $BITRATE" >> /tmp/gst/gst_goo_h263_encoder.log
gst-launch goocamera num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=176, height=144, framerate=15/1 ! gooenc_h263 level=$LEVEL bitrate=$BITRATE ! video/x-h263, width=176, height=144, framerate=15/1 ! \
filesink location=/tmp/gst/gooenc-h263-qcif-yuy2-15fps-level$LEVEL-$BITRATE-bps.263 >> /tmp/gst/gst_goo_h263_encoder.log 2>&1

echo "* H263 GOOCAMERA YUY2  CIF LEVEL $LEVEL BITRATE $BITRATE "
echo "" >> /tmp/gst/gst_goo_h263_encoder.log
echo "* H263 GOOCAMERA YUY2  CIF LEVEL $LEVEL BITRATE $BITRATE" >> /tmp/gst/gst_goo_h263_encoder.log
gst-launch goocamera num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=352, height=288, framerate=15/1 ! gooenc_h263 level=$LEVEL bitrate=$BITRATE ! video/x-h263, width=352, height=288, framerate=15/1 ! \
filesink location=/tmp/gst/gooenc-h263-cif-yuy2-15fps-level$LEVEL-$BITRATE-bps.263 >> /tmp/gst/gst_goo_h263_encoder.log 2>&1

echo "* H263 VIDEOTESTSRC I420  CIF LEVEL $LEVEL BITRATE $BITRATE "
echo "" >> /tmp/gst/gst_goo_h263_encoder.log
echo "* H263 VIDEOTESTSRC I420  CIF LEVEL $LEVEL BITRATE $BITRATE" >> /tmp/gst/gst_goo_h263_encoder.log
gst-launch videotestsrc num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)I420, width=352, height=288, framerate=15/1 ! gooenc_h263 level=$LEVEL bitrate=$BITRATE ! video/x-h263, width=352, height=288, framerate=15/1 ! \
filesink location=/tmp/gst/gooenc-h263-cif-i420-15fps-level$LEVEL-$BITRATE-bps.263 >> /tmp/gst/gst_goo_h263_encoder.log 2>&1

echo "* H263 VIDEOTESTSRC I420  QCIF LEVEL $LEVEL BITRATE $BITRATE "
echo "" >> /tmp/gst/gst_goo_h263_encoder.log
echo "* H263 VIDEOTESTSRC I420  QCIF LEVEL $LEVEL BITRATE $BITRATE" >> /tmp/gst/gst_goo_h263_encoder.log
gst-launch videotestsrc num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)I420, width=176, height=144, framerate=15/1 ! gooenc_h263 level=$LEVEL bitrate=$BITRATE ! video/x-h263, width=176, height=144, framerate=15/1 ! \
filesink location=/tmp/gst/gooenc-h263-qcif-i420-15fps-level$LEVEL-$BITRATE-bps.263 >> /tmp/gst/gst_goo_h263_encoder.log 2>&1


done
done

echo "Starting with H263 encoder camera inside a container"

for LEVEL in 1 2 4 8
 do

for BITRATE in 128000 386000
 do

echo "* H263 GOOCAMERA YUY2  QCIF LEVEL $LEVEL BITRATE $BITRATE 3GP"
echo "" >> /tmp/gst/gst_goo_h263_encoder.log
echo "* H263 GOOCAMERA YUY2  QCIF LEVEL $LEVEL BITRATE $BITRATE 3GP" >> /tmp/gst/gst_goo_h263_encoder.log
gst-launch ffmux_3g2 name=mux ! filesink location=/tmp/gst/gooenc-h263-qcif-yuy2-15fps-level$LEVEL-$BITRATE-bps.3gp  \
goocamera num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=176, height=144, framerate=15/1 ! gooenc_h263 level=$LEVEL bitrate=$BITRATE ! \
mux. >> /tmp/gst/gst_goo_h263_encoder.log 2>&1

echo "* H263 GOOCAMERA YUY2  CIF LEVEL $LEVEL BITRATE $BITRATE 3GP"
echo "" >> /tmp/gst/gst_goo_h263_encoder.log
echo "* H263 GOOCAMERA YUY2  CIF LEVEL $LEVEL BITRATE $BITRATE 3GP" >> /tmp/gst/gst_goo_h263_encoder.log
gst-launch ffmux_3g2 name=mux ! filesink location=/tmp/gst/gooenc-h263-cif-yuy2-15fps-level$LEVEL-$BITRATE-bps.3gp  \
goocamera num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=352, height=288, framerate=15/1 ! gooenc_h263 level=$LEVEL bitrate=$BITRATE ! \
mux. >> /tmp/gst/gst_goo_h263_encoder.log 2>&1


done
done








