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
	void AddFilesToPlaylist(QVector<io::File> &files, gui::Playlist *playlist);
	gui::Playlist* CreatePlaylist(const QString &name, const bool set_active,
		const PlaylistActivationOption activation_option,
		int *index, gui::playlist::Ctor ctor);
	gui::Playlist* GetComboCurrentPlaylist(int *pindex = nullptr);
	Song* GetCurrentSong(int *index = nullptr);
	Song* GetFirstSongInCurrentPlaylist();
	Song* GetFirstSongInVisiblePlaylist();
	int GetIndex(gui::Playlist *playlist) const;
	gui::Playlist* GetVisiblePlaylist();
	Song* GetVisiblePlaylistCurrentSong(int *pindex);
	bool InitDiscoverer();
	void last_play_state(GstState s) { last_play_state_ = s; }
	void MediaPause();
	void MediaPlay();
	void MediaPlayPause();
	void MessageAsyncDone();
	gui::Playlist* PickPlaylist(const i64 id, int *pindex = nullptr);
	void PlaylistComboIndexChanged(int index);
	GstElement* play_elem() const;
	GstPlayer* player() const { return player_; }
	void PlaySong(const audio::Pick direction);
	void PlayStop();
	static bool QueryAppConfigPath(QString &path);
	void ReachedEndOfStream();
	void RemoveSongsFromPlaylist(const Which which);
	bool SavePlaylistsToDisk();
	void SetActive(gui::Playlist *playlist, const PlaylistActivationOption option);
	gui::SeekPane* seek_pane() const { return seek_pane_; }
	void TrayActivated(QSystemTrayIcon::ActivationReason reason);
	void UpdatePlayIcon(const GstState new_state);
	void UpdatePlaylistDuration(gui::Playlist *playlist);
	void UpdatePlayingSongPosition(const i64 pos_is_known);
	
protected:
	void closeEvent(QCloseEvent *event);
	
private:
	
	QAction*
	AddAction(QToolBar *tb, const QString &icon_name,
		const QString &action_name, const char *tooltip = nullptr);
	
	void AddFolderToPlaylist(const QString &dp, gui::Playlist *playlist);
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
	bool SongAndPlaylistMatch(const audio::TempSongInfo &tsi) const;
	void LoadPlaylist(const QString &full_path);
	void LoadPlaylists();
	int PickSong(QVector<Song*> *vec, const int current_song_index,
		const audio::Pick pick);
	
	void ProcessAction(const QString &action_name);
	void RegisterGlobalShortcut(const QString &action_name,
		const QKeySequence &key_sequence, QIcon *icon = nullptr);
	
	void RegisterGlobalShortcuts();
	bool SavePlaylist(gui::Playlist *playlist, const QString &dir_path, const bool is_active);
	bool SavePlaylistSimple(gui::Playlist *playlist);
	void SavePlaylistState(const i64 id);
	
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
	QSystemTrayIcon *tray_icon_ = nullptr;
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
};

}
