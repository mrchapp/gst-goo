#!/bin/sh
PCMENCLOG=/tmp/gst/gst_goo_pcm_encoder.log
mkdir -p /tmp/gst

echo "Starting PCM encoder tests"

echo " PCM encoded @8KHz "
echo "" >> $PCMENCLOG
echo " PCM encoded @8KHz " >> $PCMENCLOG
gst-launch dasfsrc num-buffers=2500 ! audio/x-raw-int,rate=8000,channels=1,width=16,depth=16,endianness=1234 ! gooenc_pcm ! audio/x-raw-int,rate=8000,channels=1,width=16,depth=16,endianness=1234 ! filesink location=/tmp/gst/gooenc-pcm-8KHz.pcm >> $PCMENCLOG 2>&1

echo " PCM encoded @16KHz "
echo "" >> $PCMENCLOG
echo " PCM encoded @16KHz " >> $PCMENCLOG
gst-launch dasfsrc num-buffers=2500 ! audio/x-raw-int,rate=16000,channels=1,width=16,depth=16,endianness=1234 ! gooenc_pcm ! audio/x-raw-int,rate=16000,channels=1,width=16,depth=16,endianness=1234 ! filesink location=/tmp/gst/gooenc-pcm-16KHz.pcm >> $PCMENCLOG 2>&1

echo " PCM encoded @44.1KHz "
echo "" >> $PCMENCLOG
echo " PCM encoded @44.1KHz " >> $PCMENCLOG
gst-launch dasfsrc num-buffers=2500 ! audio/x-raw-int,rate=44100,channels=1,width=16,depth=16,endianness=1234 ! gooenc_pcm ! audio/x-raw-int,rate=44100,channels=1,width=16,depth=16,endianness=1234 ! filesink location=/tmp/gst/gooenc-pcm-44.1KHz.pcm >> $PCMENCLOG 2>&1
