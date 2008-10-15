#!/bin/sh
AACDECLOG=/tmp/gst/gst_goo_aac_decoder.log
mkdir -p /tmp/gst

echo "Starting AAC decoder tests"

echo " AAC decoding @44.1KHz 128kbps "
echo "" >> $AACDECLOG
echo " AAC decoding @44.1KHz 128kbps " >> $AACDECLOG
gst-launch filesrc location=/multimedia-repository/Audio_Files/aac/aac/dog_aac_44khz_128kbps.aac blocksize=2000 ! audio/mpeg,mpegversion=4,rate=44100,channels=2 ! goodec_aac RAW=0 SBR=0 parametric-stereo=0 ! dasfsink >> $AACDECLOG 2>&1

echo " AAC decoding @44.1KHz 64kbps " >> $AACDECLOG
echo "" >> $AACDECLOG
echo " AAC decoding @44.1KHz 64kbps " >> $AACDECLOG
gst-launch filesrc location=/multimedia-repository/Audio_Files/aac/aac/44khz_64kbps_stereo.aac blocksize=2000 ! audio/mpeg,mpegversion=4,rate=44100,channels=2 ! goodec_aac RAW=0 SBR=0 parametric-stereo=0 ! dasfsink >> $AACDECLOG 2>&1

echo " AAC decoding @44.1KHz 24kbps " >> $AACDECLOG
echo "" >> $AACDECLOG
echo " AAC decoding @44.1KHz 24kbps " >> $AACDECLOG
gst-launch filesrc location=/multimedia-repository/Audio_Files/aac/aac/44khz_24kbps_stereo.aac blocksize=2000 ! audio/mpeg,mpegversion=4,rate=44100,channels=2 ! goodec_aac RAW=0 SBR=0 parametric-stereo=0 ! dasfsink >> $AACDECLOG 2>&1
