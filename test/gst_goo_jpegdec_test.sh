#!/bin/sh

mkdir -p /tmp/gst

# 1
echo "4MP_internet_seq.jpg"
gst-launch filesrc location=/omx/patterns/4MP_internet_seq.jpg ! goodec_jpeg resize=12 ! filesink location=/tmp/gst/4MP_internet_seq.yuv

# 2
echo "penguin_UXGA.jpg"
gst-launch filesrc location=/omx/patterns/penguin_UXGA.jpg ! goodec_jpeg resize=25 ! filesink location=/tmp/gst/penguin_UXGA.yuv

# 3
echo "stone_1600x1200.jpg"
gst-launch filesrc location=/omx/patterns/stone_1600x1200.jpg ! goodec_jpeg resize=50 ! filesink location=/tmp/gst/stone_1600x1200.yuv

# 4
echo "5MP_internet_seq.jpg"
gst-launch filesrc location=/omx/patterns/5MP_internet_seq.jpg ! goodec_jpeg resize=100 ! fakesink -v

# 5 
echo "6MP_internet_seq.jpg"
gst-launch filesrc location=/omx/patterns/6MP_internet_seq.jpg ! goodec_jpeg ! fakesink -v

# 6
echo "stockholm.jpg"
gst-launch filesrc location=/omx/patterns/stockholm.jpg ! goodec_jpeg ! fakesink -v

# 7
echo "wind_1280x960.jpg"
gst-launch filesrc location=/omx/patterns/wind_1280x960.jpg ! goodec_jpeg ! filesink location=/tmp/gst/wind_1280x960.yuv

# 8
echo "flower.jpg"
gst-launch filesrc location=/omx/patterns/flower.jpg ! goodec_jpeg ! filesink location=/tmp/gst/flower.yuv

# 9
echo "64K_Exif.jpg"
gst-launch filesrc location=/omx/patterns/64K_Exif.jpg ! goodec_jpeg ! filesink location=/tmp/gst/64K_Exif.yuv

# 10
echo "shrek.jpg"
gst-launch filesrc location=/omx/patterns/shrek.jpg ! goodec_jpeg ! filesink location=/tmp/gst/shrek.yuv

# 11
echo "laugh.jpg"
gst-launch filesrc location=/omx/patterns/laugh.jpg ! goodec_jpeg ! filesink location=/tmp/gst/laugh.yuv
