#pragma once

#include "decl.hxx"
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
	Count
};

class TableModel: public QAbstractTableModel {
	Q_OBJECT
public:
	TableModel(QObject *parent = nullptr);
	virtual ~TableModel();
	
	int
	rowCount(const QModelIndex &parent = QModelIndex()) const override;
	
	int
	columnCount(const QModelIndex &parent = QModelIndex()) const override;
	
	QVariant
	data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	
	QVariant
	headerData(int section, Qt::Orientation orientation, int role) const override;
	
	QVector<SongItem*>&
	songs() { return songs_; }
	
	void
	UpdateRange(int row1, int row2, Column c);
	
private:
	
	void TimerHit();
	
	QVector<SongItem*> songs_;
	QTimer *timer_ = nullptr;
	mutable int playing_row_ = -1;
};


}
