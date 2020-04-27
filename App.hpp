#pragma once

#include "decl.hxx"
#include "err.hpp"
#include "gui/decl.hxx"
#include "types.hxx"

#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

#include <QMainWindow>
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
	
	bool
	AddBatch(QVector<quince::SongItem*> &vec);
	
	QVector<SongItem*>*
	current_playlist_songs();
	
	void
	GotAudioInfo(AudioInfo *info);
	
	SongItem*
	GetPlayingSong();
	
	bool
	InitDiscoverer();
	
	void
	MessageAsyncDone();
	
	void
	PlaylistDoubleClicked(QModelIndex index);
	
	GstElement*
	play_elem() const;
	
	void
	ReachedEndOfStream();
	
private:
	
	void AddAction(QToolBar *tb, const QString &icon_name, const QString &action_name);
	bool CreateGui();
	QToolBar* CreateMediaActionsToolBar();
	QToolBar* CreatePlaylistActionsToolBar();
	bool LoadSavedSongData();
	void ProcessAction(const QString &action_name);
	
	NO_ASSIGN_COPY_MOVE(App);
	
	gui::SliderPane *slider_pane_ = nullptr;
	gui::Table *table_ = nullptr;
	gui::TableModel *table_model_ = nullptr;
	GstPlayer *player_ = nullptr;
	DiscovererUserParams user_params_ = {nullptr, nullptr, nullptr};
};

}
