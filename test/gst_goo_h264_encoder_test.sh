#!/bin/sh

mkdir -p /tmp/gst

echo "Starting with H264 encoder with camera and videotestsrc"


for LEVEL in 1 2 8
 do
for BITRATE in 128000 
 do

echo "* H264 GOOCAMERA YUY2  QCIF LEVEL $LEVEL BITRATE $BITRATE"
echo "" >> /tmp/gst/gst_goo_h264_encoder.log
echo "* H264 GOOCAMERA YUY2  QCIF LEVEL $LEVEL BITRATE $BITRATE" >> /tmp/gst/gst_goo_h264_encoder.log
gst-launch goocamera num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=176, height=144, framerate=15/1 ! gooenc_h264 level=$LEVEL bitrate=$BITRATE ! video/x-h264, width=176, height=144, framerate=15/1 ! \
filesink location=/tmp/gst/gooenc-h264-qcif-yuy2-15fps-level$LEVEL-$BITRATE-bps.264 >> /tmp/gst/gst_goo_h264_encoder.log 2>&1

echo "* H264 GOOCAMERA YUY2  CIF LEVEL $LEVEL BITRATE $BITRATE "
echo "" >> /tmp/gst/gst_goo_h264_encoder.log
echo "* H264 GOOCAMERA YUY2  CIF LEVEL $LEVEL BITRATE $BITRATE" >> /tmp/gst/gst_goo_h264_encoder.log
gst-launch goocamera num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=352, height=288, framerate=15/1 ! gooenc_h264 level=$LEVEL bitrate=$BITRATE ! video/x-h264, width=352, height=288, framerate=15/1 ! \
filesink location=/tmp/gst/gooenc-h264-cif-yuy2-15fps-level$LEVEL-$BITRATE-bps.264 >> /tmp/gst/gst_goo_h264_encoder.log 2>&1

echo "* H264 VIDEOTESTSRC I420  CIF LEVEL $LEVEL BITRATE $BITRATE "
echo "" >> /tmp/gst/gst_goo_h264_encoder.log
echo "* H264 VIDEOTESTSRC I420  CIF LEVEL $LEVEL BITRATE $BITRATE" >> /tmp/gst/gst_goo_h264_encoder.log
gst-launch videotestsrc num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)I420, width=352, height=288, framerate=15/1 ! gooenc_h264 level=$LEVEL bitrate=$BITRATE ! video/x-h264, width=352, height=288, framerate=15/1 ! \
filesink location=/tmp/gst/gooenc-h264-cif-i420-15fps-level$LEVEL-$BITRATE-bps.264 >> /tmp/gst/gst_goo_h264_encoder.log 2>&1

echo "* H264 VIDEOTESTSRC I420  QCIF LEVEL $LEVEL BITRATE $BITRATE "
echo "" >> /tmp/gst/gst_goo_h264_encoder.log
echo "* H264 VIDEOTESTSRC I420  QCIF LEVEL $LEVEL BITRATE $BITRATE" >> /tmp/gst/gst_goo_h264_encoder.log
gst-launch videotestsrc num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)I420, width=176, height=144, framerate=15/1 ! gooenc_h264 level=$LEVEL bitrate=$BITRATE ! video/x-h264, width=176, height=144, framerate=15/1 ! \
filesink location=/tmp/gst/gooenc-h264-qcif-i420-15fps-level$LEVEL-$BITRATE-bps.264 >> /tmp/gst/gst_goo_h264_encoder.log 2>&1

done
done

for LEVEL in 8 16 32 64 128 256
 do
for BITRATE in 768000 
 do

echo "* H264 GOOCAMERA YUY2  CIF LEVEL $LEVEL BITRATE $BITRATE "
echo "" >> /tmp/gst/gst_goo_h264_encoder.log
echo "* H264 GOOCAMERA YUY2  CIF LEVEL $LEVEL BITRATE $BITRATE" >> /tmp/gst/gst_goo_h264_encoder.log
gst-launch goocamera num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=352, height=288, framerate=15/1 ! gooenc_h264 level=$LEVEL bitrate=$BITRATE ! video/x-h264, width=352, height=288, framerate=15/1 ! \
filesink location=/tmp/gst/gooenc-h264-cif-yuy2-15fps-level$LEVEL-$BITRATE-bps.264 >> /tmp/gst/gst_goo_h264_encoder.log 2>&1

