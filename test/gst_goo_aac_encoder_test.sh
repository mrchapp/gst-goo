#!/bin/sh
AACLOGFILE=/tmp/gst/gst_goo_aac_encoder.log
mkdir -p /tmp/gst

echo "Starting AAC encoder tests"

for PROFILE in 1 2 3
 do 
for SAMPLERATE in 44100 48000
 do
for BITRATE in 128000 64000 8000
 do

for BITRATEMODE in 0 1 2 3 4 5
 do

for FILEFORMAT in 0 1 2
 do

echo "AAC encoded @$SAMPLERATE Hz bit-rate $BITRATE bit-rate-mode $BITRATEMODE file-format $FILEFORMAT profile $PROFILE"
echo "" >> $AACLOGFILE
echo "AAC encoded @$SAMPLERATE Hz bit-rate $BITRATE bit-rate-mode $BITRATEMODE file-format $FILEFORMAT " >> $AACLOGFILE
gst-launch dasfsrc num-buffers=400 ! audio/x-raw-int,rate=$SAMPLERATE,channels=1, width=16, depth=16, endianness=1234 ! \
gooenc_aac bit-rate=$BITRATE bit-rate-mode=$BITRATEMODE file-format=$FILEFORMAT profile=$PROFILE process-mode=0 ! \
audio/mpeg, mpegversion=4, rate=$SAMPLERATE, channels=1 ! \
filesink location=/tmp/gst/gooenc-aac-$(($SAMPLERATE/1000))KHz-$(($BITRATE/1000))-bitrate-$BITRATEMODE-fileformat-$FILEFORMAT-profile.aac >> $AACLOGFILE 2>&1
done
done
done
done
done
