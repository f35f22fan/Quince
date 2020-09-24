#include "App.hpp"

#include "actions.hxx"
#include "ByteArray.hpp"
#include "Duration.hpp"
#include "GstPlayer.hpp"
#include "gui/Playlist.hpp"
#include "gui/PlaylistStackWidget.hpp"
#include "gui/SeekPane.hpp"
#include "gui/Table.hpp"
#include "gui/TableModel.hpp"
#include "io/File.hpp"
#include "io/io.hh"
#include "quince.hh"
#include "Song.hpp"

#include "shared/global_hotkeys.hpp"

#include <QApplication>
#include <QBoxLayout>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QLibrary>
#include <QListView>
#include <QMessageBox>
#include <QScrollArea>
#include <QShortcut>
#include <QStandardPaths>
#include <QToolBar>
#include <QTreeView>
#include <QUrl>

#include <QDBusConnection>
#include <QDBusConnectionInterface>

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

/* This function is called every time the discoverer has information
 regarding one of the URIs we provided.*/
static void on_discovered_cb (GstDiscoverer *discoverer,
	GstDiscovererInfo *info,
	GError *err, quince::DiscovererUserParams *user_params)
{
	const char *uri = gst_discoverer_info_get_uri (info);
	GstDiscovererResult result = gst_discoverer_info_get_result (info);

	switch (result)
	{
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
		mtl_trace("This URI cannot be played: %d, uri: \"%s\"",
			result, uri);
		return;
	}
	
	/* If we got no error, show the retrieved information */
	u64 nano = gst_discoverer_info_get_duration(info);
	quince::audio::Info audio_info = {.duration = nano, .uri = uri};
	
	if (false)
	{
		const GstTagList *tags = gst_discoverer_info_get_tags (info);
		if (tags) {
			g_print ("Tags:\n");
			gst_tag_list_foreach (tags, print_tag_foreach, GINT_TO_POINTER (1));
		}
		
		g_print ("Seekable: %s\n", (gst_discoverer_info_get_seekable (info) ? "yes" : "no"));
	}
	
	if (true)
	{
		GstDiscovererStreamInfo *sinfo = gst_discoverer_info_get_stream_info (info);

		if (sinfo)
		{
//			print_topology (sinfo, 1);
			GList *audio_streams = gst_discoverer_info_get_audio_streams(info);
			for (auto *l = audio_streams; l != NULL; l = l->next)
			{
				auto *discov_audio_info = (GstDiscovererAudioInfo*)l->data;
				audio_info.bitrate = gst_discoverer_audio_info_get_bitrate(discov_audio_info);
				audio_info.channels =  gst_discoverer_audio_info_get_channels(discov_audio_info);
				audio_info.sample_rate = gst_discoverer_audio_info_get_sample_rate(discov_audio_info);
			}
			
			gst_discoverer_stream_info_unref (sinfo);
		}
	}
	
	// Now find the quince::Song with corresponding uri and discoverer,
	// then update the Song and the GUI.
	auto item = user_params->pending.find(user_params->discoverer);
	
	if (item != user_params->pending.end()) {
		std::vector<quince::Song*> &vec = item->second;
		
		for (int i = 0; i < vec.size(); i++) {
			quince::Song *song = vec[i];
			
			if (song->uri() == uri) {
				song->Apply(audio_info);
				auto *table_model = user_params->app->active_table_model();
				CHECK_PTR_VOID(table_model);
				table_model->UpdateRangeDefault(i);
				quince::gui::SeekPane *seek_pane = user_params->app->seek_pane();
				
				if (seek_pane->IsActive(song))
					seek_pane->SetCurrentOrUpdate(song);
			}
		}
	}
}

/* This function is called when the discoverer has finished examining
 * all the URIs we provided.*/
static void on_finished_cb (GstDiscoverer *discoverer,
	quince::DiscovererUserParams *user_params)
{
	 // cleanup map
		auto item = user_params->pending.find(user_params->discoverer);
		
		if (item != user_params->pending.end())
			user_params->pending.erase(item);
	
	gst_discoverer_stop (user_params->discoverer); // Stop the discoverer process
	g_object_unref (user_params->discoverer); // Free resources
	g_main_loop_quit (user_params->loop);
	g_main_loop_unref (user_params->loop);
	user_params->loop = nullptr;
}

static void HotkeyCallback(const QuinceGlobalHotkeysAction action)
{
	switch(action)
	{
	case QuinceGlobalHotkeysAction::None: break;
	case QuinceGlobalHotkeysAction::PlayPause: {
		mtl_info("play/pause");
		break;
	}
	case QuinceGlobalHotkeysAction::LowerVolume: {
		mtl_info("Lower volume");
		break;
	}
	case QuinceGlobalHotkeysAction::RaiseVolume: {
		mtl_info("Raise volume");
		break;
	}
	default: {
		mtl_info("Other action");
		break;
	}
	}
}

