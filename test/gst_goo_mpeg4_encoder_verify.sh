#!/bin/sh

echo "Looking for error messages"
cat $TARGET/tmp/gst/gst_goo_mpeg4_encoder.log | grep -i -n -r error
echo "Looking for killed messages"
cat $TARGET/tmp/gst/gst_goo_mpeg4_encoder.log | grep -i -n -r killed
echo "Looking for segfaults messages"
cat $TARGET/tmp/gst/gst_goo_mpeg4_encoder.log | grep -i -n -r fault

for LEVEL in 1 2 4 8 10
 do
for BITRATE in 384000 
 do

mplayer $TARGET/tmp/gst/gooenc-mpeg4-qcif-yuy2-15fps-level$LEVEL-$BITRATE-bps.m4v > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-mpeg4-cif-yuy2-15fps-level$LEVEL-$BITRATE-bps.m4v  > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-mpeg4-cif-i420-15fps-level$LEVEL-$BITRATE-bps.m4v  > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-mpeg4-qcif-i420-15fps-level$LEVEL-$BITRATE-bps.m4v > /dev/null 2>&1 

done
done

for LEVEL in 20 40
 do
for BITRATE in 2000000
 do

mplayer $TARGET/tmp/gst/gooenc-mpeg4-cif-yuy2-15fps-level$LEVEL-$BITRATE-bps.m4v > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-mpeg4-cif-i420-15fps-level$LEVEL-$BITRATE-bps.m4v > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-mpeg4-vga-yuy2-15fps-level$LEVEL-$BITRATE-bps.m4v > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-mpeg4-vga-i420-15fps-level$LEVEL-$BITRATE-bps.m4v > /dev/null 2>&1

done
done

for LEVEL in 0 1 2 3 100
 do
for BITRATE in 384000 
 do

mplayer $TARGET/tmp/gst/gooenc-mpeg4-qcif-yuy2-15fps-level$LEVEL-$BITRATE-bps.mp4 > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-mpeg4-cif-yuy2-15fps-level$LEVEL-$BITRATE-bps.mp4  > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-mpeg4-cif-i420-15fps-level$LEVEL-$BITRATE-bps.mp4  > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-mpeg4-qcif-i420-15fps-level$LEVEL-$BITRATE-bps.mp4 > /dev/null 2>&1 

done
done

for LEVEL in 20 40 
 do
for BITRATE in 2000000
 do

mplayer $TARGET/tmp/gst/gooenc-mpeg4-cif-yuy2-15fps-level$LEVEL-$BITRATE-bps.mp4 > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-mpeg4-cif-i420-15fps-level$LEVEL-$BITRATE-bps.mp4 > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-mpeg4-vga-yuy2-15fps-level$LEVEL-$BITRATE-bps.mp4 > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-mpeg4-vga-i420-15fps-level$LEVEL-$BITRATE-bps.mp4 > /dev/null 2>&1

done
done

    
