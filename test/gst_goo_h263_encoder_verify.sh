#!/bin/sh

echo "Looking for error messages"
cat $TARGET/tmp/gst/gst_goo_h263_encoder.log | grep -n -r error
echo "Looking for killed messages"
cat $TARGET/tmp/gst/gst_goo_h263_encoder.log | grep -n -r killed
echo "Looking for segfaults messages"
cat $TARGET/tmp/gst/gst_goo_h263_encoder.log | grep -n -r fault

#H263
for LEVEL in 1 2 4 8
 do

for BITRATE in 128000 386000
 do

mplayer $TARGET/tmp/gst/gooenc-h263-qcif-yuy2-15fps-level$LEVEL-$BITRATE-bps.263 > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-h263-cif-yuy2-15fps-level$LEVEL-$BITRATE-bps.263 > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-h263-cif-i420-15fps-level$LEVEL-$BITRATE-bps.263 > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-h263-qcif-i420-15fps-level$LEVEL-$BITRATE-bps.263 > /dev/null 2>&1 

done
done

for LEVEL in 1 2 4 8
 do

for BITRATE in 128000 386000
 do

mplayer $TARGET/tmp/gst/gooenc-h263-qcif-yuy2-15fps-level$LEVEL-$BITRATE-bps.3gp > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-h263-cif-yuy2-15fps-level$LEVEL-$BITRATE-bps.3gp > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-h263-cif-i420-15fps-level$LEVEL-$BITRATE-bps.3gp > /dev/null 2>&1
mplayer $TARGET/tmp/gst/gooenc-h263-qcif-i420-15fps-level$LEVEL-$BITRATE-bps.3gp > /dev/null 2>&1 

done
done
