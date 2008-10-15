#!/bin/sh

echo "Looking for error messages"
cat $TARGET/tmp/gst/gst_goo_mpeg4_decoder.log | grep -n -r error
echo "Looking for killed messages"
cat $TARGET/tmp/gst/gst_goo_mpeg4_decoder.log | grep -n -r killed
echo "Looking for segfaults messages"
cat $TARGET/tmp/gst/gst_goo_mpeg4_decoder.log | grep -n -r fault

mplayer $TARGET/tmp/gst/goodec_mpeg4_qcif_i420.yuv -demuxer rawvideo  -rawvideo qcif:fps=15 > /dev/null 2>&1