echo "* H264 VIDEOTESTSRC I420  CIF LEVEL $LEVEL BITRATE $BITRATE "
echo "" >> /tmp/gst/gst_goo_h264_encoder.log
echo "* H264 VIDEOTESTSRC I420  CIF LEVEL $LEVEL BITRATE $BITRATE" >> /tmp/gst/gst_goo_h264_encoder.log
gst-launch videotestsrc num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)I420, width=352, height=288, framerate=15/1 ! gooenc_h264 level=$LEVEL bitrate=$BITRATE ! video/x-h264, width=352, height=288, framerate=15/1 ! \
filesink location=/tmp/gst/gooenc-h264-cif-i420-15fps-level$LEVEL-$BITRATE-bps.264 >> /tmp/gst/gst_goo_h264_encoder.log 2>&1

done
done

echo "Starting with H264 encoder camera inside a container"

for LEVEL in 1 2 8
 do
for BITRATE in 128000 
 do

echo "* H264 GOOCAMERA YUY2  QCIF LEVEL $LEVEL BITRATE $BITRATE MP4"
echo "" >> /tmp/gst/gst_goo_h264_encoder.log
echo "* H264 GOOCAMERA YUY2  QCIF LEVEL $LEVEL BITRATE $BITRATE MP4" >> /tmp/gst/gst_goo_h264_encoder.log
gst-launch ffmux_mp4 name=mux ! filesink location=/tmp/gst/gooenc-h264-qcif-yuy2-15fps-level$LEVEL-$BITRATE-bps.mp4  \
goocamera num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=176, height=144, framerate=15/1 ! gooenc_h264 level=$LEVEL bitrate=$BITRATE ! \
mux. >> /tmp/gst/gst_goo_h264_encoder.log 2>&1

echo "* H264 GOOCAMERA YUY2  CIF LEVEL $LEVEL BITRATE $BITRATE MP4"
echo "" >> /tmp/gst/gst_goo_h264_encoder.log
echo "* H264 GOOCAMERA YUY2  CIF LEVEL $LEVEL BITRATE $BITRATE MP4" >> /tmp/gst/gst_goo_h264_encoder.log
gst-launch ffmux_mp4 name=mux ! filesink location=/tmp/gst/gooenc-h264-cif-yuy2-15fps-level$LEVEL-$BITRATE-bps.mp4  \
goocamera num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=352, height=288, framerate=15/1 ! gooenc_h264 level=$LEVEL bitrate=$BITRATE ! \
mux. >> /tmp/gst/gst_goo_h264_encoder.log 2>&1

done
done

for LEVEL in 8 16 32 64 128 256
 do
for BITRATE in 768000
 do

echo "* H264 GOOCAMERA YUY2  QCIF LEVEL $LEVEL BITRATE $BITRATE MP4"
echo "" >> /tmp/gst/gst_goo_h264_encoder.log
echo "* H264 GOOCAMERA YUY2  QCIF LEVEL $LEVEL BITRATE $BITRATE MP4" >> /tmp/gst/gst_goo_h264_encoder.log
gst-launch ffmux_mp4 name=mux ! filesink location=/tmp/gst/gooenc-h264-qcif-yuy2-15fps-level$LEVEL-$BITRATE-bps.mp4  \
goocamera num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=176, height=144, framerate=15/1 ! gooenc_h264 level=$LEVEL bitrate=$BITRATE ! \
mux. >> /tmp/gst/gst_goo_h264_encoder.log 2>&1

echo "* H264 GOOCAMERA YUY2  CIF LEVEL $LEVEL BITRATE $BITRATE MP4"
echo "" >> /tmp/gst/gst_goo_h264_encoder.log
echo "* H264 GOOCAMERA YUY2  CIF LEVEL $LEVEL BITRATE $BITRATE MP4" >> /tmp/gst/gst_goo_h264_encoder.log
gst-launch ffmux_mp4 name=mux ! filesink location=/tmp/gst/gooenc-h264-cif-yuy2-15fps-level$LEVEL-$BITRATE-bps.mp4  \
goocamera num-buffers=50 ! video/x-raw-yuv, format=\(fourcc\)YUY2, width=352, height=288, framerate=15/1 ! gooenc_h264 level=$LEVEL bitrate=$BITRATE ! \
mux. >> /tmp/gst/gst_goo_h264_encoder.log 2>&1

done
done
x