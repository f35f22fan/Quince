#include "App.hpp"

#include "actions.hxx"
#include "audio.hh"
#include "Duration.hpp"
#include "GstPlayer.hpp"
#include "gui/Playlist.hpp"
#include "gui/SliderPane.hpp"
#include "gui/Table.hpp"
#include "gui/TableModel.hpp"
#include "io/io.hh"
#include "Song.hpp"

#include <QBoxLayout>
#include <QScrollArea>
#include <QToolBar>
#include <QUrl>

static const char *ICON_NAME_PAUSED = "media-playback-pause";
static const char *ICON_NAME_PLAY = "media-playback-start";

/* Print a tag in a human-readable format (name: value) */
static void
print_tag_foreach (const GstTagList *tags, const gchar *tag, gpointer user_data)
{
	if (strcmp(tag, "image") == 0)
		return;
	
	GValue val = { 0, };
	gchar *str_val;
	gint depth = GPOINTER_TO_INT (user_data);
	
	gst_tag_list_copy_value (&val, tags, tag);
	
	if (G_VALUE_HOLDS_STRING (&val))
		str_val = g_value_dup_string (&val);
	else
		str_val = gst_value_serialize (&val);
	
	g_print ("%*s[print_tag_foreach] %s[%s]: %s\n", 2 * depth, " ",
		gst_tag_get_nick (tag), tag, str_val);
	g_free (str_val);
	
	g_value_unset (&val);
}

/* Print information regarding a stream */
static void print_stream_info (GstDiscovererStreamInfo *info, gint depth) {
	gchar *desc = NULL;
	
	GstCaps *caps = gst_discoverer_stream_info_get_caps (info);
	
	if (caps) {
		if (gst_caps_is_fixed (caps))
			desc = gst_pb_utils_get_codec_description (caps);
		else
			desc = gst_caps_to_string (caps);
		gst_caps_unref (caps);
	}
	
	g_print ("%*s%s: %s\n", 2 * depth, " ",
		gst_discoverer_stream_info_get_stream_type_nick (info), (desc ? desc : ""));
	
	if (desc) {
		g_free (desc);
		desc = NULL;
	}
	
	const GstTagList *tags = gst_discoverer_stream_info_get_tags (info);
	if (tags) {
		g_print ("%*sTags:\n", 2 * (depth + 1), " ");
		gst_tag_list_foreach (tags, print_tag_foreach, GINT_TO_POINTER (depth + 2));
	}
}

/* Print information regarding a stream and its substreams, if any */
static void print_topology (GstDiscovererStreamInfo *info, gint depth) {
	GstDiscovererStreamInfo *next;
	
	if (!info)
		return;
	
	print_stream_info (info, depth);
	
	next = gst_discoverer_stream_info_get_next (info);
	if (next) {
		print_topology (next, depth + 1);
		gst_discoverer_stream_info_unref (next);
	} else if (GST_IS_DISCOVERER_CONTAINER_INFO (info)) {
		GList *tmp, *streams;
		
		streams = gst_discoverer_container_info_get_streams (GST_DISCOVERER_CONTAINER_INFO (info));
		for (tmp = streams; tmp; tmp = tmp->next) {
			GstDiscovererStreamInfo *tmpinf = (GstDiscovererStreamInfo *) tmp->data;
			print_topology (tmpinf, depth + 1);
		}
		gst_discoverer_stream_info_list_free (streams);
	}
}

/* This function is called every time the discoverer has information regarding
 * one of the URIs we provided.*/
