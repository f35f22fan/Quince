#include "TableModel.hpp"

#include "../App.hpp"
#include "../audio.hh"
#include "../audio/Meta.hpp"
#include "../Duration.hpp"
#include "../Song.hpp"
#include "SeekPane.hpp"

#include <QFont>
#include <QTime>
#include <gst/gst.h>

namespace quince::gui {

TableModel::TableModel(App *app, QObject *parent) :
app_(app),
QAbstractTableModel(parent)
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

QModelIndex
TableModel::index(int row, int column, const QModelIndex &parent) const
{
	return createIndex(row, column);
}

int
TableModel::rowCount(const QModelIndex &parent) const
{
	return songs_.size();
}

int
TableModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;
	
	return i8(Column::Count);
}

QVariant
TableModel::data(const QModelIndex &index, int role) const
{
	const Column col = static_cast<Column>(index.column());
	
	if (role == Qt::TextAlignmentRole) {
		if (col == Column::Name)
			return Qt::AlignLeft + Qt::AlignVCenter;
		
		return Qt::AlignLeft /*Qt::AlignHCenter*/ + Qt::AlignVCenter;
	}
	
	const int row = index.row();
	
	if (row >= songs_.size())
		return {};
	
	auto *song = songs_[row];
	audio::Meta &meta = song->meta();
	
	if (role == Qt::DisplayRole)
	{
		if (col == Column::Name) {
			return song->display_name();
		} else if (col == Column::Duration) {
			QString s;
			if (meta.is_duration_set()) {
				auto d = Duration::FromNs(song->meta().duration());
				s = d.toDurationString();
			}
			
			if (song->is_playing_or_paused()) {
				s.append(' ');
				playing_row_ = row;
				auto d = Duration::FromNs(song->playing_at());
				const QString dstr = d.toDurationString();
				
				if (song->is_paused())
					s.append('|').append(dstr).append('|');
				else
					s.append('[').append(dstr).append(']');
			}
			
			return s;
		} else if (col == Column::Bitrate) {
			const i32 bitrate = meta.bitrate();
			
			if (bitrate != -1) {
				return QString::number(bitrate / 1000)
					+ QLatin1String(" kbps");
			}
		} else if (col == Column::Channels) {
			if (meta.channels() != -1)
				return QString::number(meta.channels());
		} else if (col == Column::BitsPerSample) {
			if (meta.bits_per_sample() != -1)
				return QString::number(meta.bits_per_sample());
		} else if (col == Column::SampleRate) {
			if (meta.sample_rate() != -1) {
				return QString::number(meta.sample_rate());
			}
		} else if (col == Column::Genre) {
			return audio::GenreToString(meta.genre());
		}
		
		return QVariant();
	} else if (role == Qt::FontRole) {
		QFont font;
		
		if (song->is_playing_or_paused())
			font.setBold(true);
		
		return font;
	}
	
	return {};
}

QVariant
TableModel::headerData(int section_i, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole)
	{
		if (orientation == Qt::Horizontal)
		{
			const Column section = static_cast<Column>(section_i);
			
			switch (section) {
			case Column::Name:
				return QLatin1String("Name");
			case Column::Duration:
				return QLatin1String("Duration");
			case Column::Bitrate:
				return QLatin1String("Bitrate");
			case Column::Channels:
				return QLatin1String("Channels");
			case Column::BitsPerSample:
				return QLatin1String("Bits Per Sample");
			case Column::SampleRate:
				return QLatin1String("Sample Rate");
			case Column::Genre:
				return QLatin1String("Genre");
			default: {
				mtl_trace();
				return {};
			}
			}
		}
		return QString::number(section_i + 1);
	}
	return {};
}

void
TableModel::TimerHit()
{
	if (app_->slider_pane()->slider_dragged_by_user())
		return;
	
	if (playing_row_ >= songs_.size())
		return;
	
	app_->UpdatePlayingSongPosition(-1);
	UpdateRange(playing_row_, Column::Duration, playing_row_, Column::Duration);
}

void
TableModel::UpdateRange(int row1, Column c1, int row2, Column c2)
{
	int first, last;
	
	if (row1 > row2) {
		first = row2;
		last = row1;
	} else {
		first = row1;
		last = row2;
	}
	
	const QModelIndex top_left = createIndex(first, int(c1));
	const QModelIndex bottom_right = createIndex(last, int(c2));
	emit dataChanged(top_left, bottom_right, {Qt::DisplayRole});
}

}
