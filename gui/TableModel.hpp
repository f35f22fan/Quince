#pragma once

#include "decl.hxx"
#include "../decl.hxx"
#include "../err.hpp"
#include "../types.hxx"

#include <QAbstractTableModel>
#include <QTimer>

#include <type_traits>

namespace quince::gui {

enum Column {
	Name = 0,
	Duration,
	PlayingAt,
	Bitrate,
	Channels,
	BitsPerSample,
	SampleRate,
	Genre,
	Count
};

class TableModel: public QAbstractTableModel {
	Q_OBJECT
public:
	TableModel(App *app, QObject *parent = nullptr);
	virtual ~TableModel();
	
	int
	rowCount(const QModelIndex &parent = QModelIndex()) const override;
	
	int
	columnCount(const QModelIndex &parent = QModelIndex()) const override;
	
	QVariant
	data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	
	QVariant
	headerData(int section, Qt::Orientation orientation, int role) const override;
	
	QVector<Song*>&
	songs() { return songs_; }
	
	void
	UpdateRange(int row1, Column c1, int row2, Column c2);
	
private:
	
	void TimerHit();
	bool UpdatePlayingSongPosition();
	
	App *app_ = nullptr;
	QVector<Song*> songs_;
	QTimer *timer_ = nullptr;
	mutable int playing_row_ = -1;
};


}
