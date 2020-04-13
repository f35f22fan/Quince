#include "TableModel.hpp"

#include "../Duration.hpp"
#include "SongItem.hpp"

#include <QFont>
#include <QTime>

namespace quince::gui {

TableModel::TableModel(QObject *parent) : QAbstractTableModel(parent)
{
	timer_ = new QTimer(this);
	timer_->setInterval(1000);
	connect(timer_, &QTimer::timeout, this, &TableModel::TimerHit);
	timer_->start();
}

TableModel::~TableModel()
{
	for (auto *song: songs_)
		delete song;
	
	songs_.clear();
}

int
TableModel::rowCount(const QModelIndex &parent) const
{
	return songs_.size();
}

int
TableModel::columnCount(const QModelIndex &parent) const
{
	return Column::Count;
}

QVariant
TableModel::data(const QModelIndex &index, int role) const
{
	const int row = index.row();
	const int col = index.column();
	
	if (row >= songs_.size())
	{
		return QVariant();
	}
	
	auto *song = songs_[row];
	
	if (role == Qt::DisplayRole)
	{
		if (col == Column::Name) {
			return song->display_name();
		} else if (col == Column::Duration) {
			auto d = Duration::FromNs(song->duration_ns());
			return d.toDurationString();
		} else if (col == Column::PlayingAt) {
			if (song->playing()) {
				playing_row_ = row;
				return QTime::currentTime().toString();
			}
		}
		
		return QVariant();
	} else if (role == Qt::TextAlignmentRole) {
		
		if (col == Column::Name)
			return Qt::AlignLeft + Qt::AlignVCenter;
		
		return Qt::AlignLeft /*Qt::AlignHCenter*/ + Qt::AlignVCenter;
	} else if (role == Qt::FontRole) {
		QFont font;
		
		if (song->playing())
			font.setBold(true);
		
		return font;
	}
	
	return QVariant();
}

QVariant
TableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole) {
		
		if (orientation == Qt::Horizontal)
		{
			switch (section) {
			case Column::Name:
				return QString("Name");
			case Column::Duration:
				return QString("Duration");
			case Column::PlayingAt:
				return QString("Playing At");
			}
		} else {
			return QString::number(section + 1);
		}
	}
	return QVariant();
}

void
TableModel::TimerHit()
{
	if (playing_row_ >= songs_.size())
		return;
	
	QModelIndex top_left = createIndex(playing_row_, Column::PlayingAt);
	emit dataChanged(top_left, top_left, {Qt::DisplayRole});
}

void
TableModel::UpdateRange(int row1, int row2, Column c)
{
	int first, last;
	
	if (row1 > row2) {
		first = row2;
		last = row1;
	} else {
		first = row1;
		last = row2;
	}
	
	QModelIndex top_left = createIndex(first, c);
	QModelIndex bottom_right = createIndex(last, c);
	emit dataChanged(top_left, bottom_right, {Qt::DisplayRole});
}

}
