#pragma once

#include "audio.hh"
#include "decl.hxx"
#include "err.hpp"
#include "gui/decl.hxx"
#include "gui/playlist.hxx"
#include "io/io.hh"
#include "types.hxx"

#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QStackedLayout>
#include <QSystemTrayIcon>
#include <QVector>

#include <kglobalaccel.h>

namespace quince {

static const i32 PlaylistCacheVersion = 3;
static const QString AppConfigName = QLatin1String("QuincePlayer");

struct DiscovererUserParams {
	GstDiscoverer *discoverer;
	GMainLoop *loop;
	quince::App *app;
	std::unordered_map<GstDiscoverer*, std::vector<Song*>> pending;
};

enum class Which : u8 {
	All,
	Selected
};

class App : public QMainWindow {
	
public:
	App(int argc, char *argv[]);
	virtual ~App();
	
	gui::Playlist* active_playlist() const { return active_playlist_; }
	QVector<Song*>* active_playlist_songs();
	gui::TableModel* active_table_model();
	bool AddBatch(QVector<quince::Song*> &vec);
	void AddFilesToPlaylist(QVector<io::File> &files, gui::Playlist *playlist, i32 at_vec_index);
	gui::Playlist* CreatePlaylist(const QString &name, const bool set_active,
		const PlaylistActivationOption activation_option,
		int *index, gui::playlist::Ctor ctor, QString *error_msg = nullptr);
	Desktop
	desktop() const { return desktop_; }
	
	bool
	desktop_gnome() const { return desktop_ == Desktop::Gnome; }
	
	bool
	desktop_kde() const { return desktop_ == Desktop::KDE; }
	
	void DetectDesktop();
	
	gui::Playlist* GetComboCurrentPlaylist(int *pindex = nullptr);
	Song* GetCurrentSong(int *index = nullptr);
	Song* GetFirstSongInCurrentPlaylist();
	Song* GetFirstSongInVisiblePlaylist();
	int GetIndex(gui::Playlist *playlist) const;
	gui::Playlist* GetVisiblePlaylist(int *ret_index = nullptr);
	Song* GetVisiblePlaylistCurrentSong(int *pindex);
	bool InitDiscoverer();
	
	bool visible() const { return must_be_visible_; }
	void visible(const bool flag) { must_be_visible_ = flag; }
	
	void last_play_state(GstState s) { last_play_state_ = s; }
	void MediaPause();
	void MediaPlay();
	void MediaPlayPause();
	void MessageAsyncDone();
	gui::Playlist* PickPlaylist(const i64 id, int *pindex = nullptr);
	void PlaylistComboIndexChanged(int index);
	GstElement* play_elem() const;
	GstPlayer* player() const { return player_; }
	Song *PlaySong(const audio::Pick direction);
	void PlayStop();
	static bool QueryAppConfigPath(QString &path);
	void ReachedEndOfStream();
	void RemoveSongsFromPlaylist(const Which which);
	bool SavePlaylistsToDisk();
	void SetActive(gui::Playlist *playlist, const PlaylistActivationOption option);
	gui::SeekPane* seek_pane() const { return seek_pane_; }
	void TrayActivated();
	void UpdatePlayIcon(const GstState new_state);
	void UpdatePlaylistDuration(gui::Playlist *playlist);
	void UpdatePlayingSongPosition(const i64 new_pos, const bool update_gui);
	void UpdatePlaylistsVisibility(const int index);
protected:
	void closeEvent(QCloseEvent *event);
	
private:
	
	QAction*
	AddAction(QToolBar *tb, const QString &icon_name,
		const QString &action_name, const char *tooltip = nullptr);
	
	void AddFolderTo(const io::File &dir, QVector<io::File> &only_files,
		const int level, const int max_levels);
	void AskAddSongFilesToPlaylist();
	void AskDeletePlaylist();
	void AskNewPlaylist();
	void AskRenamePlaylist();
	bool CreateGui();
	QToolBar* CreateMediaActionsToolBar();
	QToolBar* CreatePlaylistActionsToolBar();
	QTabBar* CreateTabBar();
	bool DeletePlaylist(gui::Playlist *p, int index);
	i64 GenNewPlaylistId() const;
	gui::Playlist* GetPlaylistById(const i64 playlist_id, int *pindex = nullptr) const;
	void InitTrayIcon();
	bool SongAndPlaylistMatch(const audio::TempSongInfo &tsi) const;
	bool LoadPlaylist(const QString &full_path);
	void LoadPlaylists();
	int PickSong(QVector<Song*> *vec, const int current_song_index,
		const audio::Pick pick);
	
	void ProcessAction(const QString &action_name);
	void RegisterGlobalShortcut(const QString &action_name,
		const QKeySequence &key_sequence, QIcon *icon = nullptr);
	
	void RegisterGlobalShortcuts();
	void RegisterWindowShortcuts();
	bool SavePlaylist(gui::Playlist *playlist, const QString &dir_path, const bool is_active);
	bool SavePlaylistSimple(gui::Playlist *playlist);
	void SavePlaylistState(const i64 id);
	void SelectAllSongsInVisiblePlaylist();
	
	NO_ASSIGN_COPY_MOVE(App);
	
	gui::SeekPane *seek_pane_ = nullptr;
	GstPlayer *player_ = nullptr;
	DiscovererUserParams user_params_ = {nullptr, nullptr, nullptr};
	QAction *play_pause_action_ = nullptr;
	audio::PlayMode play_mode_ = audio::PlayMode::None;
	QComboBox *playlists_cb_ = nullptr;
	QVector<gui::Playlist*> playlists_;
	gui::Playlist *active_playlist_ = nullptr;
	gui::PlaylistStackWidget *playlist_stack_widget_ = nullptr;
	QStackedLayout *playlist_stack_ = nullptr;
	QIcon app_icon_;
	QSystemTrayIcon *sys_tray_icon_ = nullptr;
	GstState last_play_state_ = GST_STATE_NULL;
	QLabel *playlist_duration_ = nullptr;
	
	struct QuinceContext {
		QString unique;
		QString friendly;
		QString program_name;
	} quince_context_ {
		.unique = "QuincePlayer",
		.friendly = "Quince Player",
		.program_name = "QuinceProgramName"
	};
	
	quince::Desktop desktop_ = quince::Desktop::None;
	
	bool must_be_visible_ = true;
};

}