static void on_discovered_cb (GstDiscoverer *discoverer,
	GstDiscovererInfo *info,
	GError *err, quince::DiscovererUserParams *user_params)
{
	const char *uri = gst_discoverer_info_get_uri (info);
	GstDiscovererResult result = gst_discoverer_info_get_result (info);
	switch (result) {
	case GST_DISCOVERER_URI_INVALID:
		g_print ("Invalid URI '%s'\n", uri);
		break;
	case GST_DISCOVERER_ERROR:
		g_print ("Discoverer error: %s\n", err->message);
		break;
	case GST_DISCOVERER_TIMEOUT:
		g_print ("Timeout\n");
		break;
	case GST_DISCOVERER_BUSY:
		g_print ("Busy\n");
		break;
	case GST_DISCOVERER_MISSING_PLUGINS:{
		const GstStructure *s;
		gchar *str;
		
		s = gst_discoverer_info_get_misc (info);
		str = gst_structure_to_string (s);
		
		g_print ("Missing plugins: %s\n", str);
		g_free (str);
		break;
	}
	case GST_DISCOVERER_OK:
		//g_print ("Discovered '%s'\n", uri);
		break;
	}
	
	if (result != GST_DISCOVERER_OK) {
		g_printerr ("This URI cannot be played\n");
		return;
	}
	
	/* If we got no error, show the retrieved information */
	u64 nano = gst_discoverer_info_get_duration(info);
	
	quince::AudioInfo audio_info = {.duration = nano, .uri = uri};
	user_params->app->GotAudioInfo(&audio_info);
	
	if (false)
	{
		const GstTagList *tags = gst_discoverer_info_get_tags (info);
		if (tags) {
			g_print ("Tags:\n");
			gst_tag_list_foreach (tags, print_tag_foreach, GINT_TO_POINTER (1));
		}
		
		g_print ("Seekable: %s\n", (gst_discoverer_info_get_seekable (info) ? "yes" : "no"));
	}
	
	if (false)
	{
		GstDiscovererStreamInfo *sinfo = gst_discoverer_info_get_stream_info (info);
		if (!sinfo)
			return;
		
		g_print ("Stream information:\n");
		
		print_topology (sinfo, 1);
		
		
		// ==> Mine
		GList *audio_streams = gst_discoverer_info_get_audio_streams(info);
		for (auto *l = audio_streams; l != NULL; l = l->next)
		{
			// do something with l->data
			auto *audio_info = (GstDiscovererAudioInfo*)l->data;
			guint bitrate = gst_discoverer_audio_info_get_bitrate(audio_info);
			mtl_info("Bitrate: %u", bitrate);
			guint channels =  gst_discoverer_audio_info_get_channels(audio_info);
			mtl_info("Channels: %u", channels);
			guint sample_rate = gst_discoverer_audio_info_get_sample_rate(audio_info);
			mtl_info("Sample Rate: %u", sample_rate);
		}
		// <== Mine
		
		gst_discoverer_stream_info_unref (sinfo);
	}
}

/* This function is called when the discoverer has finished examining
 * all the URIs we provided.*/
static void on_finished_cb (GstDiscoverer *discoverer,
	quince::DiscovererUserParams *user_params)
{
	gst_discoverer_stop (user_params->discoverer); // Stop the discoverer process
	g_object_unref (user_params->discoverer); // Free resources
	g_main_loop_quit (user_params->loop);
	g_main_loop_unref (user_params->loop);
	user_params->loop = nullptr;
}

