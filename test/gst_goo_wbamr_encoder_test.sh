#!/bin/sh
# == variables ==
WBAMRLOGFILE=/tmp/gst/gst_goo_wbamr_encoder.log
mkdir -p /tmp/gst

echo "Starting WBAMR encoder tests"

echo " WBAMR encoded @16KHz bit-rate 6.60kbps "
echo "" >> $WBAMRLOGFILE
echo " WBAMR encoded @16KHz bit-rate 6.60kbps " >> $WBAMRLOGFILE
gst-launch dasfsrc num-buffers=1000 ! audio/x-raw-int,rate=16000,channels=1,width=16,depth=16,endianness=1234 ! gooenc_wbamr mime=1 dtx-mode=0 band-mode=1 acdn-mode=0 header=1 ! filesink location=/tmp/gst/gooenc-wbamr-16KHz-6.60kbps.amr >> $WBAMRLOGFILE 2>&1

echo " WBAMR encoded @16KHz bit-rate 8.85kbps "
echo "" >> $WBAMRLOGFILE
echo " WBAMR encoded @16KHz bit-rate 8.85kbps " >> $WBAMRLOGFILE
gst-launch dasfsrc num-buffers=1000 ! audio/x-raw-int,rate=16000,channels=1,width=16,depth=16,endianness=1234 ! gooenc_wbamr mime=1 dtx-mode=0 band-mode=2 acdn-mode=0 header=1 ! filesink location=/tmp/gst/gooenc-wbamr-16KHz-8.85kbps.amr >> $WBAMRLOGFILE 2>&1

echo " WBAMR encoded @16KHz bit-rate 12.65kbps "
echo "" >> $WBAMRLOGFILE
echo " WBAMR encoded @16KHz bit-rate 12.65kbps " >> $WBAMRLOGFILE
gst-launch dasfsrc num-buffers=1000 ! audio/x-raw-int,rate=16000,channels=1,width=16,depth=16,endianness=1234 ! gooenc_wbamr mime=1 dtx-mode=0 band-mode=3 acdn-mode=0 header=1 ! filesink location=/tmp/gst/gooenc-wbamr-16KHz-12.65kbps.amr >> $WBAMRLOGFILE 2>&1

echo " WBAMR encoded @16KHz bit-rate 14.25kbps "
echo "" >> $WBAMRLOGFILE
echo " WBAMR encoded @16KHz bit-rate 14.25kbps " >> $WBAMRLOGFILE
gst-launch dasfsrc num-buffers=1000 ! audio/x-raw-int,rate=16000,channels=1,width=16,depth=16,endianness=1234 ! gooenc_wbamr mime=1 dtx-mode=0 band-mode=4 acdn-mode=0 header=1 ! filesink location=/tmp/gst/gooenc-wbamr-16KHz-14.25kbps.amr >> $WBAMRLOGFILE 2>&1

echo " WBAMR encoded @16KHz bit-rate 15.85kbps "
echo "" >> $WBAMRLOGFILE
echo " WBAMR encoded @16KHz bit-rate 15.85kbps " >> $WBAMRLOGFILE
gst-launch dasfsrc num-buffers=1000 ! audio/x-raw-int,rate=16000,channels=1,width=16,depth=16,endianness=1234 ! gooenc_wbamr mime=1 dtx-mode=0 band-mode=5 acdn-mode=0 header=1 ! filesink location=/tmp/gst/gooenc-wbamr-16KHz-15.85kbps.amr >> $WBAMRLOGFILE 2>&1

echo " WBAMR encoded @16KHz bit-rate 18.25kbps "
echo "" >> $WBAMRLOGFILE
echo " WBAMR encoded @16KHz bit-rate 18.25kbps " >> $WBAMRLOGFILE
gst-launch dasfsrc num-buffers=1000 ! audio/x-raw-int,rate=16000,channels=1,width=16,depth=16,endianness=1234 ! gooenc_wbamr mime=1 dtx-mode=0 band-mode=6 acdn-mode=0 header=1 ! filesink location=/tmp/gst/gooenc-wbamr-16KHz-18.25kbps.amr >> $WBAMRLOGFILE 2>&1

echo " WBAMR encoded @16KHz bit-rate 19.85kbps "
echo "" >> $WBAMRLOGFILE
echo " WBAMR encoded @16KHz bit-rate 19.85kbps " >> $WBAMRLOGFILE
gst-launch dasfsrc num-buffers=1000 ! audio/x-raw-int,rate=16000,channels=1,width=16,depth=16,endianness=1234 ! gooenc_wbamr mime=1 dtx-mode=0 band-mode=7 acdn-mode=0 header=1 ! filesink location=/tmp/gst/gooenc-wbamr-16KHz-19.85kbps.amr >> $WBAMRLOGFILE 2>&1

echo " WBAMR encoded @16KHz bit-rate 23.05kbps "
echo "" >> $WBAMRLOGFILE
echo " WBAMR encoded @16KHz bit-rate 23.05kbps " >> $WBAMRLOGFILE
gst-launch dasfsrc num-buffers=1000 ! audio/x-raw-int,rate=16000,channels=1,width=16,depth=16,endianness=1234 ! gooenc_wbamr mime=1 dtx-mode=0 band-mode=8 acdn-mode=0 header=1 ! filesink location=/tmp/gst/gooenc-wbamr-16KHz-23.05.amr >> $WBAMRLOGFILE 2>&1

echo " WBAMR encoded @16KHz bit-rate 23.85kbps "
echo "" >> $WBAMRLOGFILE
echo " WBAMR encoded @16KHz bit-rate 23.85kbps " >> $WBAMRLOGFILE
gst-launch dasfsrc num-buffers=1000 ! audio/x-raw-int,rate=16000,channels=1,width=16,depth=16,endianness=1234 ! gooenc_wbamr mime=1 dtx-mode=0 band-mode=9 acdn-mode=0 header=1 ! filesink location=/tmp/gst/gooenc-wbamr-16KHz-23.85.amr >> $WBAMRLOGFILE 2>&1

