// comment to use pthreads.h
#define G_OS_WIN32

#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <windows.h>

int main(int argc, char* argv[])
{
	gst_init(&argc, &argv);
	GMainLoop* loop = g_main_loop_new(NULL, FALSE);
	HWND id = ::CreateWindowW(L"STATIC", L"dummy", WS_VISIBLE, 0, 0, 640, 480, NULL, NULL, NULL, NULL);

	GstElement* pipeline = gst_pipeline_new("xvoverlay");
	GstElement* src = gst_element_factory_make("videotestsrc", NULL);
	GstElement* sink = gst_element_factory_make("d3dvideosink", NULL);

	gst_bin_add_many(GST_BIN(pipeline), src, sink, NULL);
	gst_element_link(src, sink);

	gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(sink), (guintptr)id);

	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	// replace by anything that keeps your app running
	g_main_loop_run(loop);
	// This is very naive but working replacement
	//while (true);
}