namespace quince {

App::App(int argc, char *argv[]) :
app_icon_(":/resources/Quince.png")
{
	setObjectName(quince_context_.unique);
	play_mode_ = audio::PlayMode::StopAtPlaylistEnd;
	player_ = new GstPlayer(this, argc, argv);
	CHECK_TRUE_VOID(InitDiscoverer());
	CHECK_TRUE_VOID(CreateGui());
	LoadPlaylists();
	setWindowIcon(app_icon_);
	
	if (!QSystemTrayIcon::isSystemTrayAvailable())
		mtl_info("System tray not available.");
	
	QIcon tray_icon(":/resources/Quince48.png");
	
	tray_icon_ = new QSystemTrayIcon(tray_icon, this);
	tray_icon_->setVisible(true);
	connect(tray_icon_, &QSystemTrayIcon::activated, this,
		&App::TrayActivated);
	
	RegisterWindowShortcuts();
	RegisterGlobalShortcuts();
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

QVector<Song*>*
App::active_playlist_songs()
{
	auto *model = active_table_model();
	
	if (model == nullptr)
		return nullptr;
	
	return &model->songs();
}

gui::TableModel*
App::active_table_model()
{
	if (active_playlist_ == nullptr)
	{
		int index = playlists_cb_->currentIndex();
		
		if (index == -1 || index >= playlists_.size())
			return nullptr;
		
		return playlists_[index]->table_model();
	}
	
	return active_playlist_->table_model();
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
		
		// Add an asynchronous request to process the URI
		auto uri_ba = song->uri().toLocal8Bit();
//		mtl_info("Adding request for %s", uri_ba.data());
		
		if (!gst_discoverer_discover_uri_async (user_params_.discoverer, uri_ba)) {
			mtl_warn("Failed to start discovering URI '%s'\n", uri_ba.data());
			g_object_unref (user_params_.discoverer);
			return false;
		}
		
		std::vector<quince::Song*> &vec = user_params_.pending[user_params_.discoverer];
		vec.push_back(song);
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
App::AddFolderTo(const io::File &dir, QVector<io::File> &only_files, const int level,
	const int max_levels)
{
	if (level >= max_levels)
		return;
	
	QVector<io::File> subfiles;
	QString dir_path = dir.build_full_path();
	
	if (io::ListFiles(dir_path, subfiles, 0, io::IsSongExtension) != io::Err::Ok) {
		auto ba = dir_path.toLocal8Bit();
		mtl_trace("dir path: %s", ba.data());
		return;
	}
	
	for (io::File &file: subfiles) {
		if (file.is_regular())
			only_files.append(file);
	}
	
	for (io::File &file: subfiles)
	{
		if (file.is_dir()) {
			AddFolderTo(file, only_files, level + 1, max_levels);
		}
	}
}

void
App::AddFilesToPlaylist(QVector<io::File> &files,
	gui::Playlist *playlist, const i32 at_vec_index)
{
//	mtl_info("Insert at %d", at_vec_index);
	CHECK_PTR_VOID(playlist);
	gui::TableModel *model = playlist->table_model();
	QVector<Song*> songs_to_add;
	QVector<io::File> only_files;
	
	for (io::File &file: files)
	{
		if (file.is_regular())
			only_files.append(file);
	}
	
	for (io::File &file: files)
	{
		if (file.is_dir()) {
			AddFolderTo(file, only_files, 0, 3);
		}
	}
	
	for (io::File &file: only_files)
	{
		auto *song = Song::FromFile(file, playlist->id());
		
		if (song != nullptr) {
			songs_to_add.append(song);
		} else {
			// mtl_warn("Song is null: \"%s\"", song_path.data());
		}
	}
	
	if (!songs_to_add.isEmpty())
	{
		AddBatch(songs_to_add);
		model->InsertRows(at_vec_index, songs_to_add);
		SavePlaylistSimple(playlist);
	}
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
		return;
	
	gui::Playlist *playlist = GetComboCurrentPlaylist(nullptr);
	
	if (playlist == nullptr)
	{
		if (playlists_.isEmpty()) {
			playlist = CreatePlaylist("New Playlist", true,
				PlaylistActivationOption::None, nullptr,
				gui::playlist::Ctor::AssignNewId);
		} else {
			mtl_warn("Can't get current playlist");
			return;
		}
	}
	
	const QStringList filenames = box.selectedFiles();
	struct stat st;
	QVector<io::File> files;
	
	for (auto next: filenames) {
		auto ba = next.toLocal8Bit();
		
		if (lstat(ba.data(), &st) == 0)
		{
			io::File file;
			
			if (io::FileFromPath(file, next) == io::Err::Ok)
				files.append(file);
		}
	}
	
	if (!files.isEmpty())
		AddFilesToPlaylist(files, playlist, 0);
	
	seek_pane_->ActivePlaylistChanged(playlist);
}

void
App::AskDeletePlaylist()
{
	int index;
	gui::Playlist *playlist = GetComboCurrentPlaylist(&index);
	CHECK_PTR_VOID(playlist);
	QString question = QString("Delete playlist <b>") +
		playlist->name() + QLatin1String("</b>?");
	QMessageBox::StandardButton reply =
		QMessageBox::question(this,
		"Confirm", question, QMessageBox::Ok | QMessageBox::Cancel);
	
	if (reply == QMessageBox::Ok) {
		DeletePlaylist(playlist, index);
	}
}

void
App::AskNewPlaylist()
{
	bool ok;
	
	QString text = QInputDialog::getText(this,
		"New Playlist", QLatin1String("Name:"),
		QLineEdit::Normal, "New Playlist", &ok);
	
	if (ok && !text.isEmpty())
		CreatePlaylist(text, true, PlaylistActivationOption::None,
			nullptr, gui::playlist::Ctor::AssignNewId);
}

void
App::AskRenamePlaylist()
{
	int index;
	auto *current = GetComboCurrentPlaylist(&index);
	CHECK_PTR_VOID(current);
	QString current_name = current->name();
	bool ok;
	
	QString new_name = QInputDialog::getText(this,
		"Rename Playlist",
		QLatin1String("New playlist name:"), QLineEdit::Normal,
		current_name, &ok);
	
	new_name = new_name.trimmed();
	
	if (!ok && new_name.isEmpty())
		return;
	
	if (new_name == current_name)
		return;
	
	for (gui::Playlist *p : playlists_)
	{
		if (p->name() == new_name) {
			QMessageBox msgBox;
			msgBox.setText("A playlist with this name already exists.");
			msgBox.exec();
			return;
		}
	}
	
	current->name(new_name);
	CHECK_TRUE_VOID(SavePlaylistSimple(current));
	playlists_cb_->setItemText(index, new_name);
}

void
App::closeEvent(QCloseEvent *event)
{
	//QApplication::quit();
	setVisible(false);
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

	playlist_stack_widget_ = new gui::PlaylistStackWidget(central_widget, this);
	playlist_stack_ = new QStackedLayout();
	playlist_stack_widget_->setLayout(playlist_stack_);
	layout->addWidget(playlist_stack_widget_);
	
	addToolBar(Qt::BottomToolBarArea, CreatePlaylistActionsToolBar());
	
	return true;
}

QToolBar*
App::CreateMediaActionsToolBar()
{
	QToolBar *tb = new QToolBar(this);
	AddAction(tb, "go-previous", actions::MediaPlayPrev);
	RegisterGlobalShortcut(actions::MediaPlayPrev, Qt::Key_MediaPrevious);
	play_pause_action_ = AddAction(tb, ICON_NAME_PLAY,
		actions::MediaPlayPause);
	
	AddAction(tb, "media-playback-stop", actions::MediaPlayStop);
	RegisterGlobalShortcut(actions::MediaPlayStop, Qt::Key_MediaStop);
	
	AddAction(tb, "go-next", actions::MediaPlayNext);
	RegisterGlobalShortcut(actions::MediaPlayNext, Qt::Key_MediaNext);
	
	QWidget *space = new QWidget();
	space->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	space->setVisible(true);
	tb->addWidget(space);
	
	auto *song_entries_label = new QLabel(QLatin1String("Song Entries: "));
	tb->addWidget(song_entries_label);
	
	auto *w = AddAction(tb, "list-add", actions::AddSongFilesToPlaylist);
	w->setToolTip("Add song files and folders to playlist");
	
	w = AddAction(tb, "list-remove", actions::RemoveSongsFromPlaylist);
	w->setToolTip("Remove song from playlist");
	
	AddAction(tb, "edit-clear", actions::PlaylistRemoveAllEntries, "Remove all songs");
	
	auto *label = new QLabel("   ");
	tb->addWidget(label);
	tb->addSeparator();
	w = AddAction(tb, "application-exit", actions::QuitApp);
	w->setToolTip("Quit Application");
	
	return tb;
}

gui::Playlist*
App::CreatePlaylist(const QString &name, const bool set_active,
	const PlaylistActivationOption activation_option, int *index,
	gui::playlist::Ctor ctor)
{
	for (gui::Playlist *p: playlists_)
	{
		if (p->name() == name) {
			auto ba = p->name().toLocal8Bit();
			mtl_trace("Same playlist name: \"%s\"", ba.data());
			return nullptr;
		}
	}
	
	gui::Playlist *playlist = new gui::Playlist(this, name);
	
	if (ctor == gui::playlist::Ctor::AssignNewId) {
		playlist->id(GenNewPlaylistId());
//		mtl_info("Assigned new playlist id: %ld", playlist->id());
	}
	
	int n = playlist_stack_->addWidget(playlist);
	playlists_cb_->addItem(name);
	playlists_.append(playlist);
	
	if (index != nullptr)
		*index = n;
	
	if (playlist->id() != -1)
		SavePlaylistSimple(playlist);
	
	if (set_active)
		SetActive(playlist, activation_option);
	
	return playlist;
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
	playlists_cb_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	connect(playlists_cb_,
		QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &App::PlaylistComboIndexChanged);
	tb->addWidget(playlists_cb_); // takes ownership
	
	playlist_duration_ = new QLabel();
	tb->addWidget(playlist_duration_);
	
	return tb;
}

gui::Playlist*
App::GetComboCurrentPlaylist(int *pindex)
{
	int index = playlists_cb_->currentIndex();
	
	if ((index >= 0) && (index < playlists_.size()))
	{
		if (pindex != nullptr)
			*pindex = index;
		
		return playlists_[index];
	}

	if (pindex != nullptr)
		*pindex = -1;
	
	return nullptr;
}

bool
App::DeletePlaylist(gui::Playlist *p, int index)
{
	CHECK_PTR(p);
	const bool was_active = (p == active_playlist_);
	QString full_path;
	CHECK_TRUE(p->GetFullPath(full_path));
	auto ba = full_path.toLocal8Bit();
	int ret = remove(ba.data());
	
	if (ret != 0) {
		mtl_warn("Failed to delete file \"%s\"", ba.data());
		return false;
	}

	playlists_cb_->removeItem(index);
	playlists_.removeAt(index);
	playlist_stack_->removeWidget(p);
	delete p;
	index = playlists_cb_->currentIndex();
	
	if (was_active)
	{
		if ((index >= 0) && (index < playlists_.size())) {
			active_playlist_ = playlists_[index];
		} else {
			active_playlist_ = nullptr;
		}
	}
	
	return true;
}

i64
App::GenNewPlaylistId() const
{
	i64 greatest = -1;
	
	for (gui::Playlist *p : playlists_) {
		if (p->id() >= greatest)
			greatest = p->id();
	}
	
	return greatest + 1;
}

Song*
App::GetCurrentSong(int *index)
{
	auto *songs = active_playlist_songs();
	CHECK_PTR_NULL(songs);
	
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
	auto *songs = active_playlist_songs();
	
	if (songs == nullptr || songs->empty())
		return nullptr;
	
	return (*songs)[0];
}

Song*
App::GetFirstSongInVisiblePlaylist()
{
	auto *playlist = GetVisiblePlaylist();
	CHECK_PTR_NULL(playlist);
	auto &songs = playlist->songs();
	
	if (songs.isEmpty())
		return nullptr;
	
	return songs[0];
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

gui::Playlist*
App::GetPlaylistById(const i64 playlist_id, int *pindex) const
{
	const int count = playlists_.size();
	
	for (int i = 0; i < count; i++)
	{
		auto *p = playlists_[i];
		
		if (p->id() == playlist_id) {
			if (pindex != nullptr)
				*pindex = i;
			
			return p;
		}
	}
	
	return nullptr;
}

bool
App::SongAndPlaylistMatch(const audio::TempSongInfo &tsi) const
{
	gui::Playlist *playlist = GetPlaylistById(tsi.playlist_id);
	
	if (playlist == nullptr)
		return false;
	
	return playlist->HasSong(tsi.song);
}

gui::Playlist*
App::GetVisiblePlaylist()
{
	int index = playlists_cb_->currentIndex();
	
	if (index == -1 || index >= playlists_.size())
		return nullptr;
	
	return playlists_[index];
}

Song*
App::GetVisiblePlaylistCurrentSong(int *pindex)
{
	gui::Playlist *playlist = GetVisiblePlaylist();
	CHECK_PTR_NULL(playlist);
//	auto ba = playlist->name().toLocal8Bit();
//	mtl_info("Visilbe playlist: %s", ba.data());
	
	auto *model = playlist->table_model();
	QVector<Song*> &songs = model->songs();
	const int count = songs.size();
	
	for (int i = 0; i < count; i++) {
		Song *song = songs[i];
		
		if (song->is_playing_or_paused())
		{
			if (pindex != nullptr)
				*pindex = i;
			
			return song;
		}
	}
	
	return nullptr;
}

bool
App::InitDiscoverer()
{
	user_params_.app = this;
	
	// Instantiate the Discoverer
	GError *err = NULL;
	user_params_.discoverer = gst_discoverer_new (30 * GST_SECOND, &err);
	
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
//		auto ba = full_path.toLocal8Bit();
//		mtl_info("Playlist cache version %d not supported, need %d, file:\n%s",
//			cache_version, quince::PlaylistCacheVersion, ba.data());
		return;
	}
	
	i64 id = ba.next_i64();
	QString playlist_name = ba.next_string();
	const bool is_active = ba.next_u8() == 1;
	auto *playlist = CreatePlaylist(playlist_name, false,
		PlaylistActivationOption::None, nullptr,
		gui::playlist::Ctor::None);
	CHECK_PTR_VOID(playlist);
	playlist->id(id);
	auto &songs = playlist->songs();
	const i32 song_count = ba.next_i32();
	QVector<Song*> songs_to_add;
	
	for (i32 i = 0; i < song_count; i++)
	{
		Song *song = Song::From(ba, playlist->id());
		
		if (song != nullptr)
			songs_to_add.append(song);
	}
	
	gui::TableModel *model = playlist->table_model();
	model->InsertRows(songs.size(), songs_to_add);
	
	if (is_active)
		SetActive(playlist, PlaylistActivationOption::RestoreStreamPosition);
}

void
App::LoadPlaylists()
{
	QString dir_path;
	CHECK_TRUE_VOID(gui::Playlist::QuerySaveFolder(dir_path));
	
	if (!dir_path.endsWith('/'))
		dir_path.append('/');
	
	QVector<QString> names;
	
	if (io::ListFileNames(dir_path, names) != io::Err::Ok)
		return;
	
	for (const QString &filename: names)
		LoadPlaylist(dir_path + filename);
	
	{ // this fixes gui/playback glitches
		// not sure if it's still the case
		if (playlists_.size() == 1 && active_playlist_ != playlists_[0])
			SetActive(playlists_[0], PlaylistActivationOption::RestoreStreamPosition);
	}
}

void
App::MediaPause()
{
	int song_index = -1;
	Song *song = GetCurrentSong(&song_index);
	player_->Pause(song);
	
	if (song_index != -1)
		active_table_model()->UpdateRangeDefault(song_index);
}

void
App::MediaPlay()
{
	int song_index = -1;
	Song *song = GetVisiblePlaylistCurrentSong(&song_index);
	
	if (song == nullptr)
	{
		if (last_play_state_ == GST_STATE_PAUSED) {
			player_->Play(nullptr);
			return;
		}
		song = GetFirstSongInVisiblePlaylist();
		CHECK_PTR_VOID(song);
		song_index = 0;
	}
	
//	auto ba = song->display_name().toLocal8Bit();
//	mtl_trace("%s", ba.data());
	player_->Play(song);
	
	if (song_index != -1)
		active_table_model()->UpdateRangeDefault(song_index);
}

void
App::MediaPlayPause()
{
	if (last_play_state_ == GST_STATE_NULL || last_play_state_ == GST_STATE_PAUSED)
		MediaPlay();
	else
		MediaPause();
}

void
App::MessageAsyncDone()
{
//	int row_index;
//	Song *song = GetCurrentSong(&row_index);
//	CHECK_PTR_VOID(song);
//	GstState state;
//	GstStateChangeReturn ret = gst_element_get_state (play_elem(),
//		&state, NULL, GST_CLOCK_TIME_NONE);
	
//	if (ret != GST_STATE_CHANGE_SUCCESS) {
//		mtl_trace();
//		return;
//	}
	
//	//mtl_info("State is: %s", audio::StateToString(state));
	
//	if (!song->meta().is_duration_set())
//	{
//		i64 duration = -1;
//		gboolean ok = gst_element_query_duration (play_elem(),
//			GST_FORMAT_TIME, &duration);
//		CHECK_TRUE_VOID(ok);
//		song->meta().duration(duration);
//		mtl_info("DURATION IS: %ld", duration);
		
//		auto *table_model = active_table_model();
//		CHECK_PTR_VOID(table_model);
//		table_model->UpdateRangeDefault(row_index);
		
//		if (seek_pane_->current_song() == song)
//			seek_pane_->SetCurrentOrUpdateSong(song);
//	}
}

gui::Playlist*
App::PickPlaylist(const i64 id, int *pindex)
{
	const int count = playlists_.size();
	
	for (int i = 0; i < count; i++)
	{
		gui::Playlist *p = playlists_[i];
		
		if (p->id() == id) {
			if (pindex != nullptr)
				*pindex = i;
			
			return p;
		}
	}
	
	return nullptr;
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
			last_song->position(-1);
			last_song->state(GST_STATE_NULL);
			active_table_model()->UpdateRangeDefault(current_song_index);
			return current_song_index + 1;
		}
	} else if (pick == audio::Pick::Prev) {
		
		if (current_song_index == -1) {
			auto *model = active_table_model();
			
			if (model == nullptr)
				return -1;
			
			QVector<Song*> &vec = model->songs();
			return vec.isEmpty() ? -1 : vec.size() - 1;
		} else if (current_song_index == 0) {
			return -1;
		} else {
			auto *last_song = (*vec)[current_song_index];
			last_song->position(-1);
			last_song->state(GST_STATE_NULL);
			active_table_model()->UpdateRangeDefault(current_song_index);
			return current_song_index - 1;
		}
	} else {
		mtl_warn();
		return -1;
	}
}

GstElement*
App::play_elem() const { return player_->play_elem(); }

void
App::PlaylistComboIndexChanged(int index)
{
	if (index != -1 && index < playlists_.size())
		SetActive(playlists_[index], PlaylistActivationOption::None);
}

void
App::PlaySong(const audio::Pick direction)
{
	auto *vec = active_playlist_songs();
	
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
	player_->Play(playing_song);
	active_table_model()->UpdateRangeDefault(song_index);
}

void
App::PlayStop()
{
	audio::TempSongInfo &tsi = player_->temp_song_info();
	Song *song = tsi.song;
	player_->StopPlaying(song);
	
	if (song != nullptr) {
		int index;
		gui::Playlist *p = GetPlaylistById(tsi.playlist_id, &index);
		CHECK_PTR_VOID(p);
		active_table_model()->UpdateRangeDefault(index);
	}
	
	UpdatePlayIcon(GST_STATE_NULL);
}

void
App::ProcessAction(const QString &action_name)
{
//	auto ba = action_name.toLocal8Bit();
//	mtl_info("Incoming action: %s", ba.data());
	
	if (action_name == actions::MediaPlayPause)
	{
		MediaPlayPause();
	} else if (action_name == actions::MediaPlayNext) {
		PlaySong(audio::Pick::Next);
	} else if (action_name == actions::MediaPlayPrev) {
		PlaySong(audio::Pick::Prev);
	} else if (action_name == actions::MediaPlayStop) {
		PlayStop();
	} else if (action_name == actions::AddSongFilesToPlaylist) {
		AskAddSongFilesToPlaylist();
	} else if (action_name == actions::RemoveSongsFromPlaylist) {
		RemoveSongsFromPlaylist(Which::Selected);
	} else if (action_name == actions::PlaylistNew) {
		AskNewPlaylist();
	} else if (action_name == actions::QuitApp) {
		QApplication::quit();
	} else if (action_name == actions::PlaylistRename) {
		AskRenamePlaylist();
	} else if (action_name == actions::PlaylistDelete) {
		AskDeletePlaylist();
	} else if (action_name == actions::PlaylistRemoveAllEntries) {
		RemoveSongsFromPlaylist(Which::All);
	} else {
		auto ba = action_name.toLocal8Bit();
		mtl_trace("Action skipped: \"%s\"", ba.data());
	}
}

bool
App::QueryAppConfigPath(QString &path)
{
	static QString dir_path = QString();
	
	if (!dir_path.isEmpty())
	{
		path = dir_path;
		return true;
	}
	
	QString config_path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
	
	if (!config_path.endsWith('/'))
		config_path.append('/');
	
	CHECK_TRUE(io::EnsureDir(config_path, AppConfigName));
	dir_path = config_path + AppConfigName;
	path = dir_path;
	
	return true;
}

void
App::ReachedEndOfStream()
{
	auto *vec = active_playlist_songs();
	
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
			song->position(-1);
			last_playing = i;
			break;
		}
	}
	
	if (last_playing == -1 || play_mode_ == audio::PlayMode::StopAtTrackEnd)
		return;
	
	active_table_model()->UpdateRangeDefault(last_playing);
	
	if (play_mode_ == audio::PlayMode::RepeatPlaylist ||
		play_mode_ == audio::PlayMode::StopAtPlaylistEnd) {
		PlaySong(audio::Pick::Next);
	} else if (play_mode_ == audio::PlayMode::RepeatTrack) {
		auto *song = (*vec)[last_playing];
		player_->Play(song);
		active_table_model()->UpdateRangeDefault(last_playing);
	}
}

