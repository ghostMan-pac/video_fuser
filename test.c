#include <gst/gst.h>

typedef struct element_list{
		GstElement *pipeline;
		GstElement *source;
		GstElement *filter;
		GstElement *sink;
}elements_t;

static void pad_added_handler(GstElement *src, GstPad *pad, elements_t* data);

int tutorial_main (int argc, char *argv[]){
		elements_t data;
		GstBus *bus;
		GstMessage *msg;
		GstStateChangeReturn ret;
		gboolean terminate = FALSE;

		/* Initialize GStreamer */
		gst_init (&argc, &argv);

		/* Create the elements */
		data.source = gst_element_factory_make ("v4l2src", "source");
		data.filter = gst_element_factory_make ("nvvidconv", "filter");
		data.sink = gst_element_factory_make ("autovideosink", "sink");

		/* Create the empty pipeline */
		data.pipeline = gst_pipeline_new ("test-pipeline");

		if (!data.pipeline || !data.source || !data.sink || !data.filter) {
				g_printerr ("Not all elements could be created.\n");
				return -1;
		}

		/* Build the pipeline */
		gst_bin_add_many (GST_BIN (data.pipeline), data.source, data.filter, data.sink, NULL);
		if(!gst_element_link_many(data.filter, data.sink, NULL)){
				g_printerr("Elements could not be linked. \n");
				gst_object_unref(data.pipeline);
				return -1 ;	
		}		

//		g_object_set (data.source, "pattern", 0, NULL);
		// v4l2src ! videoconvert ! 'video/x-raw,format=RGBA' ! nvvidconv ! 'video/x-raw(memory:NVMM),format=RGBA,width=1280,height=720' ! autovideosink
		// videotestsrc ! video/x-raw,format=YUY2,width=1280,height=720,framerate=30/1 ! nvvidconv ! 'video/x-raw(memory:NVMM),format=RGBA' ! autovideosink
		//create caps here.check gst inspect for available caps
		GstCaps *yuv_wh_caps = gst_caps_new_simple("video/x-raw",
						"format", G_TYPE_STRING, "YUY2",
						"width", G_TYPE_INT, 480,
						"height", G_TYPE_INT, 360,
						"framerate", GST_TYPE_FRACTION, 30, 1,
						NULL);

		GstCaps * rgba_nvmm_caps = gst_caps_new_simple("video/x-raw",
						"memory", G_TYPE_STRING, "NVMM",
						"format", G_TYPE_STRING, "RGBA",
						NULL);
		//create pads here
		GstPad *source_src_pad = gst_element_get_static_pad(data.source, "src");
		gst_pad_use_fixed_caps (source_src_pad);	

		if (!gst_pad_set_caps (source_src_pad, yuv_wh_caps)) {
				g_printerr("Failed to set caps  source src pad.\n");
				GST_ELEMENT_ERROR (data.source, CORE, NEGOTIATION, (NULL),
								("Some debug information here"));
				return GST_FLOW_ERROR;
		}	

		GstPad *filter_sink_pad = gst_element_get_static_pad(data.filter, "sink");

		gst_pad_use_fixed_caps (filter_sink_pad);	

		if (!gst_pad_set_caps (filter_sink_pad, yuv_wh_caps)) {
				GST_ELEMENT_ERROR (data.filter, CORE, NEGOTIATION, (NULL),
								("Some debug information here"));
				return GST_FLOW_ERROR;
		}	


		GstPad *filter_src_pad = gst_element_get_static_pad(data.filter, "src");

		gst_pad_use_fixed_caps (filter_src_pad);	

		if (!gst_pad_set_caps (filter_src_pad, rgba_nvmm_caps)) {
				GST_ELEMENT_ERROR (data.filter, CORE, NEGOTIATION, (NULL),
								("Some debug information here"));
				return GST_FLOW_ERROR;
		}	
		GstPad *sink_sink_pad = gst_element_get_static_pad(data.sink, "sink");
		gst_pad_use_fixed_caps (sink_sink_pad);	

		if (!gst_pad_set_caps (sink_sink_pad, rgba_nvmm_caps)) {
				GST_ELEMENT_ERROR (data.sink, CORE, NEGOTIATION, (NULL),
								("Some debug information here"));
				return GST_FLOW_ERROR;
		}	
		//connect the pads 


		if (gst_pad_link( source_src_pad, filter_sink_pad) != GST_PAD_LINK_OK) {
				g_printerr("Failed to link conv1_sink_pad to coi@mp_src0pad\n");
				return -1;
		}
		if (gst_pad_link( filter_src_pad, sink_sink_pad) != GST_PAD_LINK_OK) {
				g_printerr("Failed to link conv1_sink_pad to coi@mp_src0pad\n");
				return -1;
		}
		/* Start playing */
		ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
		if (ret == GST_STATE_CHANGE_FAILURE) {
				g_printerr ("Unable to set the pipeline to the playing state.\n");
				gst_object_unref (data.pipeline);
				return -1;
		}

		/* Listen to the bus */
		bus = gst_element_get_bus (data.pipeline);
		do {
				msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
								GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

				/* Parse message */
				if (msg != NULL) {
						GError *err;
						gchar *debug_info;

						switch (GST_MESSAGE_TYPE (msg)) {
								case GST_MESSAGE_ERROR:
										gst_message_parse_error (msg, &err, &debug_info);
										g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
										g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
										g_clear_error (&err);
										g_free (debug_info);
										terminate = TRUE;
										break;
								case GST_MESSAGE_EOS:
										g_print ("End-Of-Stream reached.\n");
										terminate = TRUE;
										break;
								case GST_MESSAGE_STATE_CHANGED:
										/* We are only interested in state-changed messages from the pipeline */
										if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data.pipeline)) {
												GstState old_state, new_state, pending_state;
												gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
												g_print ("Pipeline state changed from %s to %s:\n",
																gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
										}
										break;
								default:
										/* We should not reach here */
										g_printerr ("Unexpected message received.\n");
										break;
						}
						gst_message_unref (msg);
				}
		} while (!terminate);

		/* Free resources */
		gst_object_unref (bus);
		gst_element_set_state (data.pipeline, GST_STATE_NULL);
		gst_object_unref (data.pipeline);

		return 0;
}

int main (int argc, char *argv[])
{
		return tutorial_main (argc, argv);
}


static void pad_added_handler(GstElement *src, GstPad *new_pad, elements_t *data){
		g_printerr ("Entering the callback\n");

		GstPad *sink_pad = gst_element_get_static_pad(data->filter, "sink");
		GstPadLinkReturn ret;
		GstCaps *new_pad_caps = NULL;
		GstStructure *new_pad_struct = NULL;
		const gchar *new_pad_type = NULL;

		g_print("Received new pad %s  from %s : \n", GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(src));
		if(gst_pad_is_linked(sink_pad)){
				g_print("We are already linked. Ignoring\n");
				goto exit;
		}
		new_pad_caps =gst_pad_get_current_caps(new_pad);
		new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
		new_pad_type = gst_structure_get_name(new_pad_struct);
		//todo: continue writing the code from here. so far you have added the pad and now you are going to update the caps.
		ret = gst_pad_link(new_pad, sink_pad);
		if(GST_PAD_LINK_FAILED(ret)){
				g_print("Type is %s but link failed\n",new_pad_type);
		}else{
				g_print("link succeeded\n");
		}
exit :
		return ;
}
