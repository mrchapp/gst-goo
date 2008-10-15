#!/bin/sh

echo test1_WMA_v7_32kbps_22kHz_2.rca
gst-launch filesrc location=/omx/patterns/test1_WMA_v7_32kbps_22kHz_2.rca ! goodec_wma wmatxtfile=/omx/patterns/test1_WMA_v7_32kbps_22kHz_2.rca.txt ! dasfsink

echo test1_WMA_v7_32kbps_44kHz_1.rca
gst-launch filesrc location=/omx/patterns/test1_WMA_v7_32kbps_44kHz_1.rca ! goodec_wma wmatxtfile=/omx/patterns/test1_WMA_v7_32kbps_44kHz_1.rca.txt ! dasfsink

echo test1_WMA_v7_5kbps_8kHz_1.rca
gst-launch filesrc location=/omx/patterns/test1_WMA_v7_5kbps_8kHz_1.rca ! goodec_wma wmatxtfile=/omx/patterns/test1_WMA_v7_5kbps_8kHz_1.rca.txt ! dasfsink

echo test1_WMA_v8_48kbps_44kHz_2_WMP.rca
gst-launch filesrc location=/omx/patterns/test1_WMA_v8_48kbps_44kHz_2_WMP.rca ! goodec_wma wmatxtfile=/omx/patterns/test1_WMA_v8_48kbps_44kHz_2_WMP.rca.txt ! dasfsink

echo test1_WMA_v9_1pCBR_31kbps_8kHz_1.rca
gst-launch filesrc location=/omx/patterns/test1_WMA_v9_1pCBR_31kbps_8kHz_1.rca ! goodec_wma wmatxtfile=/omx/patterns/test1_WMA_v9_1pCBR_31kbps_8kHz_1.rca.txt ! dasfsink

echo test1_WMA_v9_1pCBR_48kbps_48kHz_2.rca
gst-launch filesrc location=/omx/patterns/test1_WMA_v9_1pCBR_48kbps_48kHz_2.rca ! goodec_wma wmatxtfile=/omx/patterns/test1_WMA_v9_1pCBR_48kbps_48kHz_2.rca.txt ! dasfsink

echo test1_wma_v8_5kbps_8khz_1.rca
gst-launch filesrc location=/omx/patterns/test1_wma_v8_5kbps_8khz_1.rca ! goodec_wma wmatxtfile=/omx/patterns/test1_wma_v8_5kbps_8khz_1.rca.txt ! dasfsink

echo test2_WMA_v8_32kbps_44kHz_2.rca
gst-launch filesrc location=/omx/patterns/test2_WMA_v8_32kbps_44kHz_2.rca ! goodec_wma wmatxtfile=/omx/patterns/test2_WMA_v8_32kbps_44kHz_2.rca.txt ! dasfsink

echo test2_WMA_v9_1pCBR_128kbps_44kHz_2_NC.rca
gst-launch filesrc location=/omx/patterns/test2_WMA_v9_1pCBR_128kbps_44kHz_2_NC.rca ! goodec_wma wmatxtfile=/omx/patterns/test2_WMA_v9_1pCBR_128kbps_44kHz_2_NC.rca.txt ! dasfsink

echo test2_WMA_v9_2pVBR_Bitrate_192kbps_48kHz_2_NC.rca
gst-launch filesrc location=/omx/patterns/test2_WMA_v9_2pVBR_Bitrate_192kbps_48kHz_2_NC.rca ! goodec_wma wmatxtfile=/omx/patterns/test2_WMA_v9_2pVBR_Bitrate_192kbps_48kHz_2_NC.rca.txt ! dasfsink

