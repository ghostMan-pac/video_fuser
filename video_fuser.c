#include <gst/gst.h>

int main(int argc, char *argv[]) {
		GstBus *bus;
		GstMessage *msg;
		GstStateChangeReturn ret;
		GstElement *pipeline ,*comp,*video_test_src, *v4l2_src, *nvvidconv1, *nvvidconv2, *nvvidconv3, *video_convert, *main_sink;

		/* Initialize GStreamer */
		gst_init(&argc, &argv);

		/* Create the elements */
		pipeline = gst_pipeline_new("video-pipeline");
		comp = gst_element_factory_make("nvcompositor", "comp");
		video_test_src = gst_element_factory_make("videotestsrc", "video_test_src");
		v4l2_src = gst_element_factory_make("v4l2src", "v4l2_src");
		nvvidconv1 = gst_element_factory_make("nvvidconv", "nvvidconv1");
		nvvidconv2 = gst_element_factory_make("nvvidconv", "nvvidconv2");
		nvvidconv3 = gst_element_factory_make("nvvidconv", "nvvidconv3");
		video_convert = gst_element_factory_make("videoconvert", "video_convert"); // Convert to non-NVMM format
		main_sink = gst_element_factory_make("nv3dsink", "main_sink");

		/* Check elements creation */
		if (!pipeline || !comp || !video_test_src || !v4l2_src || !nvvidconv1 || !nvvidconv2 || !nvvidconv3 || !video_convert || !main_sink) {
				g_printerr("One or more elements could not be created. Exiting.\n");
				return -1;
		}

		GstCaps * rgba_nvmm_caps_1 = gst_caps_new_simple("video/x-raw",
						"memory", G_TYPE_STRING, "NVMM",
						"format", G_TYPE_STRING, "RGBA",
						NULL);
		GstCaps *rgba_caps_3 = gst_caps_new_simple("video/x-raw",
						"format", G_TYPE_STRING, "RGBA",
						NULL);
		GstCaps *rgba_wh_caps_4 = gst_caps_new_simple("video/x-raw",
						"format", G_TYPE_STRING, "RGBA",
						"width", G_TYPE_INT, 480,
						"height", G_TYPE_INT, 360,
						NULL);
		/* Set properties for the compositor sink_0 */
		GstPad *comp_src_0_pad = gst_element_get_static_pad(comp, "src");

		GstPad *comp_sink_0_pad = gst_element_get_request_pad(comp, "sink_0");
		GstPad *comp_sink_1_pad = gst_element_get_request_pad(comp, "sink_1");

		GstPad *v4l2src_src_pad =  gst_element_get_static_pad(v4l2_src, "src");
	
		GstPad *videotestsrc_src_pad =  gst_element_get_static_pad(video_test_src, "src");
		
		GstPad *conv1_sink_pad = gst_element_get_static_pad(nvvidconv1, "sink");
		GstPad *conv1_src_pad =  gst_element_get_static_pad(nvvidconv1, "src");
//		gst_pad_use_fixed_caps(conv1_sink_pad);
	
		GstCaps *yuv_wh_caps_2 = gst_caps_new_simple("video/x-raw",
						"format", G_TYPE_STRING, "YUY2",
						"width", G_TYPE_INT, 480,
						"height", G_TYPE_INT, 360,
						"framerate", GST_TYPE_FRACTION, 30, 1,
						NULL);
		GstPad *conv2_sink_pad = gst_element_get_static_pad(nvvidconv2, "sink");
		GstPad *conv2_src_pad =  gst_element_get_static_pad(nvvidconv2, "src");
		
		GstPad *conv3_sink_pad = gst_element_get_static_pad(nvvidconv3, "sink");
		GstPad *conv3_src_pad =  gst_element_get_static_pad(nvvidconv3, "src");

		
		GstPad *video_convert_sink_pad = gst_element_get_static_pad(video_convert, "sink");
		GstPad *video_convert_src_pad =  gst_element_get_static_pad(video_convert, "src");
		
		GstPad *main_sink_sink_pad = gst_element_get_static_pad(main_sink, "sink");
		
		g_object_set(G_OBJECT(comp_sink_0_pad),
						"xpos", 0,
						"ypos", 0,
						"width", 480,
						"height", 360,
						"zorder", 1,
						"alpha", 1.0,
						NULL);

		/* Set properties for the compositor sink_1 */
		g_object_set(G_OBJECT(comp_sink_1_pad),
						"xpos", 0,
						"ypos", 0,
						"width", 480,
						"height", 360,
						"zorder", 2,
						"alpha", 0.5,
						NULL);


		/* Set properties for the source */
		g_object_set(G_OBJECT(video_test_src),
						"pattern", 0, // 0 is the default pattern (smpte)
						NULL);
		/* Add elements to the pipeline */
		gst_bin_add_many(GST_BIN(pipeline), comp, video_test_src, v4l2_src, nvvidconv1, nvvidconv2, nvvidconv3, video_convert, main_sink, NULL);

#if 1
		if (!gst_pad_set_caps(conv1_sink_pad, yuv_wh_caps_2)) {
				g_printerr("Failed to set caps on111221 videotestsrc pad.\n");
				return -1;
		}
#endif
		/* videotestsrc pipeline */
		if (gst_pad_link( videotestsrc_src_pad, conv1_sink_pad) != GST_PAD_LINK_OK) {
				g_printerr("Failed to link conv1_sink_pad to coi@mp_src0pad\n");
				return -1;
		}
		if (gst_pad_link( conv1_src_pad, comp_sink_0_pad) != GST_PAD_LINK_OK) {
				g_printerr("Failed to link conv1_sink_pad to comp_src0pad\n");
				return -1;
		}
//		gst_object_unref(conv1_src_pad);

		/* Set the caps on conv1_pad and comp_src0pad*/
#ifdef TO_REINSTATE_aFTER_VERIFICATION
		if (!gst_pad_set_caps(conv1_src_pad, rgba_nvmm_caps_1)) {
				g_printerr("Failed to set caps on conv1 pad.\n");
				return -1;
		}
		if (!gst_pad_set_caps(comp_sink_0_pad, rgba_nvmm_caps_1)) {
				g_printerr("Failed to set caps on sink-0comp pad.\n");
				return -1;
		}
#endif

//		gst_object_unref(comp_sink_0_pad);
		//2nd pipeline 
		if (gst_pad_link( v4l2src_src_pad, video_convert_sink_pad) != GST_PAD_LINK_OK) {
				g_printerr("2Failed to link sink_0 pad of comp to conv1.\n");
				return -1;
		}
		
		if (gst_pad_link( video_convert_src_pad, conv2_sink_pad) != GST_PAD_LINK_OK) {
				g_printerr("1Failed to link sink_0 pad of comp to conv1.\n");
				return -1;
		}


		if (gst_pad_link( conv2_src_pad, comp_sink_1_pad) != GST_PAD_LINK_OK) {
				g_printerr("33Failed to link sink_0 pad of comp to conv1.\n");
				return -1;
		}
#if 0
		/* Set the caps on conv1_pad and comp_src0pad*/
		if (!gst_pad_set_caps(conv2_src_pad, rgba_wh_caps_4)) {
				g_printerr("Failed to set caps on conv1 pad.\n");
				return -1;
		}
		if (!gst_pad_set_caps(comp_sink_1_pad, rgba_wh_caps_4)) {
				g_printerr("Failed to set caps on conv1 pad.\n");
				return -1;
		}
#endif	
//		gst_object_unref(comp_sink_1_pad);
		//main pipeline
		//here we join the two sources to compositor.

		if (gst_pad_link( comp_src_0_pad, conv3_sink_pad) != GST_PAD_LINK_OK) {
				g_printerr("55Failed to link sink_0 pad of comp to conv1.\n");
				return -1;
		}

		if (gst_pad_link( conv3_src_pad, main_sink_sink_pad) != GST_PAD_LINK_OK) {
				g_printerr("Failed to link sink_0 pad of comp to conv1.\n");
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
		gst_caps_unref(rgba_nvmm_caps_1);
		gst_caps_unref(yuv_wh_caps_2);
		gst_caps_unref(rgba_caps_3);
		gst_caps_unref(rgba_wh_caps_4);

		gst_object_unref(comp_src_0_pad);
		gst_object_unref(conv1_sink_pad);
//		gst_object_unref(conv2_sink_pad);

#if 0
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

