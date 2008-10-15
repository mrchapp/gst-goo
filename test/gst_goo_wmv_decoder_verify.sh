#!/bin/sh

echo "Looking for error messages"
cat $TARGET/tmp/gst/gst_goo_wmv_decoder.log | grep -n -r error
echo "Looking for killed messages"
cat $TARGET/tmp/gst/gst_goo_wmv_decoder.log | grep -n -r killed
echo "Looking for segfaults messages"
cat $TARGET/tmp/gst/gst_goo_wmv_decoder.log | grep -n -r fault

mplayer $TARGET/tmp/gst/goodec_wmv_vc1_qcif_i420.yuv -demuxer rawvideo  -rawvideo qcif:fps=15 > /dev/null 2>&1
mplayer $TARGET/tmp/gst/goodec_wmv_vc1_cif_i420.yuv -demuxer rawvideo  -rawvideo qcif:fps=15 > /dev/null 2>&1