namespace quince {

App::App(int argc, char *argv[])
{
	play_mode_ = audio::PlayMode::StopAtPlaylistEnd;
	player_ = new GstPlayer(this, argc, argv);
	CHECK_TRUE_RET_VOID(InitDiscoverer());
	CHECK_TRUE_RET_VOID(CreateGui());
	//CHECK_TRUE_RET_VOID(LoadSavedSongData());
	setWindowIcon(QIcon(":/resources/Quince.png"));
	resize(1400, 600);
}

App::~App() {
	delete player_;
	
	if (user_params_.loop != nullptr) {
		g_main_loop_quit (user_params_.loop);
		g_main_loop_unref (user_params_.loop);
		user_params_.loop = nullptr;
	}
	

// ==>int QStackedWidget::addWidget(QWidget *widget)<==
// Ownership of widget is passed on to the QStackedWidget.
//	for (gui::Playlist *item: playlists_) {
//		delete item;
//	}
	
	playlists_.clear();
}

bool
App::AddBatch(QVector<quince::Song*> &vec)
{
	bool started = false;

	for (quince::Song *song: vec)
	{
		QUrl url(song->uri());
		QByteArray full_path_ba = url.toLocalFile().toLocal8Bit();
		const char *full_path = full_path_ba.data();
		audio::Meta &meta = song->meta();
		audio::ReadFileMeta(full_path, meta);
		
		if (song->meta().is_duration_set())
			continue;
		
		if (!started) {
			// Start the discoverer process (nothing to do yet)
			gst_discoverer_start (user_params_.discoverer);
			started = true;
		}
		
		// Add a request to process asynchronously the URI passed through the command line
		auto uri_ba = song->uri().toLocal8Bit();
		mtl_info("Adding request for %s", uri_ba.data());
		if (!gst_discoverer_discover_uri_async (user_params_.discoverer, uri_ba)) {
			mtl_warn("Failed to start discovering URI '%s'\n", uri_ba.data());
			g_object_unref (user_params_.discoverer);
			return false;
		}
	}
	
	if (started)
	{
		// Create a GLib Main Loop and set it to run, so we can wait for the signals
		user_params_.loop = g_main_loop_new (NULL, FALSE);
		g_main_loop_run (user_params_.loop);
	}

	return true;
}

QAction*
App::AddAction(QToolBar *tb, const QString &icon_name,
	const QString &action_name, const char *tooltip)
{
	QAction *action = tb->addAction(QIcon::fromTheme(icon_name), QString());
	connect(action, &QAction::triggered,
		[=] {ProcessAction(action_name);});
	
	if (tooltip != nullptr)
		action->setToolTip(tooltip);
	
	return action;
}

bool
App::CreateGui()
{
	addToolBar(Qt::TopToolBarArea, CreateMediaActionsToolBar());
	
	QWidget *central_widget = new QWidget(this);
	auto *layout = new QBoxLayout(QBoxLayout::TopToBottom);
	central_widget->setLayout(layout);
	setCentralWidget(central_widget);
	
	seek_pane_ = new gui::SliderPane(this);
	layout->addWidget(seek_pane_);

	tab_bar_ = new QTabBar();
	layout->addWidget(tab_bar_);
	
	QWidget *stack_widget = new QWidget(central_widget);
	playlist_stack_ = new QStackedLayout();
	stack_widget->setLayout(playlist_stack_);
	layout->addWidget(stack_widget);
	
	auto *playlist = CreatePlaylist("Default");
	seek_pane_->DisplayPlaylistDuration(playlist);
	
	addToolBar(Qt::BottomToolBarArea, CreatePlaylistActionsToolBar());
	
	return true;
}

QToolBar*
App::CreateMediaActionsToolBar()
{
	QToolBar *tb = new QToolBar(this);
	AddAction(tb, "go-previous", actions::MediaPlayPrev);
	play_pause_action_ = AddAction(tb, ICON_NAME_PLAY,
		actions::MediaPlayPause);
	//AddAction(tb, ICON_NAME_PAUSE, actions::MediaPause);
	AddAction(tb, "media-playback-stop", actions::MediaPlayStop);
	AddAction(tb, "go-next", actions::MediaPlayNext);
	
	return tb;
}

QToolBar*
App::CreatePlaylistActionsToolBar()
{
	QToolBar *tb = new QToolBar(this);
	AddAction(tb, "address-book-new", actions::PlaylistNew, "New Playlist");
	AddAction(tb, "list-remove", actions::PlaylistDelete, "Delete Playlist");
	AddAction(tb, "document-properties", actions::PlaylistRename, "Rename Playlist");
	
	return tb;
}

gui::Playlist*
App::CreatePlaylist(const QString &name, int *index)
{
	gui::Playlist *playlist = new gui::Playlist(this, name);
	int n = playlist_stack_->addWidget(playlist);
	tab_bar_->addTab(name);
	playlists_.append(playlist);
	
	if (index != nullptr)
		*index = n;
	
	return playlist;
}

gui::TableModel*
App::current_table_model()
{
	const int index = tab_bar_->currentIndex();
	
	if (index == -1)
	{
		mtl_trace();
		return nullptr;
	}
	
	if (index >= playlists_.size())
	{
		mtl_trace();
		return nullptr;
	}
	
	gui::Playlist *playlist = playlists_[index];
	
	return playlist->table_model();
}

QVector<Song*>*
App::current_playlist_songs()
{
	auto *model = current_table_model();
	
	if (model == nullptr) {
		mtl_trace();
		return nullptr;
	}
	
	return &model->songs();
}

Song*
App::GetCurrentSong(int *index)
{
	auto *songs = current_playlist_songs();
	
	if (songs == nullptr)
		return nullptr;
	
	for (int i = 0; i < songs->size(); i++)
	{
		auto *song = (*songs)[i];
		
		if (song->is_playing_or_paused())
		{
			if (index != nullptr)
				*index = i;
			
			return song;
		}
	}
	
	if (index != nullptr)
		*index = -1;
	
	return nullptr;
}

Song*
App::GetFirstSongInCurrentPlaylist()
{
	auto *songs = current_playlist_songs();
	
	if (songs == nullptr || songs->empty())
		return nullptr;
	
	return (*songs)[0];
}

void
App::GotAudioInfo(AudioInfo *info)
{
	auto d = quince::Duration::FromNs(info->duration);
	auto ds = d.toDurationString().toLocal8Bit();
	
	auto *songs = current_playlist_songs();
	CHECK_PTR_RET_VOID(songs);
	
	for (quince::Song *song: *songs)
	{
		if (song->uri() == info->uri) {
			song->meta().duration(info->duration);
			break;
		}
	}
}

bool
App::InitDiscoverer()
{
	user_params_.app = this;
	
	// Instantiate the Discoverer
	GError *err = NULL;
	user_params_.discoverer = gst_discoverer_new (5 * GST_SECOND, &err);
	
	if (!user_params_.discoverer)
	{
		g_print ("Error creating discoverer instance: %s\n", err->message);
		g_clear_error (&err);
		return false;
	}
	
	// Connect to the interesting signals
	g_signal_connect (user_params_.discoverer, "discovered", G_CALLBACK (on_discovered_cb), &user_params_);
	g_signal_connect (user_params_.discoverer, "finished", G_CALLBACK (on_finished_cb), &user_params_);
	
	return true;
}

bool
App::LoadSavedSongData(gui::TableModel *model)
{
	CHECK_PTR(model);
	
	QVector<Song*> &songs = model->songs();
	
	const i64 sec = 1000'000'000L;
	const i64 min = sec * 60L;
	const i64 hour = min * 60L;
	const i64 day = hour * 24L;
	
	QVector<io::File> files;
	const QString mp3_dir_path = "/media/data/Audio/0 Best Hits/";
	const QString flac_dir_path = "/media/data/Audio/0 Chillout/Psydub/Phaeleh - (2010) Fallen Light [Afterglo, AFTRCD1001]/";
	const QString opus_dir_path = "/media/data/Audio/Twin Peaks/Twin Peaks - Music From the Limited Event Series (2017)(FLAC)(CD)/";
	
	QString dir_path = opus_dir_path;
	
	if (!dir_path.endsWith('/'))
		dir_path.append('/');
	
	if (io::ListFiles(dir_path, files, 0, io::IsSongExtension) != io::Err::Ok) {
		mtl_trace();
		return false;
	}
	
	for (io::File &file: files)
	{
		auto *song = Song::FromFile(file, dir_path);
		
		if (song != nullptr)
			songs.append(song);
	}
	
	bool ok = AddBatch(songs);
	model->SignalRowsInserted(0, songs.size() - 1);
	
	return ok;
}

void
App::MessageAsyncDone()
{
	Song *song = GetCurrentSong();
	
	if (song == nullptr) {
		mtl_trace();
		return;
	}
	
	GstState state;
	
	GstStateChangeReturn ret = gst_element_get_state (play_elem(),
		&state, NULL, GST_CLOCK_TIME_NONE);
	
	if (ret != GST_STATE_CHANGE_SUCCESS) {
		mtl_trace();
		return;
	}
	
	//mtl_info("State is: %s", audio::StateToString(state));
	
	if (!song->meta().is_duration_set())
	{
		i64 duration = -1;
		gboolean ok = gst_element_query_duration (play_elem(),
			GST_FORMAT_TIME, &duration);
			
		if (!ok) {
			mtl_trace();
			return;
		}
		
		song->meta().duration(duration);
		mtl_info("DURATION IS: %ld", duration);
	}
}

GstElement*
App::play_elem() const { return player_->play_elem(); }

void
App::PlaySong(const audio::Pick direction)
{
	auto *vec = current_playlist_songs();
	
	if (vec == nullptr)
		return;
	
	int current_song_index;
	GetCurrentSong(&current_song_index);
	int song_index = PickSong(vec, current_song_index, direction);
	
	if (song_index == -1)
		return;
	
	if (song_index >= vec->size()) {
		mtl_trace();
		return;
	}
	
	Song *playing_song = (*vec)[song_index];
	player_->PlayPause(playing_song);
	current_table_model()->UpdateRangeDefault(song_index);
}

int
App::PickSong(QVector<Song*> *vec, const int current_song_index,
	const audio::Pick pick)
{
	if (pick == audio::Pick::Next)
	{
		if (current_song_index == -1) {
			if (GetFirstSongInCurrentPlaylist() != nullptr)
				return 0;
			return -1;
		} else if (current_song_index == vec->size() -1) {
			if (play_mode_ == audio::PlayMode::RepeatPlaylist) {
				if (GetFirstSongInCurrentPlaylist() != nullptr)
					return 0;
			}
			
			return -1;
		} else {
			auto *last_song = (*vec)[current_song_index];
			last_song->playing_at(-1);
			last_song->state(GST_STATE_NULL);
			current_table_model()->UpdateRangeDefault(current_song_index);
			return current_song_index + 1;
		}
	} else if (pick == audio::Pick::Prev) {
		
		if (current_song_index == -1) {
			auto *model = current_table_model();
			
			if (model == nullptr)
				return -1;
			
			QVector<Song*> &vec = model->songs();
			return vec.isEmpty() ? -1 : vec.size() - 1;
		} else if (current_song_index == 0) {
			return -1;
		} else {
			auto *last_song = (*vec)[current_song_index];
			last_song->playing_at(-1);
			last_song->state(GST_STATE_NULL);
			current_table_model()->UpdateRangeDefault(current_song_index);
			return current_song_index - 1;
		}
	} else {
		mtl_warn();
		return -1;
	}
}

void
App::PlayStop()
{
	int index;
	Song *song = GetCurrentSong(&index);
	
	if (song == nullptr)
		return;
	
	player_->StopPlaying(song);
	current_table_model()->UpdateRangeDefault(index);
	UpdatePlayIcon(song);
}

void
App::ProcessAction(const QString &action_name)
{
	if (action_name == actions::MediaPlayPause)
	{
		int song_index = -1;
		Song *playing_song = GetCurrentSong(&song_index);
		
		if (playing_song == nullptr)
		{
			playing_song = GetFirstSongInCurrentPlaylist();
			
			if (playing_song == nullptr) {
				mtl_trace();
				return;
			}
			
			song_index = 0;
		}
		
		player_->PlayPause(playing_song);
		current_table_model()->UpdateRangeDefault(song_index);
	} else if (action_name == actions::MediaPlayNext) {
		PlaySong(audio::Pick::Next);
	} else if (action_name == actions::MediaPlayPrev) {
		PlaySong(audio::Pick::Prev);
	} else if (action_name == actions::MediaPlayStop) {
		PlayStop();
	} else {
		auto ba = action_name.toLocal8Bit();
		mtl_trace("Action skipped: \"%s\"", ba.data());
	}
}

void
App::ReachedEndOfStream()
{
	auto *vec = current_playlist_songs();
	
	if (vec == nullptr)
	{
		mtl_trace();
		return;
	}
	
	int last_playing = -1;
	
	for (int i = 0; i < vec->size(); i++)
	{
		auto *song = (*vec)[i];
		
		if (song->is_playing()) {
			song->playing_at(-1);
			last_playing = i;
			break;
		}
	}
	
	if (last_playing == -1 || play_mode_ == audio::PlayMode::StopAtTrackEnd)
		return;
	
	current_table_model()->UpdateRangeDefault(last_playing);
	
	if (play_mode_ == audio::PlayMode::RepeatPlaylist ||
		play_mode_ == audio::PlayMode::StopAtPlaylistEnd) {
		PlaySong(audio::Pick::Next);
	} else if (play_mode_ == audio::PlayMode::RepeatTrack) {
		auto *song = (*vec)[last_playing];
		player_->PlayPause(song);
		current_table_model()->UpdateRangeDefault(last_playing);
	}
}

void
App::UpdatePlayIcon(Song *song)
{
	CHECK_PTR_RET_VOID(song);
	
	const char *icon_name = song->is_playing() ?
		ICON_NAME_PAUSED : ICON_NAME_PLAY;
	
	play_pause_action_->setIcon(QIcon::fromTheme(icon_name));
}

gui::UpdateTableRange
App::UpdatePlayingSongPosition(const i64 pos_is_known)
{
	Song *song = GetCurrentSong();
	
	if (song == nullptr || !song->is_playing())
		return gui::UpdateTableRange::OneColumn;
	
	i64 position = -1;
	
	if (pos_is_known >= 0) {
		position = pos_is_known;
	} else if (!gst_element_query_position (play_elem(), GST_FORMAT_TIME,
		&position))
	{
		//mtl_trace();
		return gui::UpdateTableRange::OneColumn;
	}
	
	song->playing_at(position);
	
	if (!song->meta().is_duration_set())
	{
		mtl_warn("WTH");
		/*
		duration = -1;
		gboolean ok = gst_element_query_duration (play_elem,
			GST_FORMAT_TIME, &duration);
			
		if (!ok) {
			mtl_trace();
			return false;
		}
		song->meta().duration(duration);
		*/
		return gui::UpdateTableRange::WholeRow;
		
	}
	
	seek_pane_->UpdatePosition(position);
	
	return gui::UpdateTableRange::OneColumn;
}

} // quince::
