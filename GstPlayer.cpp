#include "GstPlayer.hpp"

#include "App.hpp"
#include "Song.hpp"

#include <QUrl>

namespace quince {

static gboolean bus_callback(GstBus *bus, GstMessage *msg, gpointer data)
{
	quince::App *app = (quince::App*) data;
	
	switch (GST_MESSAGE_TYPE(msg))
	{
	case GST_MESSAGE_EOS: {
		mtl_info("End of stream");
		app->ReachedEndOfStream();
		break;
	}
	case GST_MESSAGE_ERROR: {
		gchar *debug;
		GError *error;
		gst_message_parse_error(msg, &error, &debug);
		g_free(debug);
		mtl_warn("Error %s", error->message);
		g_error_free(error);
		break;
	}
	
	case GST_MESSAGE_ASYNC_DONE: {
		app->MessageAsyncDone();
		break;
	}
	
	default: break;
	}
	
	return TRUE;
}

GstPlayer::GstPlayer(quince::App *app, int argc, char *argv[])
: app_(app)
{
	InitGst(argc, argv);
}

GstPlayer::~GstPlayer()
{
	gst_element_set_state(play_elem_, GST_STATE_NULL);
	gst_object_unref(GST_OBJECT(play_elem_));
}

void
GstPlayer::InitGst(int argc, char *argv[])
{
	gst_init(&argc, &argv);
	play_elem_ = gst_element_factory_make("playbin", "play");
	GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(play_elem_));
	gst_bus_add_watch(bus, bus_callback, app_);// loop_);
	gst_object_unref(bus);
}

void
GstPlayer::Play(Song *song_item)
{
	gst_element_set_state(play_elem_, GST_STATE_NULL);
	auto ba = song_item->uri().toLocal8Bit();
	g_object_set(G_OBJECT(play_elem_), "uri", ba.data(), NULL);
	gst_element_set_state(play_elem_, GST_STATE_PLAYING);
}

}
