#include "App.hpp"

#include "actions.hxx"
#include "audio.hh"
#include "audio/flac.hh"
#include "audio/mp3.hh"
#include "audio/ogg.hh"
#include "Duration.hpp"
#include "GstPlayer.hpp"
#include "gui/SliderPane.hpp"
#include "gui/Table.hpp"
#include "gui/TableModel.hpp"
#include "io/io.hh"
#include "SongItem.hpp"

#include <QBoxLayout>
#include <QScrollArea>
#include <QToolBar>
#include <QUrl>

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
	player_ = new GstPlayer(this, argc, argv);
	CHECK_TRUE_RET_VOID(InitDiscoverer());
	CHECK_TRUE_RET_VOID(LoadSavedSongData());
	CHECK_TRUE_RET_VOID(CreateGui());
	setWindowIcon(QIcon(":/resources/Quince.png"));
}

App::~App() {
	delete player_;
	
	if (user_params_.loop != nullptr) {
		g_main_loop_quit (user_params_.loop);
		g_main_loop_unref (user_params_.loop);
		user_params_.loop = nullptr;
	}
}

bool
App::AddBatch(QVector<quince::SongItem*> &vec)
{
	bool started = false;

	for (quince::SongItem *song: vec)
	{
		QUrl url(song->uri());
		QByteArray full_path_ba = url.toLocalFile().toLocal8Bit();
		const char *full_path = full_path_ba.data();
		audio::Meta &meta = song->meta();
		
		audio::ReadFileMeta(full_path, meta);
		
		if (song->meta().duration() != -1)
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

void
App::AddAction(QToolBar *tb, const QString &icon_name, const QString &action_name)
{
	QAction *action = tb->addAction(QIcon::fromTheme(icon_name), QString());
	connect(action, &QAction::triggered,
		[=] {ProcessAction(action_name);});
}

bool
App::CreateGui()
{
	addToolBar(Qt::TopToolBarArea, CreateMediaActionsToolBar());
	
	QWidget *central_widget = new QWidget(this);
	auto *layout = new QBoxLayout(QBoxLayout::TopToBottom);
	
	slider_pane_ = new gui::SliderPane(this);
	layout->addWidget(slider_pane_);
	
	if (table_model_ == nullptr)
		table_model_ = new gui::TableModel(this);
	
	table_ = gui::Table::Create(table_model_);
	connect(table_, &QTableView::doubleClicked, this, &App::PlaylistDoubleClicked);
	layout->addWidget(table_);
	central_widget->setLayout(layout);
	setCentralWidget(central_widget);
	
	addToolBar(Qt::BottomToolBarArea, CreatePlaylistActionsToolBar());
	
	return true;
}

QToolBar*
App::CreateMediaActionsToolBar()
{
	QToolBar *tb = new QToolBar(this);
	AddAction(tb, "go-previous", actions::MediaGoPrev);
	AddAction(tb, "media-playback-start", actions::MediaPlayPause);
	//AddAction(tb, "media-playback-pause", actions::MediaPause);
	AddAction(tb, "media-playback-stop", actions::MediaStop);
	AddAction(tb, "go-next", actions::MediaGoNext);
	
	return tb;
}

QToolBar*
App::CreatePlaylistActionsToolBar()
{
	QToolBar *tb = new QToolBar(this);
	AddAction(tb, "list-add", actions::PlaylistNew);
	AddAction(tb, "list-remove", actions::PlaylistDelete);
	AddAction(tb, "edit-redo", actions::PlaylistRename);
	
	return tb;
}

QVector<SongItem*>*
App::current_playlist_songs() { return &table_model_->songs(); }

SongItem*
App::GetPlayingSong()
{
	for (auto *song: table_model_->songs())
	{
		if (song->is_playing()) {
			return song;
		}
	}
	
	return nullptr;
}

void
App::GotAudioInfo(AudioInfo *info)
{
	auto d = quince::Duration::FromNs(info->duration);
	auto ds = d.toDurationString().toLocal8Bit();
	
	auto *songs = current_playlist_songs();
	CHECK_PTR_RET_VOID(songs);
	
	for (quince::SongItem *song: *songs)
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
App::LoadSavedSongData()
{
	if (table_model_ == nullptr)
		table_model_ = new gui::TableModel(this);
	
	QVector<SongItem*> *songs = current_playlist_songs();
	CHECK_PTR(songs);
	
	const i64 sec = 1000'000'000L;
	const i64 min = sec * 60L;
	const i64 hour = min * 60L;
	const i64 day = hour * 24L;
	
	QVector<io::File> files;
	//QString dir_path = "/media/data/Audio/0 Best Hits/";
	QString dir_path = "/media/data/Audio/0 Chillout/Psydub/Phaeleh - (2010) Fallen Light [Afterglo, AFTRCD1001]/";
	
	if (!dir_path.endsWith('/'))
		dir_path.append('/');
	
	if (io::ListFiles(dir_path, files, 0, io::IsSongExtension) != io::Err::Ok) {
		mtl_trace();
		return false;
	}
	
	for (io::File &file: files)
	{
		auto *song = SongItem::FromFile(file, dir_path);
		songs->append(song);
	}
	
	return AddBatch(*songs);
}

void
App::MessageAsyncDone()
{
	SongItem *song = GetPlayingSong();
	
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
	
	mtl_info("State is: %s", audio::StateToString(state));
	
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
App::PlaylistDoubleClicked(QModelIndex index)
{
	int row = index.row();
	
	auto &songs = table_model_->songs();
	
	if (row >= songs.size()) {
		mtl_trace();
		return;
	}
	
	int last_playing = -1;
	
	for (int i = 0; i < songs.size(); i++)
	{
		auto *song = songs[i];
		
		if (song->is_playing()) {
			song->playing_at(-1);
			last_playing = i;
			break;
		}
	}
	
	SongItem *song = songs[row];
	player_->Play(song);
	song->playing_at(0);
	table_model_->UpdateRange(row, gui::Column::Name, last_playing, gui::Column::PlayingAt);
}

void
App::ProcessAction(const QString &action_name)
{
	auto ba = action_name.toLocal8Bit();
	mtl_trace("Action: \"%s\"", ba.data());
}

void
App::ReachedEndOfStream()
{
	auto &songs = table_model_->songs();
	int last_playing = -1;
	
	for (int i = 0; i < songs.size(); i++)
	{
		auto *song = songs[i];
		
		if (song->is_playing()) {
			song->playing_at(-1);
			last_playing = i;
			break;
		}
	}
	
	table_model_->UpdateRange(last_playing, gui::Column::Name,
		last_playing, gui::Column::PlayingAt);
}

} // quince::
