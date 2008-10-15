#!/bin/sh

mkdir -p /tmp/gst

echo "Starting with WMV decoder to display and file sink"

# WMV VC1 QCIF - UYVY - DISPLAY
echo "* WMV decoder vc1 qcif - uyvy - display "
echo "" >> /tmp/gst/gst_goo_wmv_decoder.log
echo "* WMV decoder vc1 qcif - uyvy - display filename=SA00040_qcif.vc1" > /tmp/gst/gst_goo_wmv_decoder.log
gst-launch filesrc location= /omx/patterns/SA00040_qcif.vc1 ! \
video/x-wmv,wmvversion=3,width=176,height=144,framerate=15/1 ! \
goodec_wmv process-mode=1 ! \
video/x-raw-yuv, format=\(fourcc\)UYVY, width=176, height=144, framerate=15/1 ! \
goosink_pp max-lateness=-1 >> /tmp/gst/gst_goo_wmv_decoder.log 2>&1

# WMV VC1 QCIF - I420 - DISPLAY
echo "* WMV decoder vc1 qcif - i420 - filesink "
echo "" >> /tmp/gst/gst_goo_wmv_decoder.log
echo "* WMV decoder vc1 qcif - i420 - filesink filename=SA00040_qcif.vc1" >> /tmp/gst/gst_goo_wmv_decoder.log
gst-launch filesrc location= /omx/patterns/SA00040_qcif.vc1 ! \
video/x-wmv,wmvversion=3,width=176,height=144,framerate=15/1 ! \
goodec_wmv process-mode=1 ! \
video/x-raw-yuv, format=\(fourcc\)I420, width=176, height=144, framerate=15/1 ! \
filesink location=/tmp/gst/goodec_wmv_vc1_qcif_i420.yuv >> /tmp/gst/gst_goo_wmv_decoder.log 2>&1

# WMV VC1 CIF - UYVY - DISPLAY
echo "* WMV decoder vc1 cif - uyvy - display "
echo "" >> /tmp/gst/gst_goo_wmv_decoder.log
echo "* WMV decoder vc1 cif - uyvy - display filename=SA00045_cif.vc1" >> /tmp/gst/gst_goo_wmv_decoder.log
gst-launch filesrc location= /omx/patterns/SA00045_cif.vc1 ! \
video/x-wmv,wmvversion=3,width=352,height=288,framerate=15/1 ! \
goodec_wmv process-mode=1 ! \
video/x-raw-yuv, format=\(fourcc\)UYVY, width=352, height=288, framerate=15/1 ! \
goosink_pp max-lateness=-1 >> /tmp/gst/gst_goo_wmv_decoder.log 2>&1

# WMV VC1 QCIF - I420 - DISPLAY
echo "* WMV decoder vc1 qcif - i420 - filesink "
echo "" >> /tmp/gst/gst_goo_wmv_decoder.log
echo "* WMV decoder vc1 qcif - i420 - filesink filename=SA00040_qcif.vc1" >> /tmp/gst/gst_goo_wmv_decoder.log
gst-launch filesrc location= /omx/patterns/SA00040_qcif.vc1 ! \
video/x-wmv,wmvversion=3,width=176,height=144,framerate=15/1 ! \
goodec_wmv process-mode=1 ! \
video/x-raw-yuv, format=\(fourcc\)I420, width=352, height=288, framerate=15/1 ! \
filesink location=/tmp/gst/goodec_wmv_vc1_cif_i420.yuv >> /tmp/gst/gst_goo_wmv_decoder.log 2>&1

# WMV VC1 CIF - UYVY - DISPLAY
echo "* WMV decoder vc1 cif - uyvy - display "
echo "" >> /tmp/gst/gst_goo_wmv_decoder.log
echo "* WMV decoder vc1 cif - uyvy - display filename=SA00045_cif.vc1" >> /tmp/gst/gst_goo_wmv_decoder.log
gst-launch filesrc location= /omx/patterns/SA00045_cif.vc1 ! \
video/x-wmv,wmvversion=3,width=352,height=288,framerate=15/1 ! \
goodec_wmv process-mode=1 ! \
video/x-raw-yuv, format=\(fourcc\)UYVY, width=352, height=288, framerate=15/1 ! \
goosink_pp max-lateness=-1 >> /tmp/gst/gst_goo_wmv_decoder.log 2>&1

# WMV RCV QCIF - UYVY - DISPLAY
echo "* WMV decoder rcv qcif - uyvy - display "
echo "" >> /tmp/gst/gst_goo_wmv_decoder.log
echo "* WMV decoder rcv qcif - uyvy - display filename=SSL0018_QCIF.rcv" >> /tmp/gst/gst_goo_wmv_decoder.log
gst-launch filesrc location= /omx/patterns/SSL0018_QCIF.rcv ! \
rcvparse ! video/x-wmv,wmvversion=3,width=176,height=144,framerate=15/1 ! \
goodec_wmv process-mode=0 ! \
video/x-raw-yuv, format=\(fourcc\)UYVY, width=176, height=144, framerate=15/1 ! \
goosink_pp max-lateness=-1 >> /tmp/gst/gst_goo_wmv_decoder.log 2>&1

