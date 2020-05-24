#pragma once

#include "decl.hxx"
#include "../decl.hxx"
#include "../err.hpp"
#include "../types.hxx"

#include <QAbstractTableModel>
#include <QTimer>

#include <type_traits>

namespace quince::gui {

enum class Column : i8 {
	Name = 0,
	Duration,
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
	
	QModelIndex
	index(int row, int column, const QModelIndex &parent) const override;
	
	virtual bool insertRows(int row, int count, const QModelIndex &parent) override {
		mtl_trace();
		return true;
	}
	virtual bool insertColumns(int column, int count, const QModelIndex &parent) override {
		mtl_trace();
		return true;
	}
	virtual bool removeRows(int row, int count, const QModelIndex &parent) override {
		mtl_trace();
		return true;
	}
	virtual bool removeColumns(int column, int count, const QModelIndex &parent) override {
		mtl_trace();
		return true;
	}
	
	void
	SignalRowsInserted(i32 first, i32 last) {
		emit rowsInserted(QModelIndex(), first, last, {});
	}
	
	QVector<Song*>&
	songs() { return songs_; }
	
	void
	UpdateRange(int row1, Column c1, int row2, Column c2);
	
	void
	UpdateRangeDefault(int row) {
		UpdateRange(row, Column::Name, row, Column::Duration);
	}
	
	void
	UpdateRowRange(int row_start, int row_end) {
		UpdateRange(row_start, Column::Name, row_end,
			Column(i8(Column::Count) - 1));
	}
	
private:
	
	void TimerHit();
	bool UpdatePlayingSongPosition();
	
	App *app_ = nullptr;
	QVector<Song*> songs_;
	QTimer *timer_ = nullptr;
	mutable int playing_row_ = -1;
};


}
