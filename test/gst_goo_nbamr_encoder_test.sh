#!/bin/sh
NBAMRLOGFILE=/tmp/gst/gst_goo_nbamr_encoder.log
mkdir -p /tmp/gst

echo "Starting NBAMR encoder tests"

echo " NBAMR encoded @8KHz bit-rate 4.75kbps "
echo "" >> $NBAMRLOGFILE
echo " NBAMR encoded @8KHz bit-rate 4.75kbps " >> $NBAMRLOGFILE
gst-launch dasfsrc num-buffers=500 ! audio/x-raw-int,rate=8000,channels=1,width=16,depth=16,endianness=1234 ! gooenc_nbamr mime=1 dtx-mode=0 band-mode=1 acdn-mode=0 header=1 ! filesink location=/tmp/gst/gooenc-nbamr-8KHz-4.75kbps.amr >> $NBAMRLOGFILE 2>&1

echo " NBAMR encoded @8KHz bit-rate 5.15kbps "
echo "" >> $NBAMRLOGFILE
echo " NBAMR encoded @8KHz bit-rate 5.15kbps " >> $NBAMRLOGFILE
gst-launch dasfsrc num-buffers=500 ! audio/x-raw-int,rate=8000,channels=1,width=16,depth=16,endianness=1234 ! gooenc_nbamr mime=1 dtx-mode=0 band-mode=2 acdn-mode=0 header=1 ! filesink location=/tmp/gst/gooenc-nbamr-8KHz-5.15kbps.amr >> $NBAMRLOGFILE 2>&1

echo " NBAMR encoded @8KHz bit-rate 5.90kbps "
echo "" >> $NBAMRLOGFILE
echo " NBAMR encoded @8KHz bit-rate 5.90kbps " >> $NBAMRLOGFILE
gst-launch dasfsrc num-buffers=500 ! audio/x-raw-int,rate=8000,channels=1,width=16,depth=16,endianness=1234 ! gooenc_nbamr mime=1 dtx-mode=0 band-mode=3 acdn-mode=0 header=1 ! filesink location=/tmp/gst/gooenc-nbamr-8KHz-5.90kbps.amr >> $NBAMRLOGFILE 2>&1

echo " NBAMR encoded @8KHz bit-rate 6.70kbps "
echo "" >> $NBAMRLOGFILE
echo " NBAMR encoded @8KHz bit-rate 6.70kbps " >> $NBAMRLOGFILE
gst-launch dasfsrc num-buffers=500 ! audio/x-raw-int,rate=8000,channels=1,width=16,depth=16,endianness=1234 ! gooenc_nbamr mime=1 dtx-mode=0 band-mode=4 acdn-mode=0 header=1 ! filesink location=/tmp/gst/gooenc-nbamr-8KHz-6.70kbps.amr >> $NBAMRLOGFILE 2>&1

echo " NBAMR encoded @8KHz bit-rate 7.40kbps "
echo "" >> $NBAMRLOGFILE
echo " NBAMR encoded @8KHz bit-rate 7.40kbps " >> $NBAMRLOGFILE
gst-launch dasfsrc num-buffers=500 ! audio/x-raw-int,rate=8000,channels=1,width=16,depth=16,endianness=1234 ! gooenc_nbamr mime=1 dtx-mode=0 band-mode=5 acdn-mode=0 header=1 ! filesink location=/tmp/gst/gooenc-nbamr-8KHz-7.40kbps.amr >> $NBAMRLOGFILE 2>&1

echo " NBAMR encoded @8KHz bit-rate 7.95kbps "
echo "" >> $NBAMRLOGFILE
echo " NBAMR encoded @8KHz bit-rate 7.95kbps " >> $NBAMRLOGFILE
gst-launch dasfsrc num-buffers=500 ! audio/x-raw-int,rate=8000,channels=1,width=16,depth=16,endianness=1234 ! gooenc_nbamr mime=1 dtx-mode=0 band-mode=6 acdn-mode=0 header=1 ! filesink location=/tmp/gst/gooenc-nbamr-8KHz-7.95kbps.amr >> $NBAMRLOGFILE 2>&1

echo " NBAMR encoded @8KHz bit-rate 10.2kbps "
echo "" >> $NBAMRLOGFILE
echo " NBAMR encoded @8KHz bit-rate 10.2kbps " >> $NBAMRLOGFILE
gst-launch dasfsrc num-buffers=500 ! audio/x-raw-int,rate=8000,channels=1,width=16,depth=16,endianness=1234 ! gooenc_nbamr mime=1 dtx-mode=0 band-mode=7 acdn-mode=0 header=1 ! filesink location=/tmp/gst/gooenc-nbamr-8KHz-10.2kbps.amr >> $NBAMRLOGFILE 2>&1

echo " NBAMR encoded @8KHz bit-rate 12.2kbps "
echo "" >> $NBAMRLOGFILE
echo " NBAMR encoded @8KHz bit-rate 12.2kbps " >> $NBAMRLOGFILE
gst-launch dasfsrc num-buffers=500 ! audio/x-raw-int,rate=8000,channels=1,width=16,depth=16,endianness=1234 ! gooenc_nbamr mime=1 dtx-mode=0 band-mode=8 acdn-mode=0 header=1 ! filesink location=/tmp/gst/gooenc-nbamr-8KHz-12.2kbps.amr >> $NBAMRLOGFILE 2>&1
