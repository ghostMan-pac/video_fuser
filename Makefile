#all:compositor video_receiver 3video_receiver
all:
	gcc -o vfus video_fuser.c `pkg-config --cflags --libs gstreamer-1.0`
	gcc -c sem_util.c
	gcc -o vrec video_receiver.c sem_util.o `pkg-config --cflags --libs gstreamer-1.0`
	echo "Creating vrec app using gcc"
compositor:
	gcc -o comp 4compositor.c `pkg-config --cflags --libs gstreamer-1.0`
video_receiver:
	gcc -o fin_vrec fin_video_receiver.c `pkg-config --cflags --libs gstreamer-1.0`
3video_receiver:
	gcc -o vrec 3video_receiver.c `pkg-config --cflags --libs gstreamer-1.0`
clean: 
	echo "Cleaning remnant vrec"
	rm vrec

