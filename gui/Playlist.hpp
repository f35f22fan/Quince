#pragma once

#include "decl.hxx"
#include "../decl.hxx"
#include "../err.hpp"
#include "../types.hxx"

#include <QWidget>

namespace quince::gui {

class Playlist : public QWidget {
public:
	
	Playlist(App *app, const QString &name);
	virtual ~Playlist();
	
	PlaylistActivationOption&
	activation_option() { return activation_option_; }
	
	void
	CreateGui();
	
	Song*
	GetCurrentSong(int *index = nullptr);
	
	bool
	GetFullPath(QString &full_path) const;
	
	bool
	HasSong(Song *song) const;
	
	void
	id(const i64 n) { id_ = n; }
	
	i64
	id() const { return id_; }
	
	const QString&
	name() const { return name_; }
	
	void
	name(const QString &s) { name_ = s; }
	
	void
	PlaylistDoubleClicked(QModelIndex index);
	
	static bool
	QuerySaveFolder(QString &ret_val);
	
	i32
	RemoveAllSongs(); // returns num rows removed
	
	i32
	RemoveSelectedSongs(); // returns num rows removed
	
	QVector<Song*>&
	songs() const;
	
	Table*
	table() const { return table_; }
	
	TableModel*
	table_model() const { return table_model_; }
	
protected:
	virtual void dragEnterEvent(QDragEnterEvent *event) override;
	virtual void dropEvent(QDropEvent *event) override;
	
private:
	NO_ASSIGN_COPY_MOVE(Playlist);
	
	App *app_ = nullptr;
	QString name_;
	TableModel *table_model_ = nullptr;
	Table *table_ = nullptr;
	i64 id_ = -1;
	PlaylistActivationOption activation_option_ = PlaylistActivationOption::None;
	
};
}
