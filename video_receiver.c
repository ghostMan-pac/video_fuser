#include <gst/gst.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include "sem_util.h"

#define WIDTH 480
#define HEIGHT 270
#define DEPTH 3 // Assuming RGB color channels
char *shared_memory = NULL;
int shmid;
int sem_id = 0;

int sem_init()
{
	sem_id = Sem_init(3104);
	if (sem_id <0)
	{
		g_printf("sem id get failed\n");
	}
	return sem_id;
}

void sigint_handler(int signal)
{
	if (signal == SIGINT)
		g_printf("\nIntercepted SIGINT!\n");
	shmdt(shared_memory);
	shmctl(shmid, IPC_RMID, NULL);
	Sem_delete(sem_id);
	exit(1);
}

void set_signal_action(void)
{
	// Declare the sigaction structure
	struct sigaction act;

	// Set all of the structure's bits to 0 to avoid errors
	// relating to uninitialized variables...
	bzero(&act, sizeof(act));
	// Set the signal handler as the default action
	act.sa_handler = &sigint_handler;
	// Apply the action in the structure to the
	// SIGINT signal (ctrl-c)
	sigaction(SIGINT, &act, NULL);
}

int shmem_init()
{
	key_t key = 123456;

	size_t size = 4915200;
	g_printf("Initializing SHmem with key %d\n", key);
	if ((shmid = shmget(key, size, IPC_CREAT | 0666)) < 0)
	{
		g_printf("Error getting shared memory id\n");
		exit(1);
	}
	// Attached shared memory
	if ((shared_memory = (char *)shmat(shmid, NULL, 0)) == (char *)-1)
	{
		g_printf("Error attaching shared memory id\n");
		exit(1);
	}
	g_printf("Shmem initialisation success with size %d\n", size);
	return 0;
}

static GstPadProbeReturn cb_have_data(GstPad *pad,
									  GstPadProbeInfo *info,
									  gpointer user_data)
{

	GstMapInfo map;
	GstBuffer *buffer;
	g_printf("entered the call back for capturing th eimage buffer\n");
	buffer = GST_PAD_PROBE_INFO_BUFFER(info);
	//	buffer = gst_buffer_make_writable(buffer);

	/* Making a buffer writable can fail (for example if it
	 * cannot be copied and is used more than once)
	 */
	if (buffer == NULL)
		return GST_PAD_PROBE_OK;

	/* Mapping a buffer can fail (non-writable) */
	if (gst_buffer_map(buffer, &map, GST_MAP_READ))
	{

		Sem_gain(sem_id);
		memcpy(shared_memory, map.data, gst_buffer_get_size(buffer));
		Sem_release(sem_id);

		g_printf("sizeof %d \n", gst_buffer_get_size(buffer));
		gst_buffer_unmap(buffer, &map);
	}

	//	GST_PAD_PROBE_INFO_DATA(info) = buffer;

	return GST_PAD_PROBE_OK;
}

int main(int argc, char *argv[])
{
	GstElement *pipeline;
	GstBus *bus;
	GstStateChangeReturn ret;
	GstMessage *msg;
	GError *error = NULL;
	sem_init();
	shmem_init();
	/* Initialize GStreamer */
	gst_init(&argc, &argv);
	set_signal_action();
	/* Create the pipeline */
	pipeline = gst_parse_launch("nvcompositor name=comp \
			sink_0::xpos=0 sink_0::ypos=0 sink_0::width=480 sink_0::height=270 sink_0::zorder=1 sink_0::alpha=1 \
			sink_1::xpos=0 sink_1::ypos=0 sink_1::width=480 sink_1::height=270 sink_1::zorder=2 sink_1::alpha=0.5 \
			! video/x-raw\(memory:NVMM\),format=RGBA ! nvvidconv ! video/x-raw ! xvimagesink  \
			v4l2src device=\"/dev/video0\" name=source1 ! videoconvert name=vidconv1 ! video/x-raw,format=RGBA ! nvvidconv ! video/x-raw\(memory:NVMM\),format=RGBA,width=480,height=270 ! queue !  comp.sink_0 \
			v4l2src device=\"/dev/video2\" name=source2 ! videoconvert ! video/x-raw,format=RGBA ! nvvidconv ! video/x-raw\(memory:NVMM\),format=RGBA,width=480,height=270 ! queue !  comp.sink_1",
								&error);

	if (pipeline == NULL)
	{
		g_printerr("Parse error: %s\n", error->message);
		g_error_free(error);
		return -1;
	}
#ifdef USE_SRC_PAD_OF_v4L2SRC
	GstElement *video_source = gst_bin_get_by_name(GST_BIN(pipeline), "source1");

	GstPad *v4l2_src_pad = gst_element_get_static_pad(video_source, "src");
	gst_pad_add_probe(v4l2_src_pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)cb_have_data, NULL, NULL);
	gst_object_unref(v4l2_src_pad);
#endif

	GstElement *video_source = gst_bin_get_by_name(GST_BIN(pipeline), "vidconv1");

	GstPad *v4l2_src_pad = gst_element_get_static_pad(video_source, "src");
	gst_pad_add_probe(v4l2_src_pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)cb_have_data, NULL, NULL);
	gst_object_unref(v4l2_src_pad);
	/* Set the pipeline state to playing */
	ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE)
	{
		g_printerr("Unable to set the pipeline to the playing state.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	/* Wait until error or EOS */
	bus = gst_element_get_bus(pipeline);
	msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

	/* Parse message */
	if (msg != NULL)
	{
		GError *err;
		gchar *debug_info;

		switch (GST_MESSAGE_TYPE(msg))
		{
		case GST_MESSAGE_ERROR:
			gst_message_parse_error(msg, &err, &debug_info);
			g_printerr("Error received from element %s: %s\n",
					   GST_OBJECT_NAME(msg->src), err->message);
			g_printerr("Debugging information: %s\n",
					   debug_info ? debug_info : "none");
			g_clear_error(&err);
			g_free(debug_info);
			break;
		case GST_MESSAGE_EOS:
			g_print("End-Of-Stream reached.\n");
			break;
		default:
			/* We should not reach here because we only asked for ERRORs and EOS */
			g_printerr("Unexpected message received.\n");
			break;
		}
		gst_message_unref(msg);
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
	shmdt(shared_memory);
	shmctl(shmid, IPC_RMID, NULL);
	return 0;
}