# WMV RCV CIF - UYVY - DISPLAY
echo "* WMV decoder rcv cif - uyvy - display "
echo "" >> /tmp/gst/gst_goo_wmv_decoder.log
echo "* WMV decoder rcv cif - uyvy - display filename=SSM0012_CIF.rcv" >> /tmp/gst/gst_goo_wmv_decoder.log
gst-launch filesrc location= /omx/patterns/SSM0012_CIF.rcv ! \
rcvparse ! video/x-wmv,wmvversion=3,width=352,height=288,framerate=15/1 ! \
goodec_wmv process-mode=0 ! \
video/x-raw-yuv, format=\(fourcc\)UYVY, width=352, height=288, framerate=15/1 ! \
goosink_pp max-lateness=-1 >> /tmp/gst/gst_goo_wmv_decoder.log 2>&1

# WMV RCV VGA - UYVY - DISPLAY
echo "* WMV decoder rcv vga - uyvy - display "
echo "" >> /tmp/gst/gst_goo_wmv_decoder.log
echo "* WMV decoder rcv vga - uyvy - display filename=Pinball_VGA_5mbps_MPML.rcv" >> /tmp/gst/gst_goo_wmv_decoder.log
gst-launch filesrc location= /omx/patterns/Pinball_VGA_5mbps_MPML.rcv ! \
rcvparse ! video/x-wmv,wmvversion=3,width=640,height=480,framerate=30/1 ! \
goodec_wmv process-mode=0 ! \
video/x-raw-yuv, format=\(fourcc\)UYVY, width=640, height=480, framerate=15/1 ! \
goosink_pp x-scale=50 y-scale=50 rotation=90 max-lateness=-1 >> /tmp/gst/gst_goo_wmv_decoder.log 2>&1

echo "Starting with WMV demuxing tests GStreamer Clock"

#WMV QVGA 30FPS WMA
echo "* WMV decoder QVGA - uyvy - display - wma "
echo "" >> /tmp/gst/gst_goo_wmv_decoder.log
echo "* WMV decoder QVGA - uyvy - display - wma - AV_000027_StarWarsTrailer_V_WMV9_MainP_768kbit_QVGA_30fps_A_WMA_48kHz_128kbps.wmv" >> /tmp/gst/gst_goo_wmv_decoder.log
gst-launch filesrc location="/multimedia/wmv/AV_000027_StarWarsTrailer_V_WMV9_MainP_768kbit_QVGA_30fps_A_WMA_48kHz_128kbps.wmv" ! \
asfdemux name=demux \
demux.video_00 ! queue ! goodec_wmv ! goosink_pp rotation=90 \
demux.audio_00 ! queue ! goodec_wma ! dasfsink >> /tmp/gst/gst_goo_wmv_decoder.log 2>&1

#WMV VGA 30FPS WMA
echo "* WMV decoder VGA - uyvy - display - wma "
echo "" >> /tmp/gst/gst_goo_wmv_decoder.log
echo "* WMV decoder VGA - uyvy - display - wma - AV_000028_StarWarsTrailer_V_WMV9_MainP_CBR_5Mbit_VGA_30fps_A_WMA_48kHz_128kbps.wmv" >> /tmp/gst/gst_goo_wmv_decoder.log
gst-launch filesrc location="/multimedia/wmv/AV_000028_StarWarsTrailer_V_WMV9_MainP_CBR_5Mbit_VGA_30fps_A_WMA_48kHz_128kbps.wmv" ! \
asfdemux name=demux \
demux.video_00 ! queue ! goodec_wmv ! goosink_pp rotation=90 x-scale=50 y-scale=50 \
demux.audio_00 ! queue ! goodec_wma ! dasfsink >> /tmp/gst/gst_goo_wmv_decoder.log 2>&1

#WMV QCIF 30FPS WMA
echo "* WMV decoder QCIF - uyvy - display - wma "
echo "" >> /tmp/gst/gst_goo_wmv_decoder.log
echo "* WMV decoder QCIF - uyvy - display - wma - AV_000128_WMV_QCIF_15fps_128Kbps_WMA_22KHz_32Kbps.wmv" >> /tmp/gst/gst_goo_wmv_decoder.log
gst-launch filesrc location="/multimedia/wmv/AV_000128_WMV_QCIF_15fps_128Kbps_WMA_22KHz_32Kbps.wmv" ! \
asfdemux name=demux \
demux.video_00 ! queue ! goodec_wmv ! goosink_pp  \
demux.audio_00 ! queue ! goodec_wma ! dasfsink >> /tmp/gst/gst_goo_wmv_decoder.log 2>&1

