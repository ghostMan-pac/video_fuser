# video_fuser
The application will be able to use two webcam streams overlayed on top of each other using gstreamer

This application is intended for testing out the capability of the nvidia agx and nano development boards when handling two streams together.

`gst-launch-1.0 \
nvcompositor name=comp \
sink_0::xpos=0 sink_0::ypos=0 sink_0::width=1280 sink_0::height=720 sink_0::zorder=1 sink_0::alpha=1 \
sink_1::xpos=0 sink_1::ypos=0 sink_1::width=1280 sink_1::height=720 sink_1::zorder=2 sink_1::alpha=0.5 \
! 'video/x-raw(memory:NVMM),format=RGBA' ! nvvidconv ! video/x-raw ! xvimagesink \
videotestsrc ! video/x-raw,format=YUY2,width=1280,height=720,framerate=30/1 ! nvvidconv ! 'video/x-raw(memory:NVMM),format=RGBA' ! comp.sink_0 \
v4l2src ! videoconvert ! video/x-raw,format=RGBA ! nvvidconv ! 'video/x-raw(memory:NVMM),format=RGBA,width=1280,height=720' ! comp.sink_1`
