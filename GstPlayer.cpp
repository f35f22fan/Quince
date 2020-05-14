#include "GstPlayer.hpp"

#include "App.hpp"
#include "Song.hpp"
#include "gui/SliderPane.hpp"

#include <QUrl>

namespace quince {

static gboolean bus_callback(GstBus *bus, GstMessage *msg, gpointer data)
{
	quince::App *app = (quince::App*) data;
	
	switch (GST_MESSAGE_TYPE(msg))
	{
	case GST_MESSAGE_EOS: {
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
	
	default: {
		//mtl_info("other");
		break;
	}
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
GstPlayer::PlayPause(Song *song)
{
/*
GST_STATE_VOID_PENDING – no pending state.
GST_STATE_NULL – the NULL state or initial state of an element.
GST_STATE_READY – the element is ready to go to PAUSED.
GST_STATE_PAUSED – the element is PAUSED, it is ready to accept and process data. Sink elements however only accept one buffer and then block.
GST_STATE_PLAYING – the element is PLAYING, the GstClock is running and the data is flowing. 
*/
	GstState new_state = song->is_playing()
		? GST_STATE_PAUSED : GST_STATE_PLAYING;
	
	if (!song->is_playing_or_paused())
	{
		gst_element_set_state(play_elem_, GST_STATE_NULL);
		auto ba = song->uri().toLocal8Bit();
		g_object_set(G_OBJECT(play_elem_), "uri", ba.data(), NULL);
	}
	
	gst_element_set_state(play_elem_, new_state);
	song->state(new_state);
	app_->UpdatePlayIcon(song);
	app_->slider_pane()->SetCurrentSong(song);
}

void
GstPlayer::StopPlaying(Song *song)
{
	gst_element_set_state(play_elem_, GST_STATE_NULL);
	song->playing_at(-1);
	song->state(GST_STATE_NULL);
	app_->slider_pane()->SetCurrentSong(nullptr);
}

void
GstPlayer::SeekTo(const i64 new_pos)
{
	auto flag = GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT;
	if (gst_element_seek_simple(play_elem_, GST_FORMAT_TIME,
		GstSeekFlags(flag), new_pos))
	{
		app_->UpdatePlayingSongPosition(new_pos);
	}
}

}
