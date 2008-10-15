#!/bin/sh

echo "* 12KHz mono"
gst-launch filesrc location=/omx/patterns/m12.wav ! audio/x-raw-int, channels=1, rate=12000, width=16, depth=16  ! goodec_pcm ! dasfsink 

echo "* 8Hz mono"
gst-launch filesrc location=/omx/patterns/m08.wav ! audio/x-raw-int, channels=1, rate=8000, width=16, depth=16 ! goodec_pcm ! dasfsink 

echo "* 32KHz mono"
gst-launch filesrc location=/omx/patterns/m32.wav ! audio/x-raw-int, channels=1, rate=32000, width=16, depth=16 ! goodec_pcm ! dasfsink 

echo "* 44.1KHz mono"
gst-launch filesrc location=/omx/patterns/m44.wav ! audio/x-raw-int, channels=1, rate=44100, width=16, depth=16 ! goodec_pcm ! dasfsink 

echo "* 48KHz mono"
gst-launch filesrc location=/omx/patterns/m48.wav ! audio/x-raw-int, channels=1, rate=48000, width=16, depth=16 ! goodec_pcm ! dasfsink

echo "* 32KHz stereo"
gst-launch filesrc location=/omx/patterns/s32.wav ! audio/x-raw-int, channels=2, rate=32000, width=16, depth=16 ! goodec_pcm ! dasfsink 

echo "* 44.1KHz stereo"
gst-launch filesrc location=/omx/patterns/s44.wav ! audio/x-raw-int, channels=2, rate=44100, width=16, depth=16 ! goodec_pcm ! dasfsink

echo "* 48KHz mono"
gst-launch filesrc location=/omx/patterns/m48.wav ! audio/x-raw-int, channels=1, rate=48000, width=16, depth=16  ! goodec_pcm ! dasfsink 