void
App::RegisterGlobalShortcut(const QString &action_name,
	const QKeySequence &key_sequence, QIcon *icon)
{
/*
The action must have a per main component unique action->objectName()
 to enable cross-application bookeeping. If the action->objectName()
 is empty this method will do nothing and will return false.

It is mandatory that the action->objectName() doesn't change once
 the shortcut has been sucessfully registered.

When an action, identified by main component name and objectName(),
 is assigned a global shortcut for the first time on a KDE
 installation the assignment will be saved. The shortcut will
 then be restored every time setGlobalShortcut() is called with
 loading == Autoloading. */
	QAction *action = new QAction(action_name, this);
	
	if (icon != nullptr)
		action->setIcon(*icon);
	
	action->setProperty("componentName", quince_context_.unique);
	action->setProperty("componentDisplayName", quince_context_.friendly);
	
	action->setObjectName(quince_context_.unique + action_name);
	connect(action, &QAction::triggered, [=] {ProcessAction(action_name);});
	
	if (!KGlobalAccel::isGlobalShortcutAvailable(key_sequence))
	{
		/*
			mtl_info("Global shortcut already registered:");
			auto list = KGlobalAccel::getGlobalShortcutsByKey(key_sequence);
			
			for (KGlobalShortcutInfo &item : list) {
				qDebug() << "Component F(riendly)name: " << item.componentFriendlyName();
				qDebug() << "Component U(nique)name: " << item.componentUniqueName();
				qDebug() << "Context Fname: " << item.contextFriendlyName();
				qDebug() << "Context Uname: " << item.contextUniqueName();
				qDebug() << "Fname: " << item.friendlyName();
				qDebug() << "Uname: " << item.uniqueName() << "\n";
			}
		*/
	}
	
	KGlobalAccel *ga = KGlobalAccel::self();

	if (!ga->setGlobalShortcut(action, key_sequence)) {
		auto ba = key_sequence.toString().toLocal8Bit();
		mtl_warn("Failed to set shortcut [%s]", ba.data());
	}
}

