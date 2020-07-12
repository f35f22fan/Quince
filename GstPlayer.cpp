#include "GstPlayer.hpp"

#include "App.hpp"
#include "Song.hpp"
#include "gui/SeekPane.hpp"

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
	
	case GST_MESSAGE_ASYNC_START: {
		mtl_info("async start");
	}
	
	case GST_MESSAGE_ASYNC_DONE: {
		GstPlayer *player = app->player();
		auto &ssp = player->set_seek_and_pause_;
		if (ssp.pending) {
			ssp.pending = false;
			player->SetSeekAndPause_Finish();
		} else if (ssp.pending2) {
			ssp.pending2 = false;
			app->UpdatePlayingSongPosition(ssp.new_pos);
		} else {
			app->MessageAsyncDone();
		}
		break;
	};
	
	/*case GST_MESSAGE_STATE_CHANGED: {
		mtl_info("state changed");
	};
	
	case GST_MESSAGE_STEP_DONE: {
		mtl_info("Step done");
	}
	
	case GST_MESSAGE_SEGMENT_START: {
		mtl_info("Segment start");
	}
	
	case GST_MESSAGE_SEGMENT_DONE: {
		mtl_info("Segment done");
	}
		
	case GST_MESSAGE_DURATION: {
		mtl_info("Duration");
	}
	
	case GST_MESSAGE_REQUEST_STATE: {
		mtl_info("request state");
	} */
	
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
GstPlayer::Pause(Song *song)
{
/*
GST_STATE_VOID_PENDING – no pending state.
GST_STATE_NULL – the NULL state or initial state of an element.
GST_STATE_READY – the element is ready to go to PAUSED.
GST_STATE_PAUSED – the element is PAUSED, it is ready to accept and process data. Sink elements however only accept one buffer and then block.
GST_STATE_PLAYING – the element is PLAYING, the GstClock is running and the data is flowing. 
*/
	GstState new_state = GST_STATE_PAUSED;
	gst_element_set_state(play_elem_, new_state);
	
	if (song != nullptr) {
		song->state(new_state);
		app_->seek_pane()->SetCurrentOrUpdate(song);
	}
	
	app_->UpdatePlayIcon(GST_STATE_PLAYING);
	app_->last_play_state(new_state);
}

void
GstPlayer::Play(Song *song)
{
	if (song != nullptr)
	{
		song->FillIn(temp_song_info_);
		
		if (!song->is_playing_or_paused())
		{
			gst_element_set_state(play_elem_, GST_STATE_NULL);
			auto ba = song->uri().toLocal8Bit();
			g_object_set(G_OBJECT(play_elem_), "uri", ba.data(), NULL);
		}
	}
	
	GstState new_state = GST_STATE_PLAYING;
	gst_element_set_state(play_elem_, new_state);
	
	if (song != nullptr) {
		song->state(new_state);
		app_->seek_pane()->SetCurrentOrUpdate(song);
	}
	
	app_->UpdatePlayIcon(GST_STATE_PAUSED);
	app_->last_play_state(new_state);
}

void
GstPlayer::SetSeekAndPause_Start(Song *song)
{
	gst_element_set_state(play_elem_, GST_STATE_NULL);
	auto ba = song->uri().toLocal8Bit();
	g_object_set(G_OBJECT(play_elem_), "uri", ba.data(), NULL);
	
	set_seek_and_pause_.pending = true;
	set_seek_and_pause_.song = song;
	gst_element_set_state(play_elem_, GST_STATE_PAUSED);
	// now waiting for async_done on the bus to trigger SetSeekAndPause_Finish()
}

void
GstPlayer::SetSeekAndPause_Finish()
{
	Song *song = set_seek_and_pause_.song;
	app_->seek_pane()->SetCurrentOrUpdate(song);
	app_->UpdatePlayIcon(GST_STATE_PLAYING);
	
	const i64 new_pos = song->playing_at();
	//mtl_info("seek to: %ld", song->playing_at());
	set_seek_and_pause_.pending2 = true;
	set_seek_and_pause_.new_pos = new_pos;
	SeekTo(new_pos);
}

void
GstPlayer::StopPlaying(Song *song)
{
	gst_element_set_state(play_elem_, GST_STATE_NULL);
	
	if (song != nullptr) {
		song->playing_at(-1);
		song->state(GST_STATE_NULL);
	}
	
	app_->seek_pane()->SetCurrentOrUpdate(song);
}

void
GstPlayer::SeekTo(const i64 new_pos)
{
	auto flag = GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT;
	
	if (gst_element_seek_simple(play_elem_, GST_FORMAT_TIME,
		GstSeekFlags(flag), new_pos))
	{
		if (!set_seek_and_pause_.pending2)
			app_->UpdatePlayingSongPosition(new_pos);
	}
}

}
