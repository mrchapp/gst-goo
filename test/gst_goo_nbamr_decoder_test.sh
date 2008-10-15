#!/bin/sh
NBAMRDECLOG=/tmp/gst/gst_goo_nbamr_decoder.log
mkdir -p /tmp/gst

echo "Starting NBAMR decoder tests"

echo " NBAMR DECODER @8KHz 12.2kbps "
echo "" >> $NBAMRDECLOG
echo " NBAMR DECODER @8KHz 12.2kbps "  >> $NBAMRDECLOG
gst-launch filesrc location=/omx/patterns/T06_122.amr ! audio/AMR, width=16,depth=16,rate=8000, channels=1,endianness=1234 ! goodec_nbamr mime=0 process-mode=1 ! dasfsink >> $NBAMRDECLOG 2>&1

echo " NBAMR DECODER @8KHz 7.40kbps "
echo "" >> $NBAMRDECLOG
echo " NBAMR DECODER @8KHz 7.40kbps "  >> $NBAMRDECLOG
gst-launch filesrc location=/omx/patterns/T02_74.amr ! audio/AMR, width=16,depth=16,rate=8000, channels=1,endianness=1234 ! goodec_nbamr mime=0 process-mode=1 ! dasfsink >> $NBAMRDECLOG 2>&1

echo " NBAMR DECODER @8KHz 4.75kbps "
echo "" >> $NBAMRDECLOG
echo " NBAMR DECODER @8KHz 4.75kbps "  >> $NBAMRDECLOG
gst-launch filesrc location=/omx/patterns/T04_475dtx_mime.amr ! amrnbparse ! audio/AMR,width=16,depth=16,rate=8000,channels=1,endianness=1234 ! goodec_nbamr mime=1 dtx-mode=1 ! dasfsink >> $NBAMRDECLOG 2>&1