void
App::RegisterGlobalShortcuts()
{
/*	Qt::Key_VolumeDown
	Qt::Key_VolumeMute
	Qt::Key_VolumeUp
	Qt::Key_MediaPlay
	Qt::Key_MediaStop
	Qt::Key_MediaPrevious
	Qt::Key_MediaNext
	Qt::Key_MediaRecord
	Qt::Key_MediaPause
	Qt::Key_MediaTogglePlayPause
	Qt::Key_AudioRewind
	Qt::Key_AudioForward
	Qt::Key_AudioRepeat
	Qt::Key_AudioRandomPlay
	Qt::Key_Subtitle
	Qt::Key_AudioCycleTrack
	Qt::Key_MicVolumeUp
	Qt::Key_MicVolumeDown
	Qt::Key_MediaLast
	Qt::Key_Play */

// componentName (e.g. "kwin") and actionId (e.g. "Kill Window").
	
	QDBusConnectionInterface *bus = QDBusConnection::sessionBus().interface();
	const auto ServiceName = QLatin1String("org.kde.kglobalaccel");
	
	if (!bus->isServiceRegistered(ServiceName)) {
		QDBusReply<void> reply = bus->startService(ServiceName);
		CHECK_TRUE_VOID(reply.isValid());
	}
	
	//QKeySequence play_pause_sequence(Qt::Key_MediaPlay);
	RegisterGlobalShortcut(actions::MediaPlayPause, Qt::Key_MediaPlay);
	RegisterGlobalShortcut(actions::MediaPlayPrev, Qt::Key_MediaPrevious);
	RegisterGlobalShortcut(actions::MediaPlayNext, Qt::Key_MediaNext);
	RegisterGlobalShortcut(actions::MediaPlayStop, Qt::Key_MediaStop);
}