#WMV CIF 30FPS WMA
echo "* WMV decoder CIF - uyvy - display - wma "
echo "" >> /tmp/gst/gst_goo_wmv_decoder.log
echo "* WMV decoder CIF - uyvy - display - wma - rrrs.wmv" >> /tmp/gst/gst_goo_wmv_decoder.log
gst-launch filesrc location="/multimedia/wmv/rrrs.wmv" ! \
asfdemux name=demux \
demux.video_00 ! queue ! goodec_wmv ! goosink_pp  \
demux.audio_00 ! queue ! goodec_wma ! dasfsink >> /tmp/gst/gst_goo_wmv_decoder.log 2>&1

#WMV CIF 30FPS WMA
echo "* WMV decoder CIF - uyvy - display - wma "
echo "" >> /tmp/gst/gst_goo_wmv_decoder.log
echo "* WMV decoder CIF - uyvy - display - wma - rrr2.wmv" >> /tmp/gst/gst_goo_wmv_decoder.log
gst-launch filesrc location="/multimedia/wmv/rrr2.wmv" ! \
asfdemux name=demux \
demux.video_00 ! queue ! goodec_wmv ! goosink_pp  \
demux.audio_00 ! queue ! goodec_wma ! dasfsink >> /tmp/gst/gst_goo_wmv_decoder.log 2>&1

echo "Starting with WMV demuxing tests OMX Clock"

#WMV QVGA 30FPS WMA
echo "* WMV decoder QVGA - uyvy - display - wma "
echo "" >> /tmp/gst/gst_goo_wmv_decoder.log
echo "* WMV decoder QVGA - uyvy - display - wma - AV_000027_StarWarsTrailer_V_WMV9_MainP_768kbit_QVGA_30fps_A_WMA_48kHz_128kbps.wmv" >> /tmp/gst/gst_goo_wmv_decoder.log
gst-launch filesrc location="/multimedia/wmv/AV_000027_StarWarsTrailer_V_WMV9_MainP_768kbit_QVGA_30fps_A_WMA_48kHz_128kbps.wmv" ! \
asfdemux name=demux \
demux.video_00 ! queue ! goodec_wmv ! goosink_pp rotation=90 \
demux.audio_00 ! queue ! goodec_wma ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_wmv_decoder.log 2>&1

#WMV VGA 30FPS WMA
echo "* WMV decoder VGA - uyvy - display - wma "
echo "" >> /tmp/gst/gst_goo_wmv_decoder.log
echo "* WMV decoder VGA - uyvy - display - wma - AV_000028_StarWarsTrailer_V_WMV9_MainP_CBR_5Mbit_VGA_30fps_A_WMA_48kHz_128kbps.wmv" >> /tmp/gst/gst_goo_wmv_decoder.log
gst-launch filesrc location="/multimedia/wmv/AV_000028_StarWarsTrailer_V_WMV9_MainP_CBR_5Mbit_VGA_30fps_A_WMA_48kHz_128kbps.wmv" ! \
asfdemux name=demux \
demux.video_00 ! queue ! goodec_wmv ! goosink_pp rotation=90 x-scale=50 y-scale=50 \
demux.audio_00 ! queue ! goodec_wma ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_wmv_decoder.log 2>&1

#WMV QCIF 30FPS WMA
echo "* WMV decoder QCIF - uyvy - display - wma "
echo "" >> /tmp/gst/gst_goo_wmv_decoder.log
echo "* WMV decoder QCIF - uyvy - display - wma - AV_000128_WMV_QCIF_15fps_128Kbps_WMA_22KHz_32Kbps.wmv" >> /tmp/gst/gst_goo_wmv_decoder.log
gst-launch filesrc location="/multimedia/wmv/AV_000128_WMV_QCIF_15fps_128Kbps_WMA_22KHz_32Kbps.wmv" ! \
asfdemux name=demux \
demux.video_00 ! queue ! goodec_wmv ! goosink_pp  \
demux.audio_00 ! queue ! goodec_wma ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_wmv_decoder.log 2>&1

#WMV CIF 30FPS WMA
echo "* WMV decoder CIF - uyvy - display - wma "
echo "" >> /tmp/gst/gst_goo_wmv_decoder.log
echo "* WMV decoder CIF - uyvy - display - wma - rrrs.wmv" >> /tmp/gst/gst_goo_wmv_decoder.log
gst-launch filesrc location="/multimedia/wmv/rrrs.wmv" ! \
asfdemux name=demux \
demux.video_00 ! queue ! goodec_wmv ! goosink_pp  \
demux.audio_00 ! queue ! goodec_wma ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_wmv_decoder.log 2>&1

#WMV CIF 30FPS WMA
echo "* WMV decoder CIF - uyvy - display - wma "
echo "" >> /tmp/gst/gst_goo_wmv_decoder.log
echo "* WMV decoder CIF - uyvy - display - wma - rrr2.wmv" >> /tmp/gst/gst_goo_wmv_decoder.log
gst-launch filesrc location="/multimedia/wmv/rrr2.wmv" ! \
asfdemux name=demux \
demux.video_00 ! queue ! goodec_wmv ! goosink_pp  \
demux.audio_00 ! queue ! goodec_wma ! dasfsink clock-source=1 >> /tmp/gst/gst_goo_wmv_decoder.log 2>&1


