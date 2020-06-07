#include "App.hpp"

#include "actions.hxx"
#include "audio.hh"
#include "ByteArray.hpp"
#include "Duration.hpp"
#include "GstPlayer.hpp"
#include "gui/Playlist.hpp"
#include "gui/SeekPane.hpp"
#include "gui/Table.hpp"
#include "gui/TableModel.hpp"
#include "io/io.hh"
#include "Song.hpp"

#include <QBoxLayout>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QListView>
#include <QScrollArea>
#include <QStandardPaths>
#include <QToolBar>
#include <QTreeView>
#include <QUrl>

#include <sys/stat.h>
#include <sys/types.h>

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
	CHECK_TRUE_VOID(InitDiscoverer());
	CHECK_TRUE_VOID(CreateGui());
	LoadPlaylists();
	setWindowIcon(QIcon(":/resources/Quince.png"));
	resize(1400, 600);
}

App::~App()
{
	SavePlaylistsToDisk();
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

i32
App::active_playlist_index() const {
	
	if (playlists_.isEmpty())
		return -1;
	
	return playlists_cb_->currentIndex(); // initiate it
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

void
App::AddFilesToPlaylist(QVector<io::File> &files, gui::Playlist *playlist)
{
	CHECK_PTR_VOID(playlist);
	gui::TableModel *model = playlist->table_model();
	QVector<Song*> &songs = model->songs();
	i32 first = songs.size();
	i32 added = 0;
	
	for (io::File &file: files)
	{
		auto *song = Song::FromFile(file);
		
		if (song != nullptr)
		{
			added++;
			songs.append(song);
		}
	}
	
	if (added == 0)
		return;
	
	AddBatch(songs);
	const i32 last = first + added - 1;
	model->SignalRowsInserted(first, last);
}

void
App::AddFolderToPlaylist(const QString &dp, gui::Playlist *playlist)
{
	CHECK_PTR_VOID(playlist);
	QString dir_path = dp;
	
	if (!dir_path.endsWith('/'))
		dir_path.append('/');
	QVector<io::File> files;
	
	if (io::ListFiles(dir_path, files, 0, io::IsSongExtension) != io::Err::Ok) {
		mtl_trace();
		return;
	}
	
	AddFilesToPlaylist(files, playlist);
}

void
App::AskAddSongFilesToPlaylist()
{
	QFileDialog box;
// ==> Multi-file selection workaround
	box.setFileMode(QFileDialog::Directory);
	box.setOption(QFileDialog::DontUseNativeDialog, true);
	
	// Try to select multiple files and folders at the same time in QFileDialog
	QListView *l = box.findChild<QListView*>("listView");
	if (l)
		l->setSelectionMode(QAbstractItemView::MultiSelection);

	QTreeView *t = box.findChild<QTreeView*>();
	if (t)
		t->setSelectionMode(QAbstractItemView::MultiSelection);
// <== Multi-file selection workaround
	
	if (!box.exec())
	{
		mtl_trace();
		return;
	}
	
	gui::Playlist *playlist = GetActivePlaylist();
	CHECK_PTR_VOID(playlist);
	const QStringList filenames = box.selectedFiles();
	struct stat st;
	QVector<io::File> files;
	
	for (auto next: filenames) {
		auto ba = next.toLocal8Bit();
		
		if (lstat(ba.data(), &st) == 0) {
			if (S_ISDIR(st.st_mode)) {
				AddFolderToPlaylist(next, playlist);
			} else {
				files.clear();
				io::File file;
				
				if (io::FileFromPath(file, next) == io::Err::Ok)
				{
					files.append(file);
					AddFilesToPlaylist(files, playlist);
				}
			}
		}
	}
	
	seek_pane_->SetActive(playlist);
}

void
App::AskNewPlaylist()
{
	bool ok;
	QString text = QInputDialog::getText(this,
		"New Playlist",
		QLatin1String("Name:"), QLineEdit::Normal,
		"", &ok);
	if (ok && !text.isEmpty())
		CreatePlaylist(text);
}

bool
App::CreateGui()
{
	addToolBar(Qt::TopToolBarArea, CreateMediaActionsToolBar());
	
	QWidget *central_widget = new QWidget(this);
	auto *layout = new QBoxLayout(QBoxLayout::TopToBottom);
	central_widget->setLayout(layout);
	setCentralWidget(central_widget);
	
	seek_pane_ = new gui::SeekPane(this);
	layout->addWidget(seek_pane_);

	QWidget *stack_widget = new QWidget(central_widget);
	playlist_stack_ = new QStackedLayout();
	stack_widget->setLayout(playlist_stack_);
	layout->addWidget(stack_widget);
	
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
	AddAction(tb, "media-playback-stop", actions::MediaPlayStop);
	AddAction(tb, "go-next", actions::MediaPlayNext);
	
	QWidget *space = new QWidget();
	space->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	space->setVisible(true);
	tb->addWidget(space);
	
	
	auto *song_entries_label = new QLabel(QLatin1String("Song Entries: "));
	tb->addWidget(song_entries_label);
	
	auto *w = AddAction(tb, "list-add", actions::AddSongFilesToPlaylist);
	w->setToolTip("Add song files and folders to playlist");
	
	tb->addSeparator();
	w = AddAction(tb, "list-remove", actions::RemoveSongFromPlaylist);
	w->setToolTip("Remove song from playlist");
	
	
	return tb;
}

QToolBar*
App::CreatePlaylistActionsToolBar()
{
	QToolBar *tb = new QToolBar(this);
	
	auto *playlists_label = new QLabel(QLatin1String("Playlists: "));
	tb->addWidget(playlists_label);
	
	AddAction(tb, "list-add", actions::PlaylistNew, "New Playlist");
	AddAction(tb, "list-remove", actions::PlaylistDelete, "Delete Playlist");
	AddAction(tb, "document-properties", actions::PlaylistRename, "Rename Playlist");
	
	playlists_cb_ = new QComboBox();
	connect(playlists_cb_,
		QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &App::PlaylistComboIndexChanged);
	tb->addWidget(playlists_cb_); // takes ownership
	
	return tb;
}

gui::Playlist*
App::CreatePlaylist(const QString &name, int *index)
{
	for (gui::Playlist *p : playlists_)
	{
		if (p->name() == name)
			return nullptr;
	}
	
	gui::Playlist *playlist = new gui::Playlist(this, name);
	playlist->id(GenNewPlaylistId());
	int n = playlist_stack_->addWidget(playlist);
	playlists_cb_->addItem(name);
	playlists_.append(playlist);
	
	if (index != nullptr)
		*index = n;
	
	return playlist;
}

u64
App::GenNewPlaylistId() const
{
	u64 greatest = 0;
	
	for (gui::Playlist *p : playlists_) {
		if (p->id() >= greatest)
			greatest = p->id() + 1;
	}
	
	return greatest;
}

gui::Playlist*
App::GetActivePlaylist()
{
	i32 index = active_playlist_index();
	
	if (index == -1)
		return nullptr;
	
	return playlists_[index];
}

gui::TableModel*
App::current_table_model()
{
	const int index = active_playlist_index();
	
	if (index == -1)
		return nullptr;
	
	return playlists_[index]->table_model();
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

int
App::GetIndex(gui::Playlist *playlist) const
{
	const int count = playlists_.size();
	
	for (int i = 0; i < count; i++)
	{
		auto *p = playlists_[i];
		if (p == playlist)
			return i;
	}
	
	return -1;
}

void
App::GotAudioInfo(AudioInfo *info)
{
	auto d = quince::Duration::FromNs(info->duration);
	auto ds = d.toDurationString().toLocal8Bit();
	
	auto *songs = current_playlist_songs();
	CHECK_PTR_VOID(songs);
	
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

void
App::LoadPlaylist(const QString &full_path)
{
	ByteArray ba;
	
	if (io::ReadFile(full_path, ba) != io::Err::Ok)
	{
		auto path = full_path.toLocal8Bit();
		mtl_trace("Couldn't read file: %s", path.data());
		return;
	}
	
	i32 cache_version = ba.next_i32();
	
	if (cache_version != quince::PlaylistCacheVersion)
	{
		mtl_info("Cache version %d not supported, need %d",
			cache_version, quince::PlaylistCacheVersion);
		return;
	}
	
	QString playlist_name = ba.next_string();
	const bool is_active = ba.next_u8() == 1;
	auto *playlist = CreatePlaylist(playlist_name);
	CHECK_PTR_VOID(playlist);
	auto &songs = playlist->songs();
	const i32 song_count = ba.next_i32();
	
	for (i32 i = 0; i < song_count; i++)
	{
		Song *song = Song::From(ba);
		
		if (song != nullptr)
			songs.append(song);
	}
	
	gui::TableModel *model = playlist->table_model();
	model->SignalRowsInserted(0, song_count - 1);
	
	if (is_active)
		SetActive(playlist);
}

void
App::LoadPlaylists()
{
	QString dir_path;
	CHECK_TRUE_VOID(QueryPlaylistsSaveFolder(dir_path));
	
	if (!dir_path.endsWith('/'))
		dir_path.append('/');
	
	QVector<QString> names;
	
	if (io::ListFileNames(dir_path, names) != io::Err::Ok)
		return;
	
	for (const QString &filename: names)
		LoadPlaylist(dir_path + filename);
}

void
App::LoadSavedSongData(gui::Playlist *playlist)
{
	CHECK_PTR_VOID(playlist);
	
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
		return;
	}
	
	AddFilesToPlaylist(files, playlist);
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
App::PlaylistComboIndexChanged(int index)
{
	if (index != -1 && index < playlists_.size())
		SetActive(playlists_[index]);
}

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
			
			if (playing_song == nullptr)
				return;
			
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
	} else if (action_name == actions::AddSongFilesToPlaylist) {
		AskAddSongFilesToPlaylist();
	} else if (action_name == actions::RemoveSongFromPlaylist) {
		RemoveSelectedSong();
	} else if (action_name == actions::PlaylistNew) {
		AskNewPlaylist();
	} else {
		auto ba = action_name.toLocal8Bit();
		mtl_trace("Action skipped: \"%s\"", ba.data());
	}
}

bool
App::QueryAppConfigPath(QString &path)
{
	QString config_path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
	
	if (!config_path.endsWith('/'))
		config_path.append('/');
	
	CHECK_TRUE(io::EnsureDir(config_path, AppConfigName));
	path = config_path + AppConfigName;
	
	return true;
}

bool
App::QueryPlaylistsSaveFolder(QString &ret_val)
{
	QString app_config_path;
	CHECK_TRUE(QueryAppConfigPath(app_config_path));
	const QString subdir_name = QLatin1String("/Playlists");
	CHECK_TRUE(io::EnsureDir(app_config_path, subdir_name));
	ret_val = app_config_path + subdir_name;
	
	return true;
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
App::RemoveSelectedSong()
{
	gui::Playlist *playlist = GetActivePlaylist();
	CHECK_PTR_VOID(playlist);
	const i32 index = playlist->RemoveSelectedSong();
	
	if (index != -1)
	{
		seek_pane_->UpdatePlaylistDuration(playlist);
	}
}

void
App::SaveLastPlaylistState(const i32 index)
{
	if (index == -1 || index >= playlists_.size())
		return;
	
	gui::Playlist *playlist = playlists_[index];
	
	Song *song = playlist->GetCurrentSong(nullptr);
	
	if (song != nullptr)
	{
		if (song->is_playing())
			song->state(GST_STATE_PAUSED);
	}
	
}

bool
App::SavePlaylist(gui::Playlist *playlist, const QString &dir_path,
const bool is_active)
{
	quince::ByteArray ba;
	ba.add_i32(PlaylistCacheVersion);
	ba.add_string(playlist->name());
	ba.add_u8(is_active ? 1 : 0);
	auto &songs = playlist->songs();
	const i32 count = songs.size();
	ba.add_i32(count);
	
	for (int i = 0; i < count; i++)
	{
		songs[i]->SaveTo(ba);
	}
	
//	ba.to(sizeof(i32));
//	QString s = ba.next_string();
//	auto s_ba = s.toLocal8Bit();
//	mtl_info("Playlist name: \"%s\"", s_ba.data());
	
	QString full_path = dir_path + QChar('/')
		+ QString::number(playlist->id());
	
	if (io::WriteToFile(full_path, ba.data(), ba.size()) != io::Err::Ok) {
		mtl_warn("Error occured writing to file");
		return false;
	}
	
	return true;
}

bool
App::SavePlaylistsToDisk()
{
	QString path;
	CHECK_TRUE(QueryPlaylistsSaveFolder(path));
	
	bool ok = true;
	
	auto *active = GetActivePlaylist();
	
	for (gui::Playlist *p : playlists_)
	{
		if (!SavePlaylist(p, path, p == active))
			ok = false; // yet try to save other playlists
	}
	
	return ok;
}

void
App::SetActive(gui::Playlist *playlist)
{
	if (last_playlist_index_ != -1)
		SaveLastPlaylistState(last_playlist_index_);
	
	seek_pane_->SetActive(playlist);
	const int index = GetIndex(playlist);
	playlist_stack_->setCurrentIndex(index);
	
	Song *song = playlist->GetCurrentSong(nullptr);
	
	if (song != nullptr)
	{
		auto ba = song->display_name().toLocal8Bit();
		//mtl_info("Song: %s", ba.data());
		player_->SetSeekAndPause(song);
	} else {
		//mtl_info("active song == nullptr");
	}
	
	last_playlist_index_ = index;
}

void
App::UpdatePlayIcon(Song *song)
{
	const char *icon_name;
	
	if (song == nullptr) {
		icon_name = ICON_NAME_PLAY;
	} else {
		icon_name = song->is_playing() ? ICON_NAME_PAUSED : ICON_NAME_PLAY;
	}
	
	play_pause_action_->setIcon(QIcon::fromTheme(icon_name));
}

gui::UpdateTableRange
App::UpdatePlayingSongPosition(const i64 pos_is_known)
{
	Song *song = GetCurrentSong();
	
	if (song == nullptr || !song->is_playing_or_paused())
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
