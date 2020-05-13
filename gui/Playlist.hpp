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
	
	void CreateGui();
	
	Song* GetCurrentSong(int *index);
	
	const QString& name() const { return name_; }
	
	void
	PlaylistDoubleClicked(QModelIndex index);
	
	Table* table() const { return table_; }
	TableModel* table_model() const { return table_model_; }
	
private:
	NO_ASSIGN_COPY_MOVE(Playlist);
	
	App *app_ = nullptr;
	QString name_;
	TableModel *table_model_ = nullptr;
	Table *table_ = nullptr;
};
}
