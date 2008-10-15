#!/bin/sh

echo "Looking for error messages"
cat $TARGET/tmp/gst/gst_goo_h264_encoder.log | grep -i -n -r error
echo "Looking for killed messages"
cat $TARGET/tmp/gst/gst_goo_h264_encoder.log | grep -i -n -r killed
echo "Looking for segfaults messages"
cat $TARGET/tmp/gst/gst_goo_h264_encoder.log | grep -i -n -r fault

for LEVEL in 1 2 8
 do
for BITRATE in 128000 
 do

mplayer $TARGET/tmp/gst/gooenc-h264-qcif-yuy2-15fps-level$LEVEL-$BITRATE-bps.264 -fps 15 > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-h264-cif-yuy2-15fps-level$LEVEL-$BITRATE-bps.264  -fps 15 > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-h264-cif-i420-15fps-level$LEVEL-$BITRATE-bps.264  -fps 15 > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-h264-qcif-i420-15fps-level$LEVEL-$BITRATE-bps.264  -fps 15 > /dev/null 2>&1 

done
done

for LEVEL in 8 16 32 64 128 256
 do
for BITRATE in 768000 
 do

mplayer $TARGET/tmp/gst/gooenc-h264-cif-yuy2-15fps-level$LEVEL-$BITRATE-bps.264  -fps 15 > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-h264-cif-i420-15fps-level$LEVEL-$BITRATE-bps.264  -fps 15 > /dev/null 2>&1

done
done

for LEVEL in 1 2 8
 do
for BITRATE in 128000 
 do

mplayer $TARGET/tmp/gst/gooenc-h264-qcif-yuy2-15fps-level$LEVEL-$BITRATE-bps.mp4 -fps 15 > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-h264-cif-yuy2-15fps-level$LEVEL-$BITRATE-bps.mp4  -fps 15 > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-h264-cif-i420-15fps-level$LEVEL-$BITRATE-bps.mp4  -fps 15 > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-h264-qcif-i420-15fps-level$LEVEL-$BITRATE-bps.mp4  -fps 15 > /dev/null 2>&1 

done
done

for LEVEL in 8 16 32 64 128 256
 do
for BITRATE in 768000 
 do

mplayer $TARGET/tmp/gst/gooenc-h264-cif-yuy2-15fps-level$LEVEL-$BITRATE-bps.mp4  -fps 15 > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-h264-cif-i420-15fps-level$LEVEL-$BITRATE-bps.mp4  -fps 15 > /dev/null 2>&1

done
done
  
