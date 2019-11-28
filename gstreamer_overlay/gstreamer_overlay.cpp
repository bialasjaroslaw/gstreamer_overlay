// comment to use pthreads.h
#define G_OS_WIN32

#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <windows.h>

GstElement* pipeline;
GstElement* src;
GstElement* queue_vid;
GstElement* queue_aud;
GstElement* mkv_demux;
GstElement* vp8dec;
GstElement* vorbis_dec;
GstElement* sink;
GstElement* audio_conv;
GstElement* audio_sink;

static void cb_new_pad(GstElement* element, GstPad* pad, gpointer data)
{
    gchar* name = gst_pad_get_name(pad);
	// React only when video is added 
    if (strcmp(name, "video_0") == 0)
    {
        auto b1 = gst_element_link_pads(mkv_demux, name, vp8dec, "sink");
        auto b2 = gst_element_link(vp8dec, queue_vid);
        auto b3 = gst_element_link(queue_vid, sink);
        if (!b1 || !b2 || !b3)
            g_print("Linking video failed\n");
    }
	// React only when audio is added 
	if (strcmp(name, "audio_0") == 0)
	{
		auto b1 = gst_element_link_pads(mkv_demux, name, vorbis_dec, "sink");
		auto b2 = gst_element_link(vorbis_dec, queue_aud);
		auto b3 = gst_element_link(queue_aud, audio_conv);
		auto b4 = gst_element_link(audio_conv, audio_sink);
		if (!b1 || !b2 || !b3 || !b4)
			g_print("Linking audio failed\n");
	}
    g_free(name);
}

static gboolean bus_call(GstBus* bus, GstMessage* msg, gpointer data)
{
    GMainLoop* loop = (GMainLoop*)data;

    switch (GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(loop);
            break;

        case GST_MESSAGE_ERROR: {
            gchar* debug;
            GError* error;

            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);

            g_printerr("Error: %s\n", error->message);
            g_error_free(error);

            g_main_loop_quit(loop);
            break;
        }
        default:
            break;
    }

    return TRUE;
}

int main(int argc, char* argv[])
{
	gst_init(&argc, &argv);
	// Event loop in case you dont have anything else that keeps your app running
	GMainLoop* loop = g_main_loop_new(NULL, FALSE);
	HWND id = ::CreateWindowW(L"STATIC", L"dummy", WS_VISIBLE, 0, 0, 640, 480, NULL, NULL, NULL, NULL);

	// Create elements
	pipeline = gst_pipeline_new("xvoverlay");
    src = gst_element_factory_make("souphttpsrc", NULL);
	queue_vid = gst_element_factory_make("queue", NULL);
	queue_aud = gst_element_factory_make("queue", NULL);
    mkv_demux = gst_element_factory_make("matroskademux", NULL);
    vp8dec = gst_element_factory_make("vp8dec", NULL);
	vorbis_dec = gst_element_factory_make("vorbisdec", NULL);
    sink = gst_element_factory_make("d3dvideosink", NULL);
	audio_conv = gst_element_factory_make("audioconvert", NULL);
	audio_sink = gst_element_factory_make("directsoundsink", NULL);

	// Diagnostic
	if (!pipeline || !src || !queue_vid || !queue_aud || !mkv_demux || !vp8dec || !vorbis_dec || !sink || !audio_sink || !audio_conv)
		g_error("Couldn't create all elements");

	// Set source
	g_object_set(G_OBJECT(src), "location", "http://dl5.webmfiles.org/big-buck-bunny_trailer.webm", NULL);

	// This is needed if you want to quit loop in case of EOS event
	GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

	// Add elements and link first two (others will be linked after receiving information about video type)
    gst_bin_add_many(GST_BIN(pipeline), src, mkv_demux, vp8dec, vorbis_dec, queue_vid, queue_aud, sink, audio_sink, audio_conv, NULL);
    gst_element_link(src, mkv_demux);

	// Connect to event that will happen when demuxer will recognize video file
    g_signal_connect(mkv_demux, "pad-added", G_CALLBACK(cb_new_pad), NULL);
	gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(sink), (guintptr)id);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

	// Loop
    g_print("Running...\n");
    g_main_loop_run(loop);

    g_print("Returned, stopping playback\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}

