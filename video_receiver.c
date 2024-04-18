#include <gst/gst.h>

int main(int argc, char *argv[]) {
		GstElement *pipeline;
		GstBus *bus;
		GstStateChangeReturn ret;
		GstMessage *msg;
		GError *error = NULL;

		/* Initialize GStreamer */
		gst_init(&argc, &argv);

		/* Create the pipeline */
		pipeline = gst_parse_launch("nvcompositor name=comp \
						sink_0::xpos=0 sink_0::ypos=0 sink_0::width=480 sink_0::height=360 sink_0::zorder=1 sink_0::alpha=1 \
						sink_1::xpos=0 sink_1::ypos=0 sink_1::width=480 sink_1::height=360 sink_1::zorder=2 sink_1::alpha=0.5 \
						! video/x-raw\(memory:NVMM\),format=RGBA ! nvvidconv ! video/x-raw ! nv3dsink  \
						videotestsrc ! video/x-raw,format=YUY2,width=480,height=360,framerate=30/1 ! nvvidconv ! video/x-raw\(memory:NVMM\),format=RGBA ! queue ! comp.sink_0 \
						v4l2src name=source1 ! videoconvert ! video/x-raw,format=RGBA ! nvvidconv ! video/x-raw\(memory:NVMM\),format=RGBA,width=480,height=360 ! queue !  comp.sink_1", &error);

		if (pipeline == NULL) {
				g_printerr("Parse error: %s\n", error->message);
				g_error_free(error);
				return -1;
		}

		/* Set the pipeline state to playing */
		ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
		if (ret == GST_STATE_CHANGE_FAILURE) {
				g_printerr ("Unable to set the pipeline to the playing state.\n");
				gst_object_unref (pipeline);
				return -1;
		}
		/* Wait until error or EOS */
		bus = gst_element_get_bus(pipeline);
		msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

		/* Parse message */
		if (msg != NULL) {
				GError *err;
				gchar *debug_info;

				switch (GST_MESSAGE_TYPE (msg)) {
						case GST_MESSAGE_ERROR:
								gst_message_parse_error (msg, &err, &debug_info);
								g_printerr ("Error received from element %s: %s\n",
												GST_OBJECT_NAME (msg->src), err->message);
								g_printerr ("Debugging information: %s\n",
												debug_info ? debug_info : "none");
								g_clear_error (&err);
								g_free (debug_info);
								break;
						case GST_MESSAGE_EOS:
								g_print ("End-Of-Stream reached.\n");
								break;
						default:
								/* We should not reach here because we only asked for ERRORs and EOS */
								g_printerr ("Unexpected message received.\n");
								break;
				}
				gst_message_unref (msg);
		}
		/* Free resources */
		//		gst_object_unref(conv2_sink_pad);
#if 0
		gst_object_unref(comp_src_0_pad);
		gst_object_unref(conv1_sink_pad);

		/* Release elements */
		gst_object_unref(main_sink);
		gst_object_unref(video_convert);
		gst_object_unref(nvvidconv3);
		gst_object_unref(nvvidconv2);
		gst_object_unref(nvvidconv1);
		gst_object_unref(v4l2_src);
		gst_object_unref(video_test_src);
		gst_object_unref(comp);
		gst_object_unref(pipeline);
#endif
		gst_object_unref(bus);
		gst_element_set_state(pipeline, GST_STATE_NULL);
		return 0;
}
