#pragma once

#include "audio.hxx"
#include "decl.hxx"
#include "err.hpp"
#include "gui/decl.hxx"
#include "io/io.hh"
#include "types.hxx"

#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

#include <QComboBox>
#include <QMainWindow>
#include <QStackedLayout>
#include <QVector>

namespace quince {

struct DiscovererUserParams {
	GstDiscoverer *discoverer;
	GMainLoop *loop;
	quince::App *app;
};

struct AudioInfo {
	u64 duration;
	QByteArray uri;
};

class App : public QMainWindow {
	
public:
	App(int argc, char *argv[]);
	virtual ~App();
	
	i32
	active_playlist_index() const;
	
	bool AddBatch(QVector<quince::Song*> &vec);
	
	gui::TableModel*
	current_table_model();
	
	QVector<Song*>*
	current_playlist_songs();
	
	int
	GetIndex(gui::Playlist *playlist) const;
	
	void
	GotAudioInfo(AudioInfo *info);
	
	gui::Playlist*
	GetActivePlaylist();
	
	Song*
	GetCurrentSong(int *index = nullptr);
	
	Song*
	GetFirstSongInCurrentPlaylist();
	
	bool
	InitDiscoverer();
	
	void LoadSavedSongData(gui::Playlist *playlist);
	
	void
	MessageAsyncDone();
	
	void
	PlaylistComboIndexChanged(int index);
	
	void
	PlaylistDoubleClicked(QModelIndex index);
	
	GstElement*
	play_elem() const;
	
	GstPlayer*
	player() const { return player_; }
	
	void
	PlaySong(const audio::Pick direction);
	
	void
	PlayStop();
	
	void
	ReachedEndOfStream();
	
	void
	SetActive(gui::Playlist *playlist);
	
	gui::SliderPane*
	slider_pane() const { return seek_pane_; }
	
	void
	UpdatePlayIcon(Song *song);
	
	gui::UpdateTableRange
	UpdatePlayingSongPosition(const i64 pos_is_known);
	
private:
	
	QAction*
	AddAction(QToolBar *tb, const QString &icon_name,
		const QString &action_name, const char *tooltip = nullptr);
	
	void AddFilesToPlaylist(QVector<io::File> &files, gui::Playlist *playlist);
	void AddFolderToPlaylist(const QString &dp, gui::Playlist *playlist);
	void AskAddSongFilesToPlaylist();
	
	bool CreateGui();
	QToolBar* CreateMediaActionsToolBar();
	QToolBar* CreatePlaylistActionsToolBar();
	QTabBar* CreateTabBar();
	gui::Playlist* CreatePlaylist(const QString &name, int *index = nullptr);
	int PickSong(QVector<Song*> *vec, const int current_song_index,
		const audio::Pick pick);
	
	void ProcessAction(const QString &action_name);
	
	NO_ASSIGN_COPY_MOVE(App);
	
	gui::SliderPane *seek_pane_ = nullptr;
	GstPlayer *player_ = nullptr;
	DiscovererUserParams user_params_ = {nullptr, nullptr, nullptr};
	QAction *play_pause_action_ = nullptr;
	audio::PlayMode play_mode_ = audio::PlayMode::NotSet;
	QComboBox *playlists_cb_ = nullptr;
	QVector<gui::Playlist*> playlists_;
	QStackedLayout *playlist_stack_ = nullptr;
	i32 active_playlist_index_ = -1;
};

}