void
App::RegisterWindowShortcuts()
{
	auto *shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_A), this);
	shortcut->setContext(Qt::ApplicationShortcut);
	
	connect(shortcut, &QShortcut::activated, [=] {
		SelectAllSongsInVisiblePlaylist();
	});
	
	shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	shortcut->setContext(Qt::ApplicationShortcut);
	
	connect(shortcut, &QShortcut::activated, [=] {
		QApplication::quit();
	});
}

void
App::RemoveSongsFromPlaylist(const Which which)
{
	gui::Playlist *playlist = GetComboCurrentPlaylist(nullptr);
	CHECK_PTR_VOID(playlist);
	i32 count = -1;
	
	switch (which) {
	case Which::All: { count = playlist->RemoveAllSongs(); break; }
	case Which::Selected: { count = playlist->RemoveSelectedSongs(); break; }
	default: {
		mtl_trace();
		return;
	}
	}
	
	if (count > 0) {
		UpdatePlaylistDuration(playlist);
		SavePlaylistSimple(playlist);
	}
}

bool
App::SavePlaylist(gui::Playlist *playlist, const QString &dir_path,
const bool is_active)
{
	CHECK_PTR(playlist);
	quince::ByteArray ba;
	ba.add_i32(PlaylistCacheVersion);
	ba.add_i64(playlist->id());
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
App::SavePlaylistSimple(gui::Playlist *playlist)
{
	const bool is_active = playlist == active_playlist_;
	QString dir_path;
	CHECK_TRUE(gui::Playlist::QuerySaveFolder(dir_path));
	
	return SavePlaylist(playlist, dir_path, is_active);
}

void
App::SavePlaylistState(const i64 id)
{
	gui::Playlist *playlist = nullptr;
	
	for (auto *p : playlists_) {
		if (p->id() == id) {
			playlist = p;
			break;
		}
	}
	
	CHECK_PTR_VOID(playlist);
	Song *song = playlist->GetCurrentSong(nullptr);
	
	if (song != nullptr)
	{
		if (song->is_playing())
			song->state(GST_STATE_PAUSED);
	}
	
}

bool
App::SavePlaylistsToDisk()
{
	QString path;
	CHECK_TRUE(gui::Playlist::QuerySaveFolder(path));
	bool ok = true;
	
	for (gui::Playlist *p : playlists_)
	{
		if (!SavePlaylist(p, path, p == active_playlist_))
			ok = false; // yet try to save other playlists
	}
	
	return ok;
}

void
App::SelectAllSongsInVisiblePlaylist()
{
	auto *playlist = GetVisiblePlaylist();
	CHECK_PTR_VOID(playlist);
	playlist->table()->selectAll();
	playlist->table()->setFocus();
}

void
App::SetActive(gui::Playlist *playlist, const PlaylistActivationOption option)
{
	static bool executing = false;
	// The method playlists_cb_->setCurrentIndex(index)
	// triggers this method again, workaround:
	if (executing)
		return;
	
	executing = true;
//	auto ba = playlist->name().toLocal8Bit();
//	mtl_info("set active(%s): %d", ba.data(), int(option));
	active_playlist_ = playlist;
	static i64 last_playlist_id_ = -1;
	
	if (last_playlist_id_ != -1)
		SavePlaylistState(last_playlist_id_);
	
	seek_pane_->ActivePlaylistChanged(playlist);
	const int index = GetIndex(playlist);
	playlist_stack_->setCurrentIndex(index);
	playlists_cb_->setCurrentIndex(index);
	
	if (option == PlaylistActivationOption::RestoreStreamPosition) {
		Song *song = playlist->GetCurrentSong(nullptr);
		
		if (song != nullptr)
			player_->SetSeekAndPause_Start(song, nullptr);
	}
	
	last_playlist_id_ = playlist->id();
	executing = false;
}

void
App::TrayActivated(QSystemTrayIcon::ActivationReason reason)
{
	// not using do_show = !isVisible() because it returns true even
	// when the window is not visible.
	static bool do_show = false;
	//mtl_info("do_show: %s", do_show ? "true" : "false");
	setVisible(do_show);
	
	if (do_show)
	{
		activateWindow();
		raise();
	}
	
	do_show = !do_show;
}

void
App::UpdatePlayIcon(const GstState new_state)
{
	const char *icon_name;
	
	if (new_state == GST_STATE_NULL || new_state == GST_STATE_PLAYING) {
		icon_name = ICON_NAME_PLAY;
	} else {
		icon_name = ICON_NAME_PAUSED;
	}
	
	play_pause_action_->setIcon(QIcon::fromTheme(icon_name));
}

void
App::UpdatePlayingSongPosition(const i64 pos_is_known)
{
	audio::TempSongInfo &tsi = player_->temp_song_info();
	
	if (!tsi.has_data())
		return;
	
	if (pos_is_known != -1) {
		tsi.position = pos_is_known;
	} else if (!gst_element_query_position (play_elem(), GST_FORMAT_TIME,
		&tsi.position))
	{
		return;
	}
	
	if (SongAndPlaylistMatch(tsi)) {
		tsi.song->position(tsi.position);
		seek_pane_->UpdatePosition(tsi.position);
	}
}

void
App::UpdatePlaylistDuration(gui::Playlist *playlist)
{
	CHECK_PTR_VOID(playlist);
	QVector<Song*> &songs = playlist->table_model()->songs();
	i64 total = 0;
	i32 song_count = 0;
	
	for (Song *song: songs)
	{
		audio::Meta &meta = song->meta();
		
		if (meta.is_duration_set())
		{
			total += meta.duration();
			song_count++;
		}
	}
	
	Duration d = Duration::FromNs(total);
	QString s = QString(('[')).append(QString::number(song_count))
	.append(" tracks, ")
	.append(d.toDurationString())
	.append(']');
	playlist_duration_->setText(s);
}

} // quince::
