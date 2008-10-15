#!/bin/sh

display -size 176x144   -sampling-factor 2x1 $H4/tmp/gst/camera_shot_qcif.yuv
display -size 352x288   -sampling-factor 2x1 $H4/tmp/gst/camera_shot_cif.yuv
display -size 320x240   -sampling-factor 2x1 $H4/tmp/gst/camera_shot_qvga.yuv
display -size 640x480   -sampling-factor 2x1 $H4/tmp/gst/camera_shot_vga.yuv
display -size 720x560   -sampling-factor 2x1 $H4/tmp/gst/camera_shot_720x560.yuv
display -size 800x600   -sampling-factor 2x1 $H4/tmp/gst/camera_shot_svga.yuv
display -size 1024x768  -sampling-factor 2x1 $H4/tmp/gst/camera_shot_xga.yuv
display -size 1280x1024 -sampling-factor 2x1 $H4/tmp/gst/camera_shot_sxga.yuv
display -size 1600x1200 -sampling-factor 2x1 $H4/tmp/gst/camera_shot_uxga.yuv

display -size 176x144   -sampling-factor 2x1 $H4/tmp/gst/camera_video_qcif.yuv
display -size 352x288   -sampling-factor 2x1 $H4/tmp/gst/camera_video_cif.yuv
display -size 320x240   -sampling-factor 2x1 $H4/tmp/gst/camera_video_qvga.yuv
display -size 640x480   -sampling-factor 2x1 $H4/tmp/gst/camera_video_vga.yuv


