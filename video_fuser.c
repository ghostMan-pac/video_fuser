#include <gst/gst.h>

int main(int argc, char *argv[]) {
		GstElement *pipeline, *src, *src2, *conv1, *conv2, *sink, *comp;
		GstCaps *caps_1, *caps_2, *caps_3 , *caps_4;
		GstBus *bus;
		GstMessage *msg;
		GstStateChangeReturn ret;
		GstPad *conv1_sink_pad, *conv1_src_pad;
		GstElement *pipeline, *src, *src2, *conv1, *conv2, *sink, *comp;

		/* Initialize GStreamer */
		gst_init(&argc, &argv);

		/* Create the elements */
		pipeline = gst_pipeline_new("video-pipeline");
		comp = gst_element_factory_make("nvcompositor", "comp");
		video_test_src = gst_element_factory_make("videotestsrc", "src");
		v4l2_src = gst_element_factory_make("v4l2src", "src2");
		nvvidconv1 = gst_element_factory_make("nvvidconv", "nvvidconv1");
		nvvidconv2 = gst_element_factory_make("nvvidconv", "nvvidconv2");
		nvvidconv3 = gst_element_factory_make("nvvidconv", "nvvidconv3");
		video_convert = gst_element_factory_make("videoconvert", "video_convert"); // Convert to non-NVMM format
		sink = gst_element_factory_make("nv3dsink", "sink");

		/* Check elements creation */
		if (!pipeline || !comp || !src || !src2 || !conv1 || !conv2 || !sink) {
				g_printerr("One or more elements could not be created. Exiting.\n");
				return -1;
		}

		/* Set properties for the compositor sink_0 */
		GstPad *sink_0_pad = gst_element_get_request_pad(comp, "sink_0");
		g_object_set(G_OBJECT(sink_0_pad),
						"xpos", 0,
						"ypos", 0,
						"width", 480,
						"height", 360,
						"zorder", 1,
						"alpha", 1.0,
						NULL);

		/* Set properties for the compositor sink_1 */
		GstPad *sink_1_pad = gst_element_get_request_pad(comp, "sink_1");
		g_object_set(G_OBJECT(sink_1_pad),
						"xpos", 0,
						"ypos", 0,
						"width", 480,
						"height", 360,
						"zorder", 2,
						"alpha", 0.5,
						NULL);

		caps_1 = gst_caps_new_simple("video/x-raw",
						"memory", G_TYPE_STRING, "NVMM",
						"format", G_TYPE_STRING, "RGBA",
						NULL);
		caps_2 = gst_caps_new_simple("video/x-raw",
						"format", G_TYPE_STRING, "YUY2",
						"width", G_TYPE_INT, 480,
						"height", G_TYPE_INT, 360,
						"framerate", GST_TYPE_FRACTION, 30, 1,
						NULL);
//		caps_3 = gst_caps_new_simple("video/x-raw",
//						NULL);
		caps_4 = gst_caps_new_simple("video/x-raw",
						"format", G_TYPE_STRING, "RGBA",
						"width", G_TYPE_INT, 480,
						"height", G_TYPE_INT, 360,
						NULL);

		/* Set properties for the source */
		g_object_set(G_OBJECT(src),
						"pattern", 0, // 0 is the default pattern (smpte)
						NULL);

		/* Add elements to the pipeline */
		gst_bin_add_many(GST_BIN(pipeline), comp, src, src2, conv1, conv2, sink, NULL);

		/* Link the elements with appropriate caps */
		if (!gst_element_link_filtered(src, conv1, caps_2)) {
				g_printerr("Failed to link videotestsrc to nvvidconv.\n");
				return -1;
		}

		if (!gst_element_link_filtered(src2, conv2, NULL)) {
				g_printerr("Failed to link v4l2src to videoconverter.\n");
				return -1;
		}
		if (!gst_element_link_filtered(conv2, conv1, NULL)) {
				g_printerr("Failed to link videoconverter to nvvidconv.\n");
				return -1;
		}

		if (!gst_element_link_filtered( conv1,sink,NULL)) {
				g_printerr("Failed to link comp to xvimagesink.\n");
				return -1;
		}
		
	//link the sink pad of the comp to the source pad of conv1
	if (!gst_element_link_filtered(comp, conv1, caps_1)) {
				g_printerr("Failed to link comp to xvimagesink.\n");
				return -1;
		}
#if 0
		if (gst_pad_link_filtered(conv1_pad,sink_0_pad, caps_1)) {        
				g_printerr("Failed to link sink_0 pad of comp to conv1.\n");
				return -1;
		}
		if (gst_pad_link_filtered(conv1_pad,sink_1_pad, caps_4)) {        
				g_printerr("Failed to link sink_0 pad of comp to conv1.\n");
				return -1;
		}
#endif
		/* Connect sink_0 pad of comp to conv1 with caps_1 */
		conv1_src_pad = gst_element_get_static_pad(conv1, "src");
		conv1_sink_pad = gst_element_get_static_pad(conv1, "sink");
		if (gst_pad_link( conv1_sink_pad, sink_0_pad) != GST_PAD_LINK_OK) {
				g_printerr("Failed to link sink_0 pad of comp to conv1.\n");
				return -1;
		}

		/* Set the caps on conv1_pad */
		if (!gst_pad_set_caps(conv1_pad, caps_1)) {
				g_printerr("Failed to set caps on conv1 pad.\n");
				return -1;
		}
		
		if (gst_pad_link( conv1_pad, sink_1_pad) != GST_PAD_LINK_OK) {
				g_printerr("Failed to link sink_0 pad of comp to conv1.\n");
				return -1;
		}

		/* Set the caps on conv1_pad */
		if (!gst_pad_set_caps(conv1_pad, caps_4)) {
				g_printerr("Failed to set caps on conv1 pad.\n");
				return -1;
		}

		if (gst_pad_link(sink_0_pad, conv1_src_pad) != GST_PAD_LINK_OK) {
				g_printerr("Failed to link sink_0 pad of comp  src padconv1.\n");
				return -1;
		}

		/* Set the caps on conv1_pad */
		if (!gst_pad_set_caps(conv1_src_pad, caps_1)) {
				g_printerr("Failed to set caps on conv1 pad.\n");
				return -1;
		}
		
		if (gst_pad_link( conv1_pad, sink_1_pad) != GST_PAD_LINK_OK) {
				g_printerr("Failed to link sink_0 pad of comp to conv1.\n");
				return -1;
		}

		/* Set the caps on conv1_pad */
		if (!gst_pad_set_caps(conv1_pad, caps_4)) {
				g_printerr("Failed to set caps on conv1 pad.\n");
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
		gst_caps_unref(caps_1);
		gst_caps_unref(caps_2);

		gst_object_unref(bus);
		gst_element_set_state(pipeline, GST_STATE_NULL);
		gst_object_unref(pipeline);
		return 0;
}

