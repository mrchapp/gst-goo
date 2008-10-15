#!/bin/sh
WBAMRDECLOG=/tmp/gst/gst_goo_wbamr_decoder.log
mkdir -p /tmp/gst

echo "Starting WBAMR decoder tests"

echo " WBAMR DECODER @16KHz 24kbps "
echo "" >> $WBAMRDECLOG
echo " WBAMR DECODER @16KHz 24kbps " >> $WBAMRDECLOG
gst-launch filesrc location=/omx/patterns/T08_2385.cod ! audio/AMR-WB,width=16,depth=16,rate=16000,channels=1 ! goodec_wbamr mime=0 process-mode=1 ! dasfsink >> $WBAMRDECLOG 2>&1

echo " WBAMR DECODER @16KHz 15.85kbps "
echo "" >> $WBAMRDECLOG
echo " WBAMR DECODER @16KHz 15.85kbps " >> $WBAMRDECLOG
gst-launch filesrc location=/omx/patterns/T08_1585.cod ! audio/AMR-WB,width=16,depth=16,rate=16000,channels=1,endianness=1234 ! goodec_wbamr mime=0 process-mode=1 ! dasfsink >> $WBAMRDECLOG 2>&1

echo " WBAMR DECODER @16KHz 6.60kbps "
echo "" >> $WBAMRDECLOG
echo " WBAMR DECODER @16KHz 6.60kbps " >> $WBAMRDECLOG
gst-launch filesrc location=/omx/patterns/T08_660.cod ! audio/AMR-WB,width=16,depth=16,rate=16000,channels=1,endianness=1234 ! goodec_wbamr mime=0 process-mode=1 ! dasfsink >> $WBAMRDECLOG 2>&1
