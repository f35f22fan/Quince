#pragma once

#include <QMainWindow>

#include "decl.hxx"
#include "err.hpp"
#include "gui/decl.hxx"

namespace quince {

class App : public QMainWindow {
	
public:
	App(int argc, char *argv[]);
	virtual ~App();
	
	void
	PlaylistDoubleClicked(QModelIndex index);
	
private:
	
	void AddAction(QToolBar *tb, const QString &icon_name, const QString &action_name);
	bool CreateGui();
	QToolBar* CreateMediaActionsToolBar();
	QToolBar* CreatePlaylistActionsToolBar();
	bool LoadSavedSongData();
	void ProcessAction(const QString &action_name);
	
	NO_ASSIGN_COPY_MOVE(App);
	
	gui::Table *table_ = nullptr;
	gui::TableModel *table_model_ = nullptr;
	GstPlayer *player_ = nullptr;
};

